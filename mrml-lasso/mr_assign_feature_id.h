

//
// Define the mappers and reducers to assign id for each feature.
// Mappers input: the training data. (data format: text or recordio)
// Reducers output: the feature_id mapping file. (data format: recordio)

#ifndef MRML_LASSO_MR_ASSIGN_FEATURE_ID_H_
#define MRML_LASSO_MR_ASSIGN_FEATURE_ID_H_

#include <map>
#include <string>

#include "base/common.h"
#include "strutil/split_string.h"
#include "mrml/mrml.h"
#include "mrml-lasso/command_line_options.h"

namespace logistic_regression {

extern const char* kUniqueKey;

// AssignFeatureIDMapper generates the "feature to id" mapping file.
// It is necessary to run AssignFeatureID and ConvertDataFormat
// MR tasks before mrml-lr starts running.
class AssignFeatureIDMapper : public MRML_Mapper {
 public:
  AssignFeatureIDMapper();
  void Map(const std::string& key, const std::string& value);
  void Flush();

 private:
  std::map<std::string, int> feature_dict_;
  CommandLineOptions options_;
};

class AssignFeatureIDReducer : public MRML_Reducer {
 public:
  AssignFeatureIDReducer();
  void* BeginReduce(const std::string& key, const std::string& value);
  void PartialReduce(const std::string& key, const std::string& value,
                     void* partial_result);
  void EndReduce(const std::string& key, void* partial_result);

 private:
  std::map<std::string, int> feature_dict_;
  CommandLineOptions options_;
  int feature_num_;
};

}  // namespace logistic_regression

#endif  // MRML_LASSO_MR_ASSIGN_FEATURE_ID_H_
