



#include <math.h>
#include <stdlib.h>

#include <sstream>  // NOLINT. TODO(yiwang): Remove the use of ostringstream.
#include <string>

#include "boost/filesystem.hpp"
#include "boost/program_options/option.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"

#include "base/common.h"
#include "mrml/mrml_filesystem.h"
#include "mrml/mrml_recordio.h"
#include "mrml-lasso/logistic_regression.pb.h"
#include "mrml-lasso/mrml_mappers_and_reducers.h"
#include "mrml-lasso/sparse_vector_tmpl.h"

using std::string;
using std::istringstream;
using std::ostringstream;
using std::stringstream;
using std::setw;
using std::setfill;

namespace logistic_regression {

const char* kUniqueKey = "";

REGISTER_MAPPER(ComputeDenseGradientMapper);
REGISTER_REDUCER(UpdateDenseModelReducer);
REGISTER_MAPPER(ComputeSparseGradientMapper);
REGISTER_REDUCER(UpdateSparseModelReducer);

//---------------------------------------------------------------------------
// RegularizationFactor computes the value of L1-regularization term.
//---------------------------------------------------------------------------

typedef Learner<SparseRealVector> SparseLearnerStates;
typedef Learner<DenseRealVector>  DenseLearnerStates;

template <class RealVector>
double RegularizationFactor(const RealVector* s);

template <class RealVector>
void ResizeRealVector(RealVector* s, int size);

template <>
double
RegularizationFactor<DenseLearnerStates>(const DenseLearnerStates* s) {
  double ret = 0;
  for (size_t i = 0; i < s->new_x().size(); ++i) {
    ret += fabs(s->new_x()[i]);
  }
  return ret * s->l1weight();
}

template <>
double
RegularizationFactor<SparseLearnerStates>(const SparseLearnerStates* s) {
  double ret = 0;
  for (SparseRealVector::const_iterator i = s->new_x().begin();
       i != s->new_x().end(); ++i) {
    ret += fabs(i->second);
  }
  return ret * s->l1weight();
}

template <>
void
ResizeRealVector<DenseRealVector>(DenseRealVector* s, int size) {
  if (size > 0)
    s->resize(size, 0);
}

template <>
void
ResizeRealVector<SparseRealVector>(SparseRealVector* s, int size) {
  // There is nothing to do, because sparse vector needs no
  // pre-operation space allocation.
}

static string GetInitialStatesFilename(const CommandLineOptions& options) {
  ostringstream output;
  output << options.base_dir << "/" << options.states_filebase << "-"
         << setw(5) << setfill('0') << 0;
  return output.str();
}

static string IncrementSuffixNumber(const string& states_filename) {
  size_t found = states_filename.find_last_of("-");
  int suffix = atoi(states_filename.substr(found + 2).c_str()) + 1;
  ostringstream output;
  output << states_filename.substr(0, found) << "-"
         << setw(5) << setfill('0') << suffix;
  return output.str();
}

// Set *states_filename to empty string "", if no file exists in
// directory base_dir.  Assume that the states_filename has been
// copied to base_dir between iterations by external script.
static bool FindMostRecentLearnerStatesFile(const CommandLineOptions& options,
                                            string* states_filename) {
  namespace fs = boost::filesystem;

  if (!fs::is_directory(options.base_dir)) {
    LOG(ERROR) << "\"" << options.base_dir << "\" is not a valid directory.";
    return false;
  }

  *states_filename = "";  // If no file in base_dir, returns an empty string.

  for (fs::directory_iterator itr(options.base_dir);
       itr != fs::directory_iterator(); ++itr) {
    if (is_regular_file(itr->status()) &&
        itr->path().filename().string().substr(
            0, options.states_filebase.size()) ==
        options.states_filebase) {
      if (itr->path().filename().string() > *states_filename) {
        *states_filename = itr->path().filename().string();
      }
    }
  }
  return true;
}

template <class RealVector>
ComputeGradientMapper<RealVector>::ComputeGradientMapper() {
  options_.Parse(GetConfig());

  combined_loss_ = 0;
  string recent_states_filename;
  CHECK(FindMostRecentLearnerStatesFile(options_, &recent_states_filename));

  // Note: feature_weights_ is LearnerStates::new_x instead of
  // LearnerStates::x, because the evaluation map stage is likely
  // probing the effectiveness of new_x in line search.
  if (recent_states_filename.empty()) {
    feature_weights_.clear();
  } else {
    LOG(INFO) << "Load states file: " << recent_states_filename
              << " under directory: " << options_.base_dir;
    MRMLFS_File file(options_.base_dir + "/" + recent_states_filename, true);
    states_.LoadFromRecordFile(&file);
    feature_weights_ = states_.new_x();
  }
  combined_gradient_.clear();
  partial_gradient_.clear();
}

static void ParseInstanceFromProtoBufEncode(const string& line,
                                            float* num_positive,
                                            float* num_appearance,
                                            SparseRealVector* features) {
  InstancePB instance;
  CHECK(instance.ParseFromString(line));
  *num_positive = instance.num_positive();
  *num_appearance = instance.num_appearance();
  features->clear();
  for (int i = 0; i < instance.feature_size(); ++i) {
    features->set(instance.feature(i).id(), instance.feature(i).value());
  }
}

static void ParseInstanceFromText(const string& line,
                                  float* num_positives,
                                  float* num_appearances,
                                  SparseRealVector* features) {
  istringstream line_parser(line);
  line_parser >> *num_positives >> *num_appearances;

  int    feature_name;
  double feature_value;
  while (line_parser >> feature_name >> feature_value) {
    features->set(feature_name, feature_value);
  }
}
template <class RealVector>
void ComputeGradientMapper<RealVector>::Map(const std::string& key,
                                            const std::string& value) {
  float num_positives;
  float num_appearances;
  SparseRealVector features;
  if (GetInputFormat() == RecordIO)
    ParseInstanceFromProtoBufEncode(value,
                                    &num_positives, &num_appearances,
                                    &features);
  else
    ParseInstanceFromText(value,
                          &num_positives, &num_appearances,
                          &features);

  // We have a convention that if num_positive is a negative value,
  // the map input is considered `no label'.
  if (num_positives < 0) {
    return;
  }

  if (num_positives > num_appearances) {
    LOG(ERROR) << "Skip instance with invalid num_positives/num_appearances: "
               << num_positives << " / " << num_appearances;
    return;
  }

  double dot_x_b = DotProduct(features, feature_weights_);
  double partial_loss = 0;

  float num_negatives = num_appearances - num_positives;
  double inc_loss, inc_prob;
  double score = dot_x_b;
  partial_gradient_.clear();

  // process the positives instances
  if (num_positives > 0) {
    if (score < -30) {
      inc_loss = -score;
      inc_prob = 0;
    } else if (score > 30) {
      inc_loss = 0;
      inc_prob = 1;
    } else {
      double temp = 1.0 + exp(-score);
      inc_loss = log(temp);
      inc_prob = 1.0/temp;
    }
    partial_loss += inc_loss * num_positives;
    AddScaled(&partial_gradient_, features,
              -1 * num_positives * (1.0 - inc_prob));
  }

  if (num_negatives > 0) {
    score *= -1;
    if (score < -30) {
      inc_loss = -score;
      inc_prob = 0;
    } else if (score > 30) {
      inc_loss = 0;
      inc_prob = 1;
    } else {
      double temp = 1.0 + exp(-score);
      inc_loss = log(temp);
      inc_prob = 1.0/temp;
    }
    partial_loss += inc_loss * num_negatives;
    AddScaled(&partial_gradient_, features, num_negatives * (1.0 - inc_prob));
  }

  AddScaled(&combined_gradient_, partial_gradient_, 1);
  combined_loss_ += partial_loss;
}

template <class RealVector>
void ComputeGradientMapper<RealVector>::Flush() {
  int vec_size = combined_gradient_.size();
  int fragment_num = vec_size/kMessageSize +
                     ((vec_size % kMessageSize == 0) ? 0 : 1);
  LOG(INFO) << "MapOutput: vector size: " << vec_size
            << ", fragment_num: " << fragment_num;
  for (int i = 0; i < fragment_num; ++i) {
    ComputeGradientMapperOutputPB output;
    output.mutable_partial_gradient()->Clear();
    for (size_t j = i * kMessageSize; j < (i + 1) * kMessageSize
         && j < vec_size; ++j) {
      const double& value = combined_gradient_[j];
      if (value != 0) {
        RealVectorPB::Element* e =
          output.mutable_partial_gradient()->add_element();
        e->set_index(j);
        e->set_value(value);
      }
    }
    output.mutable_partial_gradient()->set_dim(vec_size);
    output.set_partial_loss(combined_loss_/(double)fragment_num);
    string output_buffer;
    output.SerializeToString(&output_buffer);
    Output(kUniqueKey, output_buffer);
  }
}

template <class RealVector>
void* UpdateModelReducer<RealVector>::BeginReduce(const std::string& key,
                                                  const std::string& value) {
  PartialReduceInfo* r = new PartialReduceInfo;
  r->word_ = key;

  // Try to find the most recently updated learner states.
  string recent_states_filename;
  CHECK(FindMostRecentLearnerStatesFile(options_, &recent_states_filename));

  if (recent_states_filename.empty()) {
    // If the states do not exist, we are doing the intialization
    // iteration, and we need to create states from configurations.
    r->learner = new Learner<RealVector>(initial_x_,
                                         options_.memory_size,
                                         options_.l1weight,
                                         options_.max_line_search_steps,
                                         options_.max_iterations,
                                         options_.convergence_tolerance,
                                         options_.max_feature_number);
    r->value = 0;
  } else {
    // If there has been an "most recently updated" states file,
    // load it and update it.
    LOG(INFO) << "Load states file: " << recent_states_filename
              << " under directory: " << options_.base_dir;

    MRMLFS_File file(options_.base_dir + "/" + recent_states_filename, true);
    r->learner = new Learner<RealVector>;
    r->learner->LoadFromRecordFile(&file);
    r->value = 0;
  }

  r->value = 1.0;
  r->gradient.clear();

  partial_gradient_.clear();
  ComputeGradientMapperOutputPB partial_output;
  CHECK(partial_output.ParseFromString(value));
  partial_gradient_.ParseFromProtoBuf(partial_output.partial_gradient());
  if (options_.max_feature_number > 0) {
    ResizeRealVector(&(r->gradient), options_.max_feature_number);
    ResizeRealVector(&partial_gradient_, options_.max_feature_number);
  }
  AddScaled(&(r->gradient), partial_gradient_, 1);
  r->value += partial_output.partial_loss();
  return r;
}

template <class RealVector>
void UpdateModelReducer<RealVector>::PartialReduce(const std::string& key,
                                                   const std::string& value,
                                                   void* partial_result) {
  ComputeGradientMapperOutputPB partial_output;
  CHECK(partial_output.ParseFromString(value));
  partial_gradient_.clear();
  partial_gradient_.ParseFromProtoBuf(partial_output.partial_gradient());
  if (options_.max_feature_number > 0) {
    ResizeRealVector(
        &(static_cast<PartialReduceInfo*>(partial_result)->gradient),
        options_.max_feature_number);
    ResizeRealVector(&partial_gradient_, options_.max_feature_number);
  }
  AddScaled(&(static_cast<PartialReduceInfo*>(partial_result)->gradient),
            partial_gradient_, 1);
  static_cast<PartialReduceInfo*>(partial_result)->value +=
    partial_output.partial_loss();
}

template <class RealVector>
void UpdateModelReducer<RealVector>::EndReduce(const std::string& key,
                                               void* partial_result) {
  PartialReduceInfo* p = static_cast<PartialReduceInfo*>(partial_result);
  p->value += RegularizationFactor(p->learner);
  p->learner->SetObjectiveValueAndGradient(p->value, &(p->gradient));

  string recent_states_filename;
  CHECK(FindMostRecentLearnerStatesFile(options_, &recent_states_filename));
  if (recent_states_filename.empty())
    p->learner->Initialize(options_.flag_file.c_str());
  else
    p->learner->GradientDescent(options_.flag_file.c_str());

  string new_states_filename = "";
  if (recent_states_filename.empty()) {
    new_states_filename = GetInitialStatesFilename(options_);
  } else {
    new_states_filename = IncrementSuffixNumber(recent_states_filename);
    new_states_filename = options_.base_dir + "/" + new_states_filename;
  }

  LOG(INFO) << "Write LearnerStates into: " << new_states_filename;
  MRMLFS_File file(new_states_filename, false);
  CHECK(file.IsOpen());
  p->learner->SaveIntoRecordFile(&file);

  if (p->learner)
    delete p->learner;
  if (p)
    delete p;
}

}  // namespace logistic_regression
