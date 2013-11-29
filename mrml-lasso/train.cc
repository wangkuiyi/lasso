

//
// This program demonstrates how to use Learner<SparseRealVector> and
// LearnerStates<SparseRealVector> to implement a L1-regularized
// logistic regression training algorithm.
//
#include <math.h>
#include <stdlib.h>

#include <vector>
#include <fstream>             // NOLINT. TODO(yiwang): use fopen API.
#include <sstream>             // NOLINT. TODO(yiwang): use SplitString.
#include <string>

#include "boost/program_options/option.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"

#include "base/common.h"

#include "mrml-lasso/learner.h"
#include "mrml-lasso/learner_sparse_impl.h"
#include "mrml-lasso/learner_dense_impl.h"
#include "mrml-lasso/vector_types.h"

using std::vector;
using std::ifstream;
using std::istringstream;
using std::string;

namespace logistic_regression {

typedef LearnerStates<SparseRealVector> SparseLearnerStates;
typedef LearnerStates<DenseRealVector>  DenseLearnerStates;

void ResizeRealVector(DenseRealVector* s, int size) {
  if (size > 0)
    s->resize(size, 0);
}

//---------------------------------------------------------------------------
// TrainingData consists of training instances (feature vector and
// label).  The feature vectors are SparseRealVector.  There are two
// realizations of function template EvaluateObjective: one accepts
// a sparse model, the other accepts a dense one.
//---------------------------------------------------------------------------

class TrainingData {
  friend void EvaluateObjective(const TrainingData& data,
                                const LearnerStates<DenseRealVector>& states,
                                double* value,
                                DenseRealVector* gradient);
 public:
  void LoadTrainingData(const string& filename,
                        const bool if_feature_binary = true);
  int dim() const { return dim_; }

 private:
  struct Instance {
    float num_positives_;
    float num_appearences_;
    vector<int32> feature_name_;
    vector<float> feature_value_;
  };

