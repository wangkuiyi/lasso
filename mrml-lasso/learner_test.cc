

//
// This is a highly specialized side-by-side test of the dense
// vector-based implementation of the trianing algorithm from Jianfeng
// Gao and our sparse vector-based implementation.
//
#include <vector>

#include "base/common.h"
#include "gtest/gtest.h"
#include "mrml-lasso/learner.h"
#include "mrml-lasso/learner_sparse_impl.h"
#include "mrml-lasso/test_utils.h"
#include "mrml-lasso/vector_types.h"

namespace logistic_regression {

//---------------------------------------------------------------------------
// Define the class template LearnerTestUtil.
//---------------------------------------------------------------------------
template <class RealVector>
class LearnerTestUtil : public LearnerStatesTestUtil<RealVector> {
 public:
  void TestMakeSteepestDescDir_XLongerThanGrad();
  void TestMakeSteepestDescDir_GradLongerThanX();
  void TestDirDeriv_XLongerThanDir();
  void TestDirDeriv_DirLongerThanX();
  void TestGetNextPoint();
  void TestFixDirSigns();

 private:
  typedef std::vector<double> DblVec;

  int dim;
  DblVec x, new_x;
  DblVec dir;
  DblVec grad, new_grad;
  double l1weight;

  static double dotProduct(const DblVec& a, const DblVec& b);
  static void scaleInto(DblVec& a, const DblVec& b, double c);
  static void addMultInto(DblVec& a, const DblVec& b, const DblVec& c,
                          double d);

