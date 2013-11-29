

//
// Define the mappers only to convert features to id in traing data.
// Mappers input: the training data. (data format: text or recordio)
// Mappers output: the new training data in which features has been
// converted from string to id. (data format: recordio)
// External input: the feature_id mapping file. (data format: recordio)

#ifndef MRML_LASSO_MR_CONVERT_DATA_FORMAT_H_
#define MRML_LASSO_MR_CONVERT_DATA_FORMAT_H_

#include <map>
#include <string>

#include "base/common.h"
#include "strutil/split_string.h"
#include "mrml/mrml.h"
#include "mrml-lasso/command_line_options.h"

namespace logistic_regression {

extern const char* kUniqueKey;

class ConvertDataFormatMapper : public MRML_Mapper {
 public:
  ConvertDataFormatMapper();
  void Map(const std::string& key, const std::string& value);
  void Flush() {}

 private:
  std::map<std::string, int> feature_dict_;
  CommandLineOptions options_;
};

}  // namespace logistic_regression

#endif  // MRML_LASSO_MR_CONVERT_DATA_FORMAT_H_
