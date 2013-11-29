


#include <math.h>
#include <stdlib.h>

#include <sstream>  // NOLINT. TODO(leostarzhou): Use SplitString in /strutil
                    // to substitute the use of istringstream.
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
#include "mrml-lasso/mr_convert_data_format.h"

using std::map;
using std::string;
using std::istringstream;
using std::ostringstream;
using std::stringstream;
using std::setw;
using std::setfill;

namespace logistic_regression {

REGISTER_MAPPER(ConvertDataFormatMapper);

ConvertDataFormatMapper::ConvertDataFormatMapper() {
  options_.Parse(GetConfig());
  feature_dict_.clear();
  MRMLFS_File file(options_.feature_id_file, true);
  string feature_name;
  string feature_id;
  while ( true == MRML_ReadRecord(&file, &feature_name, &feature_id) ) {
    feature_dict_[feature_name] = atoi(feature_id.c_str());
  }
}

void ConvertDataFormatMapper::Map(const std::string& key,
                                  const std::string& value) {
  float num_positive;
  float num_appearance;
  string feature_name;
  double feature_value;
  InstancePB instance;
  ostringstream oss;
  InstancePB pb;

  if (GetInputFormat() == Text) {
    istringstream line_parser(value);
    line_parser >> num_positive >> num_appearance;
    pb.set_num_positive(num_positive);
    pb.set_num_appearance(num_appearance);
    while (line_parser >> feature_name >> feature_value) {
      if (!feature_dict_.count(feature_name)) {
        LOG(ERROR) << "could not find feature[" << feature_name << "]'s id.";
        return;
      }
      InstancePB::Feature* e = pb.add_feature();
      e->set_name(feature_name);
      e->set_id(feature_dict_[feature_name]);
      e->set_value(feature_value);
    }
    string output_buffer;
    pb.SerializeToString(&output_buffer);
    Output(kUniqueKey, output_buffer);
  } else if (GetInputFormat() == RecordIO) {
    CHECK(instance.ParseFromString(value));
    pb.set_num_positive(instance.num_positive());
    pb.set_num_appearance(instance.num_appearance());
    for (int i = 0; i < instance.feature_size(); ++i) {
      feature_name = instance.feature(i).name();
      if (!feature_dict_.count(feature_name)) {
        LOG(ERROR) << "could not find feature[" << feature_name << "]'s id.";
        return;
      }
      InstancePB::Feature* e = pb.add_feature();
      e->set_name(feature_name);
      e->set_id(feature_dict_[feature_name]);
      e->set_value(instance.feature(i).value());
    }
    string output_buffer;
    pb.SerializeToString(&output_buffer);
    Output(kUniqueKey, output_buffer);
  }
}

}  // namespace logistic_regression
