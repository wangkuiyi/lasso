

//
// Define class templates Learner and implements part of its member
// functions that do not depend on the vector type (dense or sparse).
//
#ifndef MRML_LASSO_LEARNER_H_
#define MRML_LASSO_LEARNER_H_

#include <math.h>

#include "mrml-lasso/vector_types.h"
#include "mrml-lasso/learner_states.h"
#include "mrml-lasso/termination_flag.h"

namespace logistic_regression {

// Forward declaration:
template <class RealVector> class LearnerTestUtil;

// Learner encapsulate learning / optimization operations on
// LearnerStates.  There are two specializations of this class
// template: one for dense vector representation, defined in
// learner-dense-impl.hh, and another for sparse vector
// representation, defined in learner-sparse-impl.h.
//
// After the (MapReduce or local) evaluator finishes computing value
// and gradient of the loss function, it notifies Learner the result
// via SetObjectiveValueAndGradient().  Then it invokes Initialize()
// once and a successive number of GradientDescent().
template <class RealVector>
class Learner : public LearnerStates<RealVector> {
  friend class LearnerTestUtil<RealVector>;

 public:
  Learner(const RealVector& initial_x,
          int memory_size,
          double l1_weight,
          int max_line_search_steps,
          int max_iterations,
          double convergence_tolerance,
          int max_feature_number);

  // This default constructor is used only for unit testing of the
  // sparse version algorithm implementation. So it has only
  // realization of the sparse version.
  Learner();

  void SetObjectiveValueAndGradient(double value, RealVector* gradient);
  void Initialize(const char* term_flag_filename);
  void GradientDescent(const char* term_flag_filename);

 protected:
  typedef TerminationFlag<LearnerStates<RealVector> > TermFlag;

  void UpdateDir();
  void MakeSteepestDescDir();
  void MapDirByInverseHessian();
  void FixDirSigns();

  void IncreaseMemory(RealVector** next_s, RealVector** next_y);

