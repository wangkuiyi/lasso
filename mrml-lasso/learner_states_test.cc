

//
#include <sstream>

#include "base/common.h"
#include "gtest/gtest.h"
#include "mrml/mrml.h"
#include "mrml/mrml_filesystem.h"
#include "mrml/mrml_recordio.h"

#include "mrml-lasso/learner_states.h"
#include "mrml-lasso/test_utils.h"

namespace logistic_regression {

using std::istringstream;
using std::ostringstream;

template <class RealVector>
void testLearnerStatesSerialization() {
  std::cout << "Run " << __FUNCTION__ << "\n";

  LearnerStatesTestUtil<RealVector> u;
  static const char* kTempFile = "/tmp/testLearnerStatesSerialization";
  {
    LearnerStates<RealVector> states;
    u.Construct(&states);
    MRMLFS_File out(kTempFile, false);
    CHECK(out.IsOpen());
    states.SaveIntoRecordFile(&out);
  }
  {
    LearnerStates<RealVector> states;
    MRMLFS_File in(kTempFile, true);
    states.LoadFromRecordFile(&in);
    u.Check(states);
  }
}

}  // namespace logsitic_regression

TEST(LearnerStatesSerializationTest, DenseRealVector) {
  using logistic_regression::DenseRealVector;
  logistic_regression::testLearnerStatesSerialization<DenseRealVector>();
}

TEST(LearnerStatesSerializationTest, SparseRealVector) {
  using logistic_regression::SparseRealVector;
  logistic_regression::testLearnerStatesSerialization<SparseRealVector>();
}
