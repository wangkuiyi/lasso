



#include <math.h>
#include <stdlib.h>

#include <sstream>  // NOLINT.  TODO(leostarzhou): Use SplitString in /strutil
                    //  to substitute the usage of istringstream.
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
#include "mrml-lasso/mr_assign_feature_id.h"

using std::map;
using std::string;
using std::istringstream;
using std::ostringstream;
using std::stringstream;
using std::setw;
using std::setfill;

namespace logistic_regression {

REGISTER_MAPPER(AssignFeatureIDMapper);
REGISTER_REDUCER(AssignFeatureIDReducer);

AssignFeatureIDMapper::AssignFeatureIDMapper() {
  options_.Parse(GetConfig());
  feature_dict_.clear();
}

void AssignFeatureIDMapper::Map(const std::string& key,
                                const std::string& value) {
  float num_positive;
  float num_appearance;
  string feature_name;
  double feature_value;
  int feature_num = 0;
  InstancePB instance;

  if (GetInputFormat() == Text) {
    istringstream line_parser(value);
    line_parser >> num_positive >> num_appearance;
    while (line_parser >> feature_name >> feature_value) {
      if (!feature_dict_.count(feature_name)) {
        ++feature_num;
        feature_dict_[feature_name] = feature_num;
      }
    }
  } else if (GetInputFormat() == RecordIO) {
    CHECK(instance.ParseFromString(value));
    for (int i = 0; i < instance.feature_size(); ++i) {
      feature_name = instance.feature(i).name();
      if (!feature_dict_.count(feature_name)) {
        ++feature_num;
        feature_dict_[feature_name] = feature_num;
      }
    }
  }
}

void AssignFeatureIDMapper::Flush() {
  map<string, int>::const_iterator map_it = feature_dict_.begin();
  while (map_it != feature_dict_.end()) {
    Output(kUniqueKey, map_it->first);
    ++map_it;
  }
}

AssignFeatureIDReducer::AssignFeatureIDReducer() {
  options_.Parse(GetConfig());
  feature_dict_.clear();
  feature_num_ = 0;
}

void* AssignFeatureIDReducer::BeginReduce(const std::string& key,
                                          const std::string& value) {
  if (!feature_dict_.count(value)) {
    ++feature_num_;
    feature_dict_[value] = feature_num_;
  }
  return NULL;                  // No intermediate result.
}

void AssignFeatureIDReducer::PartialReduce(const std::string& key,
                                           const std::string& value,
                                           void* partial_result) {
  if (!feature_dict_.count(value)) {
    ++feature_num_;
    feature_dict_[value] = feature_num_;
  }
}

void AssignFeatureIDReducer::EndReduce(const std::string& key,
                                       void* partial_result) {
  map<string, int>::const_iterator map_it = feature_dict_.begin();
  while (map_it != feature_dict_.end()) {
    ostringstream oss;
    if (GetOutputFormat() == RecordIO) {
      oss << map_it->second;
      Output(map_it->first,oss.str());
    } else {
      oss << map_it->first << "\t" << map_it->second;
      Output("", oss.str());
    }
    ++map_it;
  }
}

}  // namespace logistic_regression
