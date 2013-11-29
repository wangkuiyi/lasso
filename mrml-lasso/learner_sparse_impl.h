

//
// Define the class template DenseVectorImpl and operations required
// by the OWLQN algorithm.
//
#ifndef MRML_LASSO_LEARNER_SPARSE_IMPL_H_
#define MRML_LASSO_LEARNER_SPARSE_IMPL_H_

#include <math.h>

#include "base/common.h"

#include "mrml-lasso/learner.h"
#include "mrml-lasso/termination_flag.h"
#include "mrml-lasso/vector_types.h"

namespace logistic_regression {

template <>
Learner<SparseRealVector>::Learner() : LearnerStates<SparseRealVector>() {}


template <>
void Learner<SparseRealVector>::MakeSteepestDescDir() {
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
    dir_.clear();

    SparseRealVector::const_iterator ix = x_.begin();
    SparseRealVector::const_iterator ig = grad_.begin();

    while (ix != x_.end() && ig != grad_.end()) {
      if (ix->first <= ig->first) {
        // x[i] != 0, grad[i] may or may not be 0.
        const IndexType& index = ix->first;
        if (ix->second < 0) {
          dir_.set(index, - grad_[index] + l1weight_);
        } else if (ix->second > 0) {
          dir_.set(index, - grad_[index] - l1weight_);
        }
        if (ig->first == ix->first)
          ++ig;
        ++ix;
      } else if (ig->first < ix->first) {
        // x[i] == 0 && grad_[i] != 0
        const IndexType& index = ig->first;
        if (ig->second < - l1weight_) {
          dir_.set(index, - ig->second - l1weight_);
        } else if (ig->second > l1weight_) {
          dir_.set(index, - ig->second + l1weight_);
        }
        ++ig;
      }
      // else if (x[i]==0 && grad[i]==0), no change to dir[i] means
      // to keep it zero.
    }

    while (ix != x_.end()) {
      // x[i] != 0 && grad[i] == 0
      const IndexType& index = ix->first;
      if (ix->second < 0) {
        dir_.set(index, l1weight_);
      } else if (ix->second > 0) {
        dir_.set(index, - l1weight_);
      }
      ++ix;
    }

    while (ig != grad_.end()) {
      // x[i] == 0 && grad[i] != 0
      const IndexType& index = ig->first;
      if (ig->second < - l1weight_) {
        dir_.set(index, - ig->second - l1weight_);
      } else if (ig->second > l1weight_) {
        dir_.set(index, - ig->second + l1weight_);
      }
      ++ig;
    }
  }

  // Set steepest descent dir to new_grad_.
  new_grad_ = dir_;

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "dir = " << dir_ << "\n"
            << "new_grad = " << new_grad_ << "\n";
#endif  // DEBUG_PRINT_VARS
}


template <>
void Learner<SparseRealVector>::MapDirByInverseHessian() {
  PRINT_EXECUTION_TRACE;

  int count = static_cast<int>(s_list_.size());

  if (count != 0) {
    for (int i = count - 1; i >= 0; i--) {
      alphas_[i] = - DotProduct(*s_list_[i], dir_) / ro_list_[i];
      AddScaled(&dir_, *y_list_[i], alphas_[i]);
    }

    const SparseRealVector& last_y = *y_list_[count - 1];
    double y_dot_y = DotProduct(last_y, last_y);
    double scalar = ro_list_[count - 1] / y_dot_y;
    Scale(&dir_, scalar);

    for (int i = 0; i < count; i++) {
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
void Learner<SparseRealVector>::FixDirSigns() {
  PRINT_EXECUTION_TRACE;

  if (l1weight_ > 0) {
    SparseRealVector::iterator i = dir_.begin();
    SparseRealVector::const_iterator j = new_grad_.begin();

    while (i != dir_.end() && j != new_grad_.end()) {
      if (i->first == j->first) {
        if (i->second * j->second <= 0) {
          SparseRealVector::iterator t(i);
          ++i;
          ++j;
          dir_.erase(t);
        } else {
          ++i;
          ++j;
        }
      } else if (i->first < j->first) {
        SparseRealVector::iterator t(i);
        ++i;
        dir_.erase(t);
      } else if (i->first > j->first) {
        ++j;
      }
    }

    while (i != dir_.end()) {
      SparseRealVector::iterator t(i);
      ++i;
      dir_.erase(t);
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
double Learner<SparseRealVector>::DirDeriv() const {
  PRINT_EXECUTION_TRACE;

  double ret = 0;

  if (l1weight_ == 0) {
    ret = DotProduct(dir_, grad_);
  } else {
    SparseRealVector::const_iterator i_dir = dir_.begin();
    SparseRealVector::const_iterator i_x = x_.begin();

    while (i_dir != dir_.end() && i_x != x_.end()) {
      const IndexType& index = i_dir->first;
      if (i_dir->first == i_x->first) {
        // dir[i] != 0 && x[i] != 0
        if (i_x->second < 0) {
          ret += i_dir->second * (grad_[index] - l1weight_);
        } else if (i_x->second > 0) {
          ret += i_dir->second * (grad_[index] + l1weight_);
        }
        ++i_dir;
        ++i_x;
      } else if (i_dir->first < i_x->first) {
        // dir[i] != 0 && x[i] == 0
        if (i_dir->second < 0) {
          ret += i_dir->second *(grad_[index] - l1weight_);
        } else if (i_dir->second > 0) {
          ret += i_dir->second *(grad_[index] + l1weight_);
        }
        ++i_dir;
      } else {
        // dir[i] == 0 && x[i] != 0
        ++i_x;
      }
    }

    while (i_dir != dir_.end()) {
      // dir[i] != 0 && x[i] == 0
      const IndexType& index = i_dir->first;
      if (i_dir->second < 0) {
        ret += i_dir->second *(grad_[index] - l1weight_);
      } else if (i_dir->second > 0) {
        ret += i_dir->second *(grad_[index] + l1weight_);
      }
      ++i_dir;
    }
  }

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "DirDeriv ret = " << ret << "\n";
#endif  // DEBUG_PRINT_VARS

  return ret;
}


template <>
void Learner<SparseRealVector>::GetNextPoint(double alpha) {
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
    SparseRealVector::const_iterator i_x = x_.begin();
    SparseRealVector::const_iterator i_new_x = new_x_.begin();

    while (i_x != x_.end() && i_new_x != new_x_.end()) {
      if (i_x->first < i_new_x->first) {
        ++i_x;
      } else if (i_new_x->first < i_x->first) {
        ++i_new_x;
      } else {                // i_x->first == i_new_x->first
        if (i_x->second * i_new_x->second < 0) {
          SparseRealVector::const_iterator temp = i_new_x;
          temp++;
          new_x_.set(i_new_x->first, 0);
          i_new_x = temp;
        } else {
          ++i_new_x;
        }
        ++i_x;
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

#endif  // MRML_LASSO_LEARNER_SPARSE_IMPL_H_
