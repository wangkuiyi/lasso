


#include <boost/filesystem.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "base/common.h"
#include "mrml/mrml_filesystem.h"
#include "mrml/mrml_recordio.h"

#include "mrml-lasso/command_line_options.h"

namespace logistic_regression {

using std::string;

void CommandLineOptions::Parse(const std::vector<string>& cmdline) {
  namespace po = boost::program_options;
  po::options_description desc("LR-MPI initialization options");
  desc.add_options()
      ("feature_id_file",
       po::value<string>(&feature_id_file),
       "# feature to id mapping file")
      ("states_file_dir",
       po::value<string>(&base_dir),
       "# where the states files are located")
      ("states_filebase",
       po::value<string>(&states_filebase),
       "# states_filebase")
      ("flag_file",
       po::value<string>(&flag_file),
       "flag_file")
      ("memory_size",
       po::value<int>(&memory_size)->default_value(10),
       "# lr.memory_size")
      ("l1weight",
       po::value<double>(&l1weight)->default_value(1),
       "# lr.l1weight")
      ("max_line_search_steps",
       po::value<int>(&max_line_search_steps)->default_value(20),
       "# lr.max_line_search_steps")
      ("max_iterations",
       po::value<int>(&max_iterations)->default_value(120),
       "# lr.max_iterations")
      ("convergence_tolerance",
       po::value<double>(&convergence_tolerance)->default_value(1e-4),
       "# lr.convergence_tolerance")
      ("max_feature_number",
       po::value<int>(&max_feature_number)->default_value(0),
       "# features, required when learning a dense model");
  po::parsed_options parsed =
      po::command_line_parser(cmdline).options(desc).allow_unregistered().
      run();
  po::variables_map vm;
  po::store(parsed, vm);
  po::notify(vm);

  LOG(INFO) << "command line:"                                             \
            << "\tfeature_id_file:" << feature_id_file                     \
            << "\tstates_file_dir:" << base_dir                            \
            << "\tstates_filebase:" << states_filebase                     \
            << "\tflag_file:" << flag_file                                 \
            << "\tmemory_size:" << memory_size                             \
            << "\tl1weight:" << l1weight                                   \
            << "\tmax_line_search_steps:" << max_line_search_steps         \
            << "\tmax_iterations:" << max_iterations                       \
            << "\tconvergence_tolerance:" << convergence_tolerance         \
            << "\tmax_feature_number:" << max_feature_number;

  CHECK_LT(0, memory_size);
  CHECK_LE(0, l1weight);
  CHECK_LT(1, max_line_search_steps);
  CHECK_LT(1, max_iterations);
  CHECK_LT(0, convergence_tolerance);
}
}
