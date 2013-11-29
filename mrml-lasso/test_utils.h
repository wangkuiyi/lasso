

//
// A set of utilities for testing basic components.  Many utilities
// are template accepting a vector type (either sparse or dense).
//
#ifndef MRML_LASSO_TEST_UTILS_H_
#define MRML_LASSO_TEST_UTILS_H_

#include "base/common.h"
#include "gtest/gtest.h"
#include "mrml-lasso/learner_states.h"
#include "mrml-lasso/logistic_regression.pb.h"
#include "mrml-lasso/vector_types.h"

namespace logistic_regression {

//---------------------------------------------------------------------------
// class template RealVectorTestUtil: for Sparse/Dense-RealVector
//---------------------------------------------------------------------------

template <class RealVector>
class RealVectorTestUtil {
 public:
  inline void Construct(RealVector* v);
  inline void Check(const RealVector& v);
  inline void CheckProtoBuf(const RealVectorPB& pb);
};

template <>
inline void RealVectorTestUtil<SparseRealVector>::
Construct(SparseRealVector* v) {
  v->set(1, 10);
  v->set(2, 0);
  v->set(3, 30);
}

template <>
inline void RealVectorTestUtil<DenseRealVector>::
Construct(DenseRealVector* v) {
  v->push_back(10);
  v->push_back(0);
  v->push_back(30);
}

template <>
inline void RealVectorTestUtil<SparseRealVector>::
Check(const SparseRealVector& v) {
  EXPECT_EQ(v.size(), 2);
  EXPECT_EQ(v[1], 10);
  EXPECT_EQ(v[2], 0);
  EXPECT_EQ(v[3], 30);
}

template <>
inline void RealVectorTestUtil<DenseRealVector>::
Check(const DenseRealVector& v) {
  EXPECT_EQ(v.size(), 3);
  EXPECT_EQ(v[0], 10);
  EXPECT_EQ(v[1], 0);
  EXPECT_EQ(v[2], 30);
}

template <>
inline void RealVectorTestUtil<SparseRealVector>::
CheckProtoBuf(const RealVectorPB& pb) {
  EXPECT_EQ(pb.element_size(), 2);
  EXPECT_EQ(pb.element(0).index(), 1);
  EXPECT_EQ(pb.element(0).value(), 10);
  EXPECT_EQ(pb.element(1).index(), 3);
  EXPECT_EQ(pb.element(1).value(), 30);
}

template <>
inline void RealVectorTestUtil<DenseRealVector>::
CheckProtoBuf(const RealVectorPB& pb) {
  EXPECT_EQ(pb.element_size(), 2);
  EXPECT_EQ(pb.element(0).index(), 0);
  EXPECT_EQ(pb.element(0).value(), 10);
  EXPECT_EQ(pb.element(1).index(), 2);
  EXPECT_EQ(pb.element(1).value(), 30);
}

//---------------------------------------------------------------------------
// class template RealVectorPtrDequeTestUtil
//---------------------------------------------------------------------------

template <class RealVector>
class RealVectorPtrDequeTestUtil {
 public:
  void Construct(RealVectorPtrDeque<RealVector>* q);
  void Check(const RealVectorPtrDeque<RealVector>& q);
};

template <class RealVector>
void RealVectorPtrDequeTestUtil<RealVector>::
Construct(RealVectorPtrDeque<RealVector>* q) {
  RealVectorTestUtil<RealVector> u;
  RealVector* v = new RealVector;
  u.Construct(v);
  q->resize(3, NULL);
  (*q)[1] = v;
}

template <class RealVector>
void RealVectorPtrDequeTestUtil<RealVector>::
Check(const RealVectorPtrDeque<RealVector>& q) {
  EXPECT_EQ(q.size(), 3);
  EXPECT_TRUE(q[0] == NULL);
  EXPECT_TRUE(q[2] == NULL);
  const RealVector* v = q[1];
  RealVectorTestUtil<RealVector> u;
  u.Check(*v);
}

//---------------------------------------------------------------------------
// class ImprovementFilterTestUtil
//---------------------------------------------------------------------------

class ImprovementFilterTestUtil {
 public:
  void Construct(ImprovementFilter* filter) {
    filter->value_history_.push_back(1111);
    filter->value_history_.push_back(2222);
  }

  void Check(const ImprovementFilter& filter) {
    EXPECT_EQ(1111, filter.value_history_[0]);
    EXPECT_EQ(2222, filter.value_history_[1]);
  }
};


//---------------------------------------------------------------------------
// class template LearnerStatesTestUtil, for Sparse/Dense-LearnerStates
//---------------------------------------------------------------------------

template <class RealVector>
class LearnerStatesTestUtil {
 public:
  inline void Construct(LearnerStates<RealVector>* states);
  inline void Check(const LearnerStates<RealVector>& states);
};


template <class RealVector>
inline void LearnerStatesTestUtil<RealVector>::
Construct(LearnerStates<RealVector>* states) {
  RealVectorTestUtil<RealVector> u;
  u.Construct(&states->x_);
  u.Construct(&states->new_x_);
  u.Construct(&states->grad_);
  u.Construct(&states->new_grad_);
  u.Construct(&states->dir_);

  RealVectorPtrDequeTestUtil<RealVector> ud;
  ud.Construct(&states->s_list_);
  ud.Construct(&states->y_list_);

  states->ro_list_.push_back(333);
  states->ro_list_.push_back(444);

  states->alphas_.push_back(555);
  states->alphas_.push_back(666);

  states->value_ = 777;
  states->iteration_ = 888;
  states->memory_size_ = 999;
  states->l1weight_ = 1000;

  ImprovementFilterTestUtil ui;
  ui.Construct(&(states->improvement_filter_));
}

template <class RealVector>
void LearnerStatesTestUtil<RealVector>::
Check(const LearnerStates<RealVector>& states) {
  RealVectorTestUtil<RealVector> u;
  u.Check(states.x_);
  u.Check(states.new_x_);
  u.Check(states.grad_);
  u.Check(states.new_grad_);
  u.Check(states.dir_);

  RealVectorPtrDequeTestUtil<RealVector> ud;
  ud.Check(states.s_list_);
  ud.Check(states.y_list_);

  EXPECT_EQ(2, states.ro_list_.size());
  EXPECT_EQ(333, states.ro_list_[0]);
  EXPECT_EQ(444, states.ro_list_[1]);

  EXPECT_EQ(2, states.alphas_.size());
  EXPECT_EQ(555, states.alphas_[0]);
  EXPECT_EQ(666, states.alphas_[1]);

  EXPECT_EQ(777, states.value_);
  EXPECT_EQ(888, states.iteration_);
  EXPECT_EQ(999, states.memory_size_);
  EXPECT_EQ(1000, states.l1weight_);

  ImprovementFilterTestUtil ui;
  ui.Check(states.improvement_filter_);
}

}  // namespace logistic_regression

#endif  // MRML_LASSO_TEST_UTILS_H_