  int dim_;
  bool if_feature_binary_;
  vector<Instance> data_;
};


void TrainingData::LoadTrainingData(const string& filename,
                                    const bool if_feature_binary) {
  ifstream input(filename.c_str());
  CHECK(input.is_open());

  data_.clear();
  dim_ = 0;
  if_feature_binary_ = if_feature_binary;
  string line;
  while (getline(input, line)) {
    istringstream line_parser(line);
    data_.push_back(Instance());
    line_parser >> data_.back().num_positives_
                >> data_.back().num_appearences_;
    if (data_.back().num_positives_ > data_.back().num_appearences_) {
      data_.pop_back();
      continue;
    }

    int32 feature_name;
    float feature_value;
    while (line_parser >> feature_name >> feature_value) {
      dim_ = std::max(dim_, feature_name);
      data_.back().feature_name_.push_back(feature_name);
      if (!if_feature_binary)
        data_.back().feature_value_.push_back(feature_value);
    }
  }
  ++dim_;            // convert max-feature-index into num-features.
}

//---------------------------------------------------------------------------
// RegularizationFactor computes the value of L1-regularization term.
//---------------------------------------------------------------------------

double
RegularizationFactor(const DenseLearnerStates& s) {
  double ret = 0;
  for (size_t i = 0; i < s.new_x().size(); ++i) {
    ret += fabs(s.new_x()[i]);
  }
  return ret * s.l1weight();
}

//---------------------------------------------------------------------------
// EvaluateObjective computes the value and gradient of the LASSO
// objective function.
//---------------------------------------------------------------------------

void EvaluateObjective(const TrainingData& data,
                       const LearnerStates<DenseRealVector>& states,
                       double* value,
                       DenseRealVector* gradient) {
  gradient->clear();
  *value = 1.0;

  // Compute value and gradient of the logistic loss function.
  for (int i = 0; i < data.data_.size(); ++i) {
    double dot_feature_model = 0;
    for (int j = 0; j < data.data_[i].feature_name_.size(); ++j) {
      int32 feature_name = data.data_[i].feature_name_[j];
      double feature_value = 1.0;
      if (!data.if_feature_binary_)
        feature_value = data.data_[i].feature_value_[j];
      if (feature_name < states.new_x().size())
        dot_feature_model += states.new_x()[feature_name] * feature_value;
      else
        break;
    }

    float num_positives = data.data_[i].num_positives_;
    float num_negatives = data.data_[i].num_appearences_
                          - data.data_[i].num_positives_;
    double score = dot_feature_model;
    double insLoss, insProb;
    // process the positives instances
    if (num_positives > 0) {
      if (score < -30) {
        insLoss = -score;
        insProb = 0;
      } else if (score > 30) {
        insLoss = 0;
        insProb = 1;
      } else {
        double temp = 1.0 + exp(-score);
        insLoss = log(temp);
        insProb = 1.0/temp;
      }
      *value += insLoss * num_positives;
      for (int k = 0; k < data.data_[i].feature_name_.size(); ++k) {
        int32 feature_name = data.data_[i].feature_name_[k];
        double feature_value = 1.0;
        if (!data.if_feature_binary_)
          feature_value = data.data_[i].feature_value_[k];
        while (gradient->size() <= feature_name)
          gradient->push_back(0);
        (*gradient)[feature_name] +=
            feature_value * (-1 * num_positives * (1.0 - insProb));
      }
    }

    if (num_negatives > 0) {
      score *= -1;
      if (score < -30) {
        insLoss = -score;
        insProb = 0;
      } else if (score > 30) {
        insLoss = 0;
        insProb = 1;
      } else {
        double temp = 1.0 + exp(-score);
        insLoss = log(temp);
        insProb = 1.0/temp;
      }
      *value += insLoss * num_negatives;
      for (int k = 0; k < data.data_[i].feature_name_.size(); ++k) {
        int32 feature_name = data.data_[i].feature_name_[k];
        double feature_value = 1.0;
        if (!data.if_feature_binary_)
          feature_value = data.data_[i].feature_value_[k];
        while (gradient->size() <= feature_name)
          gradient->push_back(0);
        (*gradient)[feature_name] +=
            feature_value * num_negatives * (1.0 - insProb);
      }
    }
  }
  ResizeRealVector(gradient, data.dim());
  *value += RegularizationFactor(states);
}

//---------------------------------------------------------------------------
// The training procedure
//---------------------------------------------------------------------------

bool DoesFileExist(const char* filename) {
  string cmd_test_file = string("test -e ") + filename;
  return NULL == fopen(filename, "r");
}

void Train(const TrainingData& training_data,
           const DenseRealVector& initial_x, const double l1_weight) {
  static const char* kTerminationFlagFilename = "./tmp/term-lr";

  Learner<DenseRealVector> learner(initial_x,
                              10,  // int memory_size,
                              l1_weight,  // double l1_weight,
                              20,  // int max_line_search_steps,
                              120,  // int max_iterations,
                              1e-4,  // double convergence_tolerance
                              initial_x.size());

  double value;
  DenseRealVector gradient;
  EvaluateObjective(training_data, learner, &value, &gradient);
  learner.SetObjectiveValueAndGradient(value, &gradient);
  learner.Initialize(kTerminationFlagFilename);
  int count = 0;
  while (DoesFileExist(kTerminationFlagFilename)) {
    count++;
    int iteration = learner.iteration();
    int search_step = learner.line_search_step();
    std::cout << "iter ************************* \t" << count
              << "\t" << iteration << "\t" << search_step << "\n";
    EvaluateObjective(training_data, learner, &value, &gradient);
    learner.SetObjectiveValueAndGradient(value, &gradient);
    learner.GradientDescent(kTerminationFlagFilename);
    std::cout << "." << std::flush;
  }
  std::ofstream fout("./lr_model");
  fout << learner.new_x();
}

}  // namespace logistic_regression

//----------------------------------------------------------------------------
// The regression test entry point
//----------------------------------------------------------------------------

int main(int argc, char** argv) {
  using logistic_regression::Train;
  using logistic_regression::SparseRealVector;
  using logistic_regression::DenseRealVector;
  using logistic_regression::TrainingData;
  namespace po = boost::program_options;

  static const char* kTerminationFlagFilename = "./tmp/term-lr";
  if (NULL != fopen(kTerminationFlagFilename, "r")) {
    std::cout << "plz remove ./tmp/term-lr first.\n";
    return 0;
  }
  system("mkdir -p ./tmp");

  po::options_description desc("Supported options");
  desc.add_options()
      ("help", "Produce help message.")
      ("l1_weight", po::value<string>(), "l1_weight")
      ("if_feature_binary", po::value<bool>(), "if_feature_binary")
      ("input_data", po::value<string>(), "the training data file name");
  po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(desc).allow_unregistered().
      run();
  po::variables_map vm;
  po::store(parsed, vm);
  po::notify(vm);

  CHECK(vm.count("input_data"));
  CHECK(vm.count("l1_weight"));

  double l1_weight = atof((vm["l1_weight"].as<string>()).c_str());
  TrainingData training_data;
  if (vm["if_feature_binary"].as<bool>())
    training_data.LoadTrainingData(vm["input_data"].as<string>(), true);
  else
    training_data.LoadTrainingData(vm["input_data"].as<string>(), false);

  DenseRealVector initial_x(training_data.dim(), 0);
  Train(training_data, initial_x, l1_weight);

  return 0;
}
