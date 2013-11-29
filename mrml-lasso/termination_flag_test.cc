

//
#include <stdlib.h>

#include <iostream>
#include <string>

#include "base/common.h"
#include "gtest/gtest.h"
#include "mrml-lasso/learner_states.h"
#include "mrml-lasso/termination_flag.h"
#include "mrml-lasso/vector_types.h"

using std::string;
using std::cout;

namespace logistic_regression {

template <class RealVector>
void testTerminationFlag() {
  typedef TerminationFlag<LearnerStates<RealVector> > TermFlag;

  static const char* kTempFilename = "/tmp/tmp_term_flag";

  cout << "Run " << __FUNCTION__ << "\n";

  // Remove the flag file to ensure that it does not exist.
  string cmd_remove_file = string("rm ") + kTempFilename;
  system(cmd_remove_file.c_str());

  // Create the flag file.
  EXPECT_EQ(true,
            TermFlag::SetLocally(kTempFilename,
                                 "Somewhat reasons.\n",
                                 NULL));

  // Check the existence of the flag file.
  string cmd_test_file = string("test -e ") + kTempFilename;
  int result = system(cmd_test_file.c_str());
  EXPECT_EQ(WEXITSTATUS(result), 0);

  // Remove the temp file
  system(cmd_remove_file.c_str());
}

}  // namespace logstic_regression

int main(int argc, char** argv) {
  using logistic_regression::testTerminationFlag;
  using logistic_regression::SparseRealVector;
  using logistic_regression::DenseRealVector;

  testTerminationFlag<DenseRealVector>();
  testTerminationFlag<SparseRealVector>();
  return 0;
}
