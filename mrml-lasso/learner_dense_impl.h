

//
// Some member functions of class template Learner depends on the
// vector type (dense or sparse).  Here we specialize these functions
// with DenseRealVector.
//
#ifndef MRML_LASSO_LEARNER_DENSE_IMPL_H_
#define MRML_LASSO_LEARNER_DENSE_IMPL_H_

#include <math.h>
#include <stdlib.h>

#include "base/common.h"
#include "mrml-lasso/learner.h"
#include "mrml-lasso/termination_flag.h"
#include "mrml-lasso/vector_types.h"

namespace logistic_regression {

template <>
Learner<DenseRealVector>::Learner() : LearnerStates<DenseRealVector>() {}

template <>
Learner<DenseRealVector>::Learner(const DenseRealVector& initial_x,
                                  int memory_size,
                                  double l1_weight,
                                  int max_line_search_steps,
                                  int max_iterations,
                                  double convergence_tolerance,
                                  int max_feature_number)
    : LearnerStates<DenseRealVector>(initial_x,
                                     memory_size,
                                     l1_weight,
                                     max_line_search_steps,
                                     max_iterations,
                                     convergence_tolerance) {
  if (max_feature_number > 0) {
    x_.resize(max_feature_number);
    new_x_.resize(max_feature_number);
    grad_.resize(max_feature_number);
    new_grad_.resize(max_feature_number);
    dir_.resize(max_feature_number);
  }
}

template <>
void Learner<DenseRealVector>::MakeSteepestDescDir() {
  PRINT_EXECUTION_TRACE;

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "x = " << x_ << "\n"
            << "dir = " << dir_ << "\n"
            << "grad = " << grad_ << "\n"
            << "l1weight = " << l1weight_ << "\n";
#endif  // DEBUG_PRINT_VARS

  if (l1weight_ == 0) {
    ScaleInto(&dir_, grad_, -1);
  } else {
    CHECK_EQ(x_.size(), dir_.size());
    CHECK_EQ(grad_.size(), dir_.size());
    size_t dim = dir_.size();
    for (size_t i = 0; i < dim; ++i) {
      if (x_[i] < 0) {
        dir_[i] = -grad_[i] + l1weight_;
      } else if (x_[i] > 0) {
        dir_[i] = -grad_[i] - l1weight_;
      } else {
        if (grad_[i] < -l1weight_) {
          dir_[i] = -grad_[i] - l1weight_;
        } else if (grad_[i] > l1weight_) {
          dir_[i] = -grad_[i] + l1weight_;
        } else {
          dir_[i] = 0;
        }
      }
    }
  }

  new_grad_ = dir_;

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "dir = " << dir_ << "\n"
            << "new_grad = " << new_grad_ << "\n";
#endif  // DEBUG_PRINT_VARS
}


template <>
void Learner<DenseRealVector>::MapDirByInverseHessian() {
  PRINT_EXECUTION_TRACE;

  size_t count = s_list_.size();

  if (count != 0) {
    for (int i = count - 1; i >= 0; --i) {
      alphas_[i] = - DotProduct(*s_list_[i], dir_) / ro_list_[i];
      AddScaled(&dir_, *y_list_[i], alphas_[i]);
    }

    const DenseRealVector& last_y_ = *y_list_[count - 1];
    double y_dot_y = DotProduct(last_y_, last_y_);
    double scalar = ro_list_[count - 1] / y_dot_y;
    Scale(&dir_, scalar);

    for (uint32 i = 0; i < count; i++) {
      double beta = DotProduct(*y_list_[i], dir_) / ro_list_[i];
      AddScaled(&dir_, *s_list_[i], -alphas_[i] - beta);
    }
  }

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "dir = " << dir_ << "\n";
#endif  // DEBUG_PRINT_VARS
}


template <>
void Learner<DenseRealVector>::FixDirSigns() {
  PRINT_EXECUTION_TRACE;

  if (l1weight_ > 0) {
    for (size_t i = 0; i < dir_.size(); ++i) {
      if (dir_[i] * new_grad_[i] <= 0) {
        dir_[i] = 0;
      }
    }
  }

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "dir = " << dir_ << "\n";
#endif  // DEBUG_PRINT_VARS

#ifdef DEBUG_PRINT_PROGRESS
  std::cout << __FUNCTION__ << "\n";
#endif  // DEBUG_PRINT_PROGRESS
}


template <>
double Learner<DenseRealVector>::DirDeriv() const {
  PRINT_EXECUTION_TRACE;

  double ret = 0;

  if (l1weight_ == 0) {
    ret = DotProduct(dir_, grad_);
  } else {
    for (size_t i = 0; i < dir_.size(); ++i) {
      if (dir_[i] != 0) {
        if (x_[i] < 0) {
          ret += dir_[i] * (grad_[i] - l1weight_);
        } else if (x_[i] > 0) {
          ret += dir_[i] * (grad_[i] + l1weight_);
        } else if (dir_[i] < 0) {
          ret += dir_[i] * (grad_[i] - l1weight_);
        } else if (dir_[i] > 0) {
          ret += dir_[i] * (grad_[i] + l1weight_);
        }
      }
    }
  }

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "DirDeriv ret = " << ret << "\n";
#endif  // DEBUG_PRINT_VARS

  return ret;
}


template <>
void Learner<DenseRealVector>::GetNextPoint(double alpha) {
  PRINT_EXECUTION_TRACE;

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "new_x = " << new_x_ << "\n"
            << "x = " << x_ << "\n"
            << "dir = " << dir_ << "\n"
            << "alpha = " << alpha << "\n";
#endif  // DEBUG_PRINT_VARS
  AddScaledInto(&new_x_, x_, dir_, alpha);
#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "new_x = " << new_x_ << "\n"
            << "x = " << x_ << "\n";
#endif  // DEBUG_PRINT_VARS
  if (l1weight_ > 0) {
    for (size_t i = 0; i < x_.size(); i++) {
      if (x_[i] * new_x_[i] < 0.0) {
        new_x_[i] = 0.0;
      }
    }
  }

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "new_x = " << new_x_ << "\n"
            << "x = " << x_ << "\n";
#endif  // DEBUG_PRINT_VARS
}

}  // namespace logistic_regression

#endif  // MRML_LASSO_LEARNER_DENSE_IMPL_H_