  void MakeSteepestDescDir();
  double DirDeriv() const;
  void GetNextPoint(double alpha);
  void FixDirSigns();
};

//---------------------------------------------------------------------------
// Specialization of LearnerTestUtil<SparseRealVector>.
//---------------------------------------------------------------------------

template <>
double LearnerTestUtil<SparseRealVector>::dotProduct(const DblVec& a,
                                                     const DblVec& b) {
  double result = 0;
  for (size_t i = 0; i < a.size(); i++) {
    result += a[i] * b[i];
  }
  return result;
}

template <>
void LearnerTestUtil<SparseRealVector>::scaleInto(DblVec& a, const DblVec& b,
                                                  double c) {
  for (size_t i = 0; i < a.size(); i++) {
    a[i] = b[i] * c;
  }
}

template <>
void LearnerTestUtil<SparseRealVector>::addMultInto(DblVec& a,
                                                    const DblVec& b,
                                                    const DblVec& c,
                                                    double d) {
  for (size_t i = 0; i < a.size(); i++) {
    a[i] = b[i] + c[i] * d;
  }
}

/* Read-only: steepestDescDir (newGrad) */
/* Write: dir */
template <>
void LearnerTestUtil<SparseRealVector>::FixDirSigns() {
  if (l1weight > 0) {
    for (size_t i = 0; i < dim; i++) {
      if (dir[i] * new_grad[i] <= 0) {
        dir[i] = 0;
      }
    }
  }
}

/* Read-only: x, dir */
/* Write: new_x */
template <>
void LearnerTestUtil<SparseRealVector>::GetNextPoint(double alpha) {
  addMultInto(new_x, x, dir, alpha);
  if (l1weight > 0) {
    for (size_t i = 0; i < dim; i++) {
      if (x[i] * new_x[i] < 0.0) {
        new_x[i] = 0.0;          // Here sets a weight to 0.
      }
    }
  }
}

/* Read-only: x, grad, l1weight, dim */
/* Write: dir, new_grad */
template <>
void LearnerTestUtil<SparseRealVector>::MakeSteepestDescDir() {
  if (l1weight == 0) {
    scaleInto(dir, grad, -1);
  } else {
    for (size_t i = 0; i < dim; i++) {
      if (x[i] < 0) {
        dir[i] = -grad[i] + l1weight;
      } else if (x[i] > 0) {
        dir[i] = -grad[i] - l1weight;
      } else {
        if (grad[i] < -l1weight) {
          dir[i] = -grad[i] - l1weight;
        } else if (grad[i] > l1weight) {
          dir[i] = -grad[i] + l1weight;
        } else {
          dir[i] = 0;
        }
      }
    }
  }
  new_grad = dir;
}

/* Read-only: x, dir, grad */
template <>
double LearnerTestUtil<SparseRealVector>::DirDeriv() const {
  if (l1weight == 0) {
    return dotProduct(dir, grad);
  } else {
    double val = 0.0;
    for (size_t i = 0; i < dim; i++) {
      if (dir[i] != 0) {
        if (x[i] < 0) {
          val += dir[i] * (grad[i] - l1weight);
        } else if (x[i] > 0) {
          val += dir[i] * (grad[i] + l1weight);
        } else if (dir[i] < 0) {
          val += dir[i] * (grad[i] - l1weight);
        } else if (dir[i] > 0) {
          val += dir[i] * (grad[i] + l1weight);
        }
      }
    }
    return val;
  }
}

template <>
void LearnerTestUtil<SparseRealVector>::TestDirDeriv_XLongerThanDir() {
  Learner<SparseRealVector> lner;

  lner.l1weight_ = 2;  this->l1weight = 2;
  lner.x_.clear();     lner.grad_.clear();   lner.dir_.clear();
  this->x.clear();     this->grad.clear();   this->dir.clear();

  lner.x_.set(0, 0);   lner.grad_.set(0, 0);   lner.dir_.set(0, 0);
  this->x.push_back(0);  this->grad.push_back(0);  this->dir.push_back(0);
  lner.x_.set(1, 1);   lner.grad_.set(1, 3);   lner.dir_.set(1, 3);
  this->x.push_back(1);  this->grad.push_back(3);  this->dir.push_back(3);
  lner.x_.set(2, 1);   lner.grad_.set(2, -3);  lner.dir_.set(2, -3);
  this->x.push_back(1);  this->grad.push_back(-3); this->dir.push_back(-3);
  lner.x_.set(3, -1);  lner.grad_.set(3, 3);   lner.dir_.set(3, 3);
  this->x.push_back(-1); this->grad.push_back(3);  this->dir.push_back(3);
  lner.x_.set(4, -1);  lner.grad_.set(4, -3);  lner.dir_.set(4, -3);
  this->x.push_back(-1); this->grad.push_back(-3); this->dir.push_back(-3);
  lner.x_.set(5, 0);   lner.grad_.set(5, 3);   lner.dir_.set(5, 3);
  this->x.push_back(0);  this->grad.push_back(3);  this->dir.push_back(3);
  lner.x_.set(6, 0);   lner.grad_.set(6, -3);  lner.dir_.set(6, -3);
  this->x.push_back(0);  this->grad.push_back(-3); this->dir.push_back(-3);
  lner.x_.set(7, 1);   lner.grad_.set(7, 0);   lner.dir_.set(7, 0);
  this->x.push_back(1);  this->grad.push_back(0);  this->dir.push_back(0);
  lner.x_.set(8, -1);  lner.grad_.set(8, 0);   lner.dir_.set(8, 0);
  this->x.push_back(-1); this->grad.push_back(0);  this->dir.push_back(0);

  this->dim = this->x.size();
  EXPECT_EQ(this->x.size(), this->grad.size());
  EXPECT_EQ(this->x.size(), this->dir.size());
  EXPECT_EQ(this->dim, 9);

  this->dir.resize(this->dim, 0);

  double g_result = this->DirDeriv();
  double s_result = lner.DirDeriv();
  EXPECT_EQ(g_result, s_result);
}

template <>
void LearnerTestUtil<SparseRealVector>::TestDirDeriv_DirLongerThanX() {
  Learner<SparseRealVector> lner;

  lner.l1weight_ = 2;  this->l1weight = 2;
  lner.x_.clear();     lner.grad_.clear();   lner.dir_.clear();
  this->x.clear();     this->grad.clear();   this->dir.clear();

  lner.x_.set(0, 0);   lner.grad_.set(0, 0);   lner.dir_.set(0, 0);
  this->x.push_back(0);  this->grad.push_back(0);  this->dir.push_back(0);
  lner.x_.set(1, 1);   lner.grad_.set(1, 3);   lner.dir_.set(1, 3);
  this->x.push_back(1);  this->grad.push_back(3);  this->dir.push_back(3);
  lner.x_.set(2, 1);   lner.grad_.set(2, -3);  lner.dir_.set(2, -3);
  this->x.push_back(1);  this->grad.push_back(-3); this->dir.push_back(-3);
  lner.x_.set(3, -1);  lner.grad_.set(3, 3);   lner.dir_.set(3, 3);
  this->x.push_back(-1); this->grad.push_back(3);  this->dir.push_back(3);
  lner.x_.set(4, -1);  lner.grad_.set(4, -3);  lner.dir_.set(4, -3);
  this->x.push_back(-1); this->grad.push_back(-3); this->dir.push_back(-3);
  lner.x_.set(5, 1);   lner.grad_.set(5, 0);   lner.dir_.set(5, 0);
  this->x.push_back(1);  this->grad.push_back(0);  this->dir.push_back(0);
  lner.x_.set(6, -1);  lner.grad_.set(6, 0);   lner.dir_.set(6, 0);
  this->x.push_back(-1); this->grad.push_back(0);  this->dir.push_back(0);
  lner.x_.set(7, 0);   lner.grad_.set(7, 3);   lner.dir_.set(7, 3);
  this->x.push_back(0);  this->grad.push_back(3);  this->dir.push_back(3);
  lner.x_.set(8, 0);   lner.grad_.set(8, -3);  lner.dir_.set(8, -3);
  this->x.push_back(0);  this->grad.push_back(-3); this->dir.push_back(-3);

  this->dim = this->x.size();
  EXPECT_EQ(this->x.size(), this->grad.size());
  EXPECT_EQ(this->x.size(), this->dir.size());
  EXPECT_EQ(this->dim, 9);

  this->dir.resize(this->dim, 0);

  double g_result = this->DirDeriv();
  double s_result = lner.DirDeriv();
  EXPECT_EQ(g_result, s_result);
}

template <>
void LearnerTestUtil<SparseRealVector>::
TestMakeSteepestDescDir_XLongerThanGrad() {
  Learner<SparseRealVector> lner;

  lner.l1weight_ = 2;    this->l1weight = 2;
  lner.x_.clear();       lner.grad_.clear();
  this->x.clear();       this->grad.clear();

  lner.x_.set(0, 0);   lner.grad_.set(0, 0);
  this->x.push_back(0);  this->grad.push_back(0);
  lner.x_.set(1, 1);   lner.grad_.set(1, 3);
  this->x.push_back(1);  this->grad.push_back(3);
  lner.x_.set(2, 1);   lner.grad_.set(2, -3);
  this->x.push_back(1);  this->grad.push_back(-3);
  lner.x_.set(3, -1);  lner.grad_.set(3, 3);
  this->x.push_back(-1); this->grad.push_back(3);
  lner.x_.set(4, -1);  lner.grad_.set(4, -3);
  this->x.push_back(-1); this->grad.push_back(-3);
  lner.x_.set(5, 0);   lner.grad_.set(5, 3);
  this->x.push_back(0);  this->grad.push_back(3);
  lner.x_.set(6, 0);   lner.grad_.set(6, -3);
  this->x.push_back(0);  this->grad.push_back(-3);
  lner.x_.set(7, 1);   lner.grad_.set(7, 0);
  this->x.push_back(1);  this->grad.push_back(0);
  lner.x_.set(8, -1);  lner.grad_.set(8, 0);
  this->x.push_back(-1); this->grad.push_back(0);

  this->dim = this->x.size();
  EXPECT_EQ(this->x.size(), this->grad.size());
  EXPECT_EQ(this->dim, 9);

  this->dir.resize(this->dim, 0);
  this->new_grad.resize(this->dim, 0);

  this->MakeSteepestDescDir();
  lner.MakeSteepestDescDir();

  for (int index = 0; index < this->dim; ++index) {
    if (this->dir[index] != lner.dir_[index]) {
      LOG(ERROR) << " index == " << index << "\n"
                 << " this->dir[i] == " << this->dir[index] << "\n"
                 << " lner.dir_[index] == " << lner.dir_[index] << "\n";
    }
    if (this->new_grad[index] != lner.new_grad_[index]) {
      LOG(ERROR) << " index == " << index << "\n"
                 << " this->new_grad[i] == " << this->new_grad[index] << "\n"
                 << " lner.new_grad_[index] == " << lner.new_grad_[index];
    }
  }
}

template <>
void LearnerTestUtil<SparseRealVector>::
TestMakeSteepestDescDir_GradLongerThanX() {
  Learner<SparseRealVector> lner;

  lner.l1weight_ = 2;  this->l1weight = 2;
  lner.x_.clear();     lner.grad_.clear();
  this->x.clear();        this->grad.clear();

  lner.x_.set(0, 0);   lner.grad_.set(0, 0);
  this->x.push_back(0);  this->grad.push_back(0);
  lner.x_.set(1, 1);   lner.grad_.set(1, 3);
  this->x.push_back(1);  this->grad.push_back(3);
  lner.x_.set(2, 1);   lner.grad_.set(2, -3);
  this->x.push_back(1);  this->grad.push_back(-3);
  lner.x_.set(3, -1);  lner.grad_.set(3, 3);
  this->x.push_back(-1); this->grad.push_back(3);
  lner.x_.set(4, -1);  lner.grad_.set(4, -3);
  this->x.push_back(-1); this->grad.push_back(-3);
  lner.x_.set(5, 1);   lner.grad_.set(5, 0);
  this->x.push_back(1);  this->grad.push_back(0);
  lner.x_.set(6, -1);  lner.grad_.set(6, 0);
  this->x.push_back(-1); this->grad.push_back(0);
  lner.x_.set(7, 0);   lner.grad_.set(7, 3);
  this->x.push_back(0);  this->grad.push_back(3);
  lner.x_.set(8, 0);   lner.grad_.set(8, -3);
  this->x.push_back(0);  this->grad.push_back(-3);

  this->dim = this->x.size();
  EXPECT_EQ(this->x.size(), this->grad.size());
  EXPECT_EQ(this->dim, 9);

  this->dir.resize(this->dim, 0);
  this->new_grad.resize(this->dim, 0);

  this->MakeSteepestDescDir();
  lner.MakeSteepestDescDir();

  for (int index = 0; index < this->dim; ++index) {
    if (this->dir[index] != lner.dir_[index]) {
      LOG(ERROR) << "index :" << index << "\n"
                 << "dir[index] :" << this->dir[index] << "\n"
                 << "lner.dir_[index] :" << lner.dir_[index];
    }
    if (this->new_grad[index] != lner.new_grad_[index]) {
      LOG(ERROR) << "index :" << index << "\n"
                 << "new_grad[index] :" << this->new_grad[index] << "\n"
                 << "lner.new_grad_[index] :" << lner.new_grad_[index];
    }
  }
}

template <>
void LearnerTestUtil<SparseRealVector>::TestGetNextPoint() {
  Learner<SparseRealVector> lner;

  this->l1weight = 2;    lner.l1weight_ = 2;

  lner.x_.clear();       lner.dir_.clear();
  this->x.clear();       this->dir.clear();

  lner.x_.set(0, 0);   lner.dir_.set(0, 0);
  this->x.push_back(0);  this->dir.push_back(0);
  lner.x_.set(1, 1);   lner.dir_.set(1, 3);
  this->x.push_back(1);  this->dir.push_back(3);
  lner.x_.set(2, 1);   lner.dir_.set(2, -3);
  this->x.push_back(1);  this->dir.push_back(-3);
  lner.x_.set(3, -1);  lner.dir_.set(3, 3);
  this->x.push_back(-1); this->dir.push_back(3);
  lner.x_.set(4, -1);  lner.dir_.set(4, -3);
  this->x.push_back(-1); this->dir.push_back(-3);
  lner.x_.set(5, 1);   lner.dir_.set(5, 0);
  this->x.push_back(1);  this->dir.push_back(0);
  lner.x_.set(6, -1);  lner.dir_.set(6, 0);
  this->x.push_back(-1); this->dir.push_back(0);
  lner.x_.set(7, 0);   lner.dir_.set(7, 3);
  this->x.push_back(0);  this->dir.push_back(3);
  lner.x_.set(8, 0);   lner.dir_.set(8, -3);
  this->x.push_back(0);  this->dir.push_back(-3);

  this->dim = this->x.size();
  EXPECT_EQ(this->x.size(), this->dir.size());
  EXPECT_EQ(this->dim, 9);
  this->new_x.resize(this->dim, 0);

  this->GetNextPoint(0.5);
  lner.GetNextPoint(0.5);

  for (int index = 0; index < this->dim; ++index) {
    if (this->new_x[index] != lner.new_x_[index]) {
      LOG(ERROR) << " index :" << index << "\n"
                 << " this->new_x[index] :" << this->new_x[index] << "\n"
                 << " lner.new_x_[index] :" << lner.new_x_[index] << "\n";
    }
  }
}

template <>
void LearnerTestUtil<SparseRealVector>::TestFixDirSigns() {
  Learner<SparseRealVector> lner;

  this->l1weight = 2;    lner.l1weight_ = 2;

  lner.new_grad_.clear();       lner.dir_.clear();
  this->new_grad.clear();       this->dir.clear();

  lner.new_grad_.set(0, 0);   lner.dir_.set(0, 0);
  this->new_grad.push_back(0);  this->dir.push_back(0);
  lner.new_grad_.set(1, 1);   lner.dir_.set(1, 3);
  this->new_grad.push_back(1);  this->dir.push_back(3);
  lner.new_grad_.set(2, 1);   lner.dir_.set(2, -3);
  this->new_grad.push_back(1);  this->dir.push_back(-3);
  lner.new_grad_.set(3, -1);  lner.dir_.set(3, 3);
  this->new_grad.push_back(-1); this->dir.push_back(3);
  lner.new_grad_.set(4, -1);  lner.dir_.set(4, -3);
  this->new_grad.push_back(-1); this->dir.push_back(-3);
  lner.new_grad_.set(5, 1);   lner.dir_.set(5, 0);
  this->new_grad.push_back(1);  this->dir.push_back(0);
  lner.new_grad_.set(6, -1);  lner.dir_.set(6, 0);
  this->new_grad.push_back(-1); this->dir.push_back(0);
  lner.new_grad_.set(7, 0);   lner.dir_.set(7, 3);
  this->new_grad.push_back(0);  this->dir.push_back(3);
  lner.new_grad_.set(8, 0);   lner.dir_.set(8, -3);
  this->new_grad.push_back(0);  this->dir.push_back(-3);

  this->dim = this->new_grad.size();
  EXPECT_EQ(this->new_grad.size(), this->dir.size());
  EXPECT_EQ(this->dim, 9);

  this->FixDirSigns();
  lner.FixDirSigns();

  for (int index = 0; index < this->dim; ++index) {
    if (this->dir[index] != lner.dir_[index]) {
      LOG(ERROR) << " index :" << index << "\n"
                 << " this->dir[index] :" << this->dir[index] << "\n"
                 << " lner.dir_[index] :" << lner.dir_[index] << "\n";
    }
  }
}

}  // namespace logistic_regression

using logistic_regression::LearnerTestUtil;
using logistic_regression::SparseRealVector;

class SparseLearnerTest : public ::testing::Test {
 protected:
  LearnerTestUtil<SparseRealVector> test_util;
};

TEST_F(SparseLearnerTest, TestMakeSteepestDescDir_XLongerThanGrad) {
  test_util.TestMakeSteepestDescDir_XLongerThanGrad();
}

TEST_F(SparseLearnerTest, TestMakeSteepestDescDir_GradLongerThanX) {
  test_util.TestMakeSteepestDescDir_GradLongerThanX();
}

TEST_F(SparseLearnerTest, TestDirDeriv_XLongerThanDir) {
  test_util.TestDirDeriv_XLongerThanDir();
}

TEST_F(SparseLearnerTest, TestDirDeriv_DirLongerThanX) {
    test_util.TestDirDeriv_DirLongerThanX();
}

TEST_F(SparseLearnerTest, TestGetNextPoint) {
  test_util.TestGetNextPoint();
}

TEST_F(SparseLearnerTest, TestFixDirSigns) {
  test_util.TestFixDirSigns();
}
