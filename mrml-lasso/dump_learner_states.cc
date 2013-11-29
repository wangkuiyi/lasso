

//
// A utility program that dump a learner states saved in a RecordIO
// file.
//
#include <iostream>    // NOLINT: TODO(yiwang): add C-style print facility
                       // for sparse vectors.

#include "boost/program_options/option.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"

#include "base/common.h"
#include "mrml/mrml_filesystem.h"
#include "mrml/mrml_recordio.h"

#include "mrml-lasso/logistic_regression.pb.h"
#include "mrml-lasso/learner_states.h"
#include "mrml-lasso/vector_types.h"

int main(int argc, char* argv[]) {
  using std::cout;
  using std::string;
  using logistic_regression::LearnerStates;
  using logistic_regression::SparseRealVector;
  using logistic_regression::DenseRealVector;
  namespace po = boost::program_options;

  po::options_description desc("Supported options");
  desc.add_options()
    ("learner_states_file", po::value<string>(), "the LearnerStatesPB file name")
    ("model_only", po::value<bool>(), "to dump model only or not");
  po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(desc).allow_unregistered().
      run();
  po::variables_map vm;
  po::store(parsed, vm);
  po::notify(vm);
  CHECK(vm.count("learner_states_file"));
  CHECK(vm.count("model_only"));

  MRMLFS_File file(vm["learner_states_file"].as<string>(), true);
  LearnerStates<DenseRealVector> states;
  states.LoadFromRecordFile(&file);
  if (vm["model_only"].as<bool>())
    cout << states.new_x();
  else
    cout << states;
  return 0;
}
