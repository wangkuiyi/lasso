

//
#include "gtest/gtest.h"

#include "base/common.h"
#include "base/cvector.h"

class DestructDetector {
 public:
  explicit DestructDetector(bool* flag) {
    flag_ = flag;
  }
  ~DestructDetector() {
    *flag_ = true;
  }
 private:
  bool* flag_;
};


TEST(CVectorTest, NumericalValueElements) {
  CVector<double> vd(3, 0.1);
  EXPECT_EQ(vd.size(), 3);
  for (int i = 0; i < vd.size(); ++i) {
    EXPECT_EQ(vd.data()[i], 0.1);
  }

  // Note: uncomment the following line to test
  // DISALLOW_COPY_AND_ASSIGN of CVector<Element>.
  // CVector<double> vcd = vd;

  vd.resize(4, 0.2);
  EXPECT_EQ(vd.size(), 4);
  for (int i = 0; i < vd.size(); ++i) {
    EXPECT_EQ(vd.data()[i], 0.2);
  }
}

TEST(CVectorTest, PointerValueElements) {
  CVector<DestructDetector*> vv(3);
  EXPECT_EQ(vv.size(), 3);
  for (int i = 0; i < vv.size(); ++i) {
    DestructDetector* d = vv.data()[i];
    EXPECT_EQ(static_cast<DestructDetector*>(NULL), d);
  }

  // Note: uncomment the following line to test
  // DISALLOW_COPY_AND_ASSIGN of CVector<Element*>.
  // CVector<DestructDetector*> vvcd = vv;

  bool destruct_flag = false;
  vv.data()[1] = new DestructDetector(&destruct_flag);

  vv.resize(0);
  EXPECT_EQ(vv.size(), 0);
  EXPECT_EQ(destruct_flag, true);
}