  double DirDeriv() const;
  void GetNextPoint(double alpha);
  void Shift();
};

//---------------------------------------------------------------------------
// Implementation of the part of Learner, which does not depends on
// the difference between dense/sparse algorithms.
//---------------------------------------------------------------------------

#ifdef DEBUG_PRINT_TRACE
#  define PRINT_EXECUTION_TRACE std::cout << __FUNCTION__ << "\n";
#else
#  define PRINT_EXECUTION_TRACE
#endif  // DEBUG_PRINT_TRACE


extern const char* kErrorNonDescentDirection;
extern const char* kEnoughLongLineSearch;
extern const char* kEnoughNumberOfIterations;


template <class RealVector>
Learner<RealVector>::Learner(const RealVector& initial_x,
                             int memory_size,
                             double l1_weight,
                             int max_line_search_steps,
                             int max_iterations,
                             double convergence_tolerance,
                             int max_feature_number)
    : LearnerStates<RealVector>(initial_x,
                                memory_size,
                                l1_weight,
                                max_line_search_steps,
                                max_iterations,
                                convergence_tolerance) {}

template <class RealVector>
void
Learner<RealVector>::SetObjectiveValueAndGradient(double value,
                                                  RealVector* gradient) {
  PRINT_EXECUTION_TRACE;

  this->value_ = value;
  this->new_grad_.swap(*gradient);

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "value = " << this->value_ << "\n"
            << "new_x = " << this->new_x_ << "\n"
            << "grad = " << this->grad_ << "\n"
            << "new_grad = " << this->new_grad_ << "\n"
            << "memory = " << this->memory_size_ << "\n";
#endif  // DEBUG_PRINT_VARS
}


// Expect that SetObjectiveValueAndGradient was just invoked.
template <class RealVector>
void Learner<RealVector>::Initialize(const char* term_flag_filename) {
  this->grad_ = this->new_grad_;
  this->improvement_filter_.GetImprovement(this->value_);

  UpdateDir();
  this->dir_deriv_ = DirDeriv();
  if (this->dir_deriv_ >= 0) {
    TermFlag::SetLocally(term_flag_filename, kErrorNonDescentDirection,
                         this);
    return;
  }

  // In the first (initialization) iteration, we adopt a heuristic
  // choice of step fraction and step backoff (degrade factor) for
  // line search.
  CHECK_EQ(this->iteration_, 0);
  CHECK_EQ(this->line_search_step_, 0);
  double norm_dir = sqrt(DotProduct(this->dir_, this->dir_));
  this->step_fraction_ = (1 / norm_dir);
  this->degrade_factor_ = 0.1;

  // Start some steps of line search.
  this->old_value_ = this->value_;
  GetNextPoint(this->step_fraction_);
}


// Expect that SetObjectiveValueAndGradient was just invoked.
template <class RealVector>
void Learner<RealVector>::GradientDescent(const char* term_flag_filename) {
  static const double c1 = 1e-4;

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "value = " << this->value_ << "\n"
            << "old_value = " << this->old_value_ << "\n"
            << "dir_deriv = " << this->dir_deriv_ << "\n"
            << "step_fraction = " << this->step_fraction_ << "\n";
#endif  // DEBUG_PRINT_VARS

  if (this->value_ <=
      this->old_value_ + c1 * this->dir_deriv_ * this->step_fraction_) {
    // We have succeeded with a line search and may start a new iteration.
    if (this->improvement_filter_.GetImprovement(this->value_) <
        this->convergence_tolerance_) {
      TermFlag::SetLocally(term_flag_filename,
                           "SUCCEEDED: We have converged.",
                           this);
      return;
    }

    // Shift states into a new iteration.  This increases iteration_
    // and clear line_search_step_.
    Shift();

    if (this->iteration_ > this->max_iterations_) {
      TermFlag::SetLocally(term_flag_filename,
                           kEnoughNumberOfIterations, this);
      return;
    }

    UpdateDir();
    this->dir_deriv_ = DirDeriv();
    if (this->dir_deriv_ >= 0) {
      TermFlag::SetLocally(term_flag_filename,
                           kErrorNonDescentDirection,
                           this);
      return;
    }

    // For iterations other than iteration, use a simple choice of
    // step fraction and degrade factor.
    if (this->iteration_ > 0) {
      this->step_fraction_ = 1;
      this->degrade_factor_ = 0.5;
    }

    // Start some steps of line search
    this->old_value_ = this->value_;
    GetNextPoint(this->step_fraction_);
  } else {
    // We need to go on with line searching.
    ++this->line_search_step_;
    if (this->line_search_step_ > this->max_line_search_steps_) {
      TermFlag::SetLocally(term_flag_filename, kEnoughLongLineSearch, this);
      return;
    }
    this->step_fraction_ *= this->degrade_factor_;
    GetNextPoint(this->step_fraction_);
  }
}

template <class RealVector>
void Learner<RealVector>::UpdateDir() {
  PRINT_EXECUTION_TRACE;

  MakeSteepestDescDir();
  MapDirByInverseHessian();
  FixDirSigns();
}

template <>
void Learner<DenseRealVector>::IncreaseMemory(DenseRealVector** next_s,
                                              DenseRealVector** next_y) {
  *next_s = new DenseRealVector(x_.size(), 0);
  *next_y = new DenseRealVector(x_.size(), 0);
}

template <>
void Learner<SparseRealVector>::IncreaseMemory(SparseRealVector** next_s,
                                               SparseRealVector** next_y) {
  *next_s = new SparseRealVector;
  *next_y = new SparseRealVector;
}

template <class RealVector>
void Learner<RealVector>::Shift() {
  PRINT_EXECUTION_TRACE;

  RealVector* next_s = NULL;
  RealVector* next_y = NULL;

  // If s_list_.size < memory_size_, we try to expand s_list_.  If
  // we cannot (due to memeory allocation error), we reduce
  // memory_size_.
  if (this->s_list_.size() < this->memory_size_) {
    try {
      IncreaseMemory(&next_s, &next_y);
    } catch(std::bad_alloc) {
      this->memory_size_ = this->s_list_.size();
      if (next_s != NULL) {
        delete next_s;
        next_s = NULL;
      }
      if (next_y != NULL) {
        delete next_y;
        next_y = NULL;
      }
    }
  }

  if (next_s == NULL) {
    CHECK_LT(0, this->s_list_.size());
    CHECK_LT(0, this->y_list_.size());
    CHECK_LT(0, this->ro_list_.size());
    next_s = this->s_list_.front();
    this->s_list_.pop_front();
    next_y = this->y_list_.front();
    this->y_list_.pop_front();
    this->ro_list_.pop_front();
  }

  AddScaledInto(next_s, this->new_x_,    this->x_,    -1);
  AddScaledInto(next_y, this->new_grad_, this->grad_, -1);
  double ro = DotProduct(*next_s, *next_y);

  this->s_list_.push_back(next_s);
  this->y_list_.push_back(next_y);
  this->ro_list_.push_back(ro);

  this->x_.swap(this->new_x_);
  this->grad_.swap(this->new_grad_);

  this->line_search_step_ = 0;
  ++(this->iteration_);

#ifdef DEBUG_PRINT_VARS
  std::cout << __FUNCTION__ << "@" << __FILE__ << ":" << __LINE__ << "\n"
            << "x = " << this->x_ << "\n"
            << "new_x = " << this->new_x_ << "\n"
            << "dir = " << this->dir_ << "\n"
            << "grad = " << this->grad_ << "\n"
            << "new_grad = " << this->new_grad_ << "\n";
#endif  // DEBUG_PRINT_VARS
}

}  // namespace logistic_regression

#endif  // MRML_LASSO_LEARNER_H_
