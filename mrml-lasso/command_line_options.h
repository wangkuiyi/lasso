

//
// Encapsulate command line options used by all the MR classes.
//
#ifndef MRML_LASSO_COMMAND_LINE_OPTIONS_H_
#define MRML_LASSO_COMMAND_LINE_OPTIONS_H_

#include <string>
#include <vector>

namespace logistic_regression {

class CommandLineOptions {
 public:
  std::string feature_id_file;
  std::string base_dir;
  std::string states_filebase;
  std::string flag_file;

  int memory_size;
  double l1weight;
  int max_line_search_steps;
  int max_iterations;
  double convergence_tolerance;
  int max_feature_number;  // only valid in "leanrer==dense" situation

  void Parse(const std::vector<std::string>& cmdline);
};

}  // namespace logistic_regression

#endif  // MRML_LASSO_COMMAND_LINE_OPTIONS_H_
