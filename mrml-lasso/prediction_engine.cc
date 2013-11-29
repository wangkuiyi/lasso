


#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <fstream>         // NOLINT. TODO(leostarzhou): Use fopen API.
#include <map>
#include <sstream>
#include <vector>

#include "mrml-lasso/prediction_engine.h"

using std::map;
using std::vector;
using std::string;
using std::ifstream;
using std::istringstream;

namespace logistic_regression {

PredictionEngine::PredictionEngine(const string& model_file_name) {
  ifstream input(model_file_name.c_str());
  if (!input.is_open()) {
    printf("Fatal:model file name is valid!\n");
    return;
  };

  feature_weights_.clear();
  string line;
  string value;
  getline(input, line);
  istringstream line_parser(line);
  size_t pos = 0;
  while (line_parser >> value) {
    if ((pos = value.find(":", 0)) != string::npos) {
      string feature_name = value.substr(0, pos);
      string feature_value = value.substr(pos+1, value.size()+1-pos);
      int index = atoi(feature_name.c_str());
      double val = atof(feature_value.c_str());
      feature_weights_[index] = val;
    }
  }
}

PredictionEngine::~PredictionEngine() {
  feature_weights_.clear();
}

int PredictionEngine::Predict(const vector<int>& feature_names,
                              const vector<double>& feature_values,
                              double& ret) {
  if (feature_names.size() != feature_values.size()) {
    printf("Fatal: input parameters are invalid!\n");
    return -1;
  }
  ret = 0;
  vector<int>::const_iterator iter1 = feature_names.begin();
  vector<double>::const_iterator iter2 = feature_values.begin();
  while (iter1 != feature_names.end()) {
    ret += *iter2 * feature_weights_[*iter1];
    ++iter1;
    ++iter2;
  }
  ret = 1.0/(1 + exp(-ret));
  return 0;
}

}  // namespace logistic_regression
