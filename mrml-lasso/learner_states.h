

//
// Define classes and class tempaltes that form the learner states.
// Implement member function templates or friend templates (in
// particular, those for IO ).
//
#ifndef MRML_LASSO_LEARNER_STATES_H_
#define MRML_LASSO_LEARNER_STATES_H_

#include <deque>
#include <sstream>
#include <string>
#include <vector>

#include "base/common.h"
#include "mrml/mrml_filesystem.h"
#include "mrml/mrml_recordio.h"
#include "mrml-lasso/logistic_regression.pb.h"
#include "mrml-lasso/sparse_vector_tmpl.h"

class MRMLFS_File;

namespace logistic_regression {

using std::deque;
using std::istream;
using std::ostream;
using std::pair;
using std::string;
using std::vector;
using std::ostringstream;

//---------------------------------------------------------------------------
// Foward declarations:
//---------------------------------------------------------------------------
class ImprovementFilter;
class ImprovementFilterTestUtil;
template <class RealVector> class RealVectorPtrDeque;
template <class RealVector> class RealVectorPtrDequeTestUtil;
template <class RealVector> class LearnerStates;
template <class RealVector> class LearnerStatesTestUtil;

ostream& operator<<(ostream&, const deque<double>&);
ostream& operator<<(ostream&, const ImprovementFilter&);
template <class RealVector>
ostream& operator<<(ostream&, const RealVectorPtrDeque<RealVector>&);
template <class RealVector>
ostream& operator<<(ostream&, const LearnerStates<RealVector>&);

//---------------------------------------------------------------------------
// RealVectorPtrDeque is the data structure for saving S-list and
// Y-list in class LearnerStates.  The template parameter,
// RealVector, may refer to either DenseRealVector or
// SparseRealVector.
//---------------------------------------------------------------------------
template <class RealVector>
class RealVectorPtrDeque : public deque<RealVector*> {
  friend class RealVectorPtrDequeTestUtil<RealVector>;
  friend ostream& operator<< <RealVector>(ostream&, const
                                          RealVectorPtrDeque<RealVector>&);
 public:
  ~RealVectorPtrDeque() { Cleanup(); }
  inline void Cleanup();
  void WriteAsRecords(MRMLFS_File* file, const string& key_base) const;
  void ReadAsRecords(MRMLFS_File* file, const string& key_base);
};

//---------------------------------------------------------------------------
// ImprovementFilter is a utility class to check whether an
// optimization can stop according to the average improvement of the
// recent num_iterations_to_average iterations.
//---------------------------------------------------------------------------
class ImprovementFilter {
  friend class ImprovementFilterTestUtil;
  friend ostream& operator<<(ostream&, const ImprovementFilter&);
 public:
  // If value_history_ is full of kNumIterationsToAverage elements,
  // return averaged improvement computed from value_history_ and
  // new_value.  Otherwise, returns infinity.  In either case,
  // append new_value to value_history_.
  static const int kNumIterationsToAverage = 5;
  double GetImprovement(double new_value);

  void SerializeToProtoBuf(DoubleSequencePB* pb) const;
  void ParseFromProtoBuf(const DoubleSequencePB& pb);
 private:
  std::deque<double> value_history_;
};

//---------------------------------------------------------------------------
// LearnerStates contains all what we need to persist for pausing
// and resuming of a long-time learning process.  Interface Leaner
// defines the operations with states defined in this class.  The
// template parameter, RealVector, may refer to SparseRealVector or
// DenseRealVector.
//---------------------------------------------------------------------------
template <class RealVector>
class LearnerStates {
  friend class LearnerStatesTestUtil<RealVector>;
  friend ostream& operator<< <RealVector>(ostream&,
                                          const LearnerStates<RealVector>&);

 public:
  LearnerStates(const RealVector& initial_x,
                int memory_size,
                double l1_weight,
                int max_line_search_steps,
                int max_iterations,
                double convergence_tolerance);
  LearnerStates() {}

  void SaveIntoRecordFile(MRMLFS_File* file) const;
  void LoadFromRecordFile(MRMLFS_File* file);
  void DebugOutput(std::ostream* out) const;

  const RealVector& new_x()     const { return new_x_; }
  const RealVector& x()         const { return x_; }
  const RealVector& new_grad()  const { return new_grad_; }
  const RealVector& grad()      const { return grad_; }
  const RealVector& dir()       const { return dir_; }
  const double& value()         const { return value_; }
  const int& iteration()        const { return iteration_; }
  const double& l1weight()      const { return l1weight_; }
  const int& line_search_step() const { return line_search_step_; }
  const int& max_line_search_steps() const { return max_line_search_steps_; }

  RealVector& new_grad()        { return new_grad_; }
  double& value()               { return value_; }

 protected:
  RealVector x_;              // Model parameters.
  RealVector new_x_;          // Model parameters under trial in line search.
  RealVector grad_;           // The gradient of model paramters.
  RealVector new_grad_;
  RealVector dir_;            // The update direction of model parameters.

  RealVectorPtrDeque<RealVector> s_list_;
  RealVectorPtrDeque<RealVector> y_list_;
  std::deque<double> ro_list_;
  std::deque<double> alphas_;  // Has fixed size of memory_size_.

  double value_;               // The value of objective function.
  double old_value_;           // Value before line search in an iteration.
  double dir_deriv_;           // The derivative of dir_ before a line search.
  double step_fraction_;       // The fraction of a step in line search.
  double degrade_factor_;      // The degredation of step fraction.
  int iteration_;              // Increment by Shift().
  int max_iterations_;         // Stop learning once enough number of
  // iterations were done.
  int line_search_step_;       // Increment for each line search step in an
  // iteration, and cleared by Shift().
  int max_line_search_steps_;  // If line search in an iteration is too long,
  // we will stop the learning process.
  double convergence_tolerance_;
  int memory_size_;            // The memory length of BGFS.
  double l1weight_;            // The weight of the L1 regularization term.
  ImprovementFilter improvement_filter_;
};

//---------------------------------------------------------------------------
// Serialization utilities
//---------------------------------------------------------------------------
void SerializeDoubleToProtoBuf(const double& v, DoublePB* pb);
void ParseDoubleFromProtoBuf(const DoublePB& pb, double* v);

void SerializeInt32ToProtoBuf(int32 v, Int32PB* pb);
void ParseInt32FromProtoBuf(const Int32PB& pb, int32* v);

void SerializeDequeToProtoBuf(const std::deque<double>& deque,
                              DoubleSequencePB* pb);
void ParseDequeFromProtoBuf(const DoubleSequencePB& pb,
                            std::deque<double>* deque);

//---------------------------------------------------------------------------
// Implementation of class template RealVectorPtrDeque
//---------------------------------------------------------------------------

template <class RealVector>
inline void RealVectorPtrDeque<RealVector>::
WriteAsRecords(MRMLFS_File* file, const string& key_base) const {
  Int32PB int_pb;
  SerializeInt32ToProtoBuf(this->size(), &int_pb);
  MRML_WriteRecord(file, key_base + ".size", int_pb);

  for (size_t i = 0; i < this->size(); ++i) {
    RealVector* real_vector = (*this)[i];
    ostringstream oss;
    oss << key_base << i;
    if (real_vector != NULL) {
      // If deque[i] == NULL, we write an empty sparse vector.
      // Since we have deque[i]->size() > 0 if deque[i] != NULL, an
      // empty sparse vector can be used to indicate a NULL.
      real_vector->SerializeToRecordIO(file, oss.str());
    }
  }
}

template <class RealVector>
inline void RealVectorPtrDeque<RealVector>::
ReadAsRecords(MRMLFS_File* file, const string& key_base) {
  Cleanup();

  string key;
  Int32PB int_pb;
  MRML_ReadRecord(file, &key, &int_pb);
  CHECK_EQ(key, key_base + ".size");
  int deque_size = 0;
  ParseInt32FromProtoBuf(int_pb, &deque_size);
  CHECK_LE(0, deque_size);

  if (deque_size > 0) {
    this->resize(deque_size);
    for (size_t i = 0; i < deque_size; ++i) {
      int32 vec_size = 0;
      RealVector* vector = new RealVector;
      ostringstream oss;
      oss << key_base << i;
      vector->ParseFromRecordIO(file, oss.str(), vec_size);
      if (vec_size > 0) {
        (*this)[i] = vector;
      } else {
        (*this)[i] = NULL;
      }
    }
  }
}

template <class RealVector>
inline void RealVectorPtrDeque<RealVector>::Cleanup() {
  for (size_t s = 0; s < this->size(); ++s) {
    if ((*this)[s] != NULL)
      delete (*this)[s];
  }
  this->clear();
}

template <class RealVector>
inline ostream& operator<<(ostream& out,
                           const RealVectorPtrDeque<RealVector>& q) {
  for (size_t s = 0; s < q.size(); ++s) {
    if (q[s] != NULL) {
      out << s << ":" << *q[s] << "\t";
    }
  }
  return out;
}

//---------------------------------------------------------------------------
// Implementation of class template LearnerStates
//---------------------------------------------------------------------------

template <class RealVector>
LearnerStates<RealVector>::LearnerStates(const RealVector& initial_x,
                                         int memory_size,
                                         double l1_weight,
                                         int max_line_search_steps,
                                         int max_iterations,
                                         double convergence_tolerance)
    : x_(initial_x),
      new_x_(initial_x),
      alphas_(memory_size),
      value_(0),
      old_value_(0),
      dir_deriv_(0),
      step_fraction_(1),
      degrade_factor_(0.5),
      iteration_(0),
      max_iterations_(max_iterations),
      line_search_step_(0),
      max_line_search_steps_(max_line_search_steps),
      convergence_tolerance_(convergence_tolerance),
      memory_size_(memory_size),
      l1weight_(l1_weight) {
  CHECK_LT(0, memory_size);
  CHECK_LT(0, max_iterations);
  CHECK_LT(0, max_line_search_steps);
  CHECK_LT(0, convergence_tolerance);
}

#define WriteAsRealVectorPB(variable) {             \
    variable.SerializeToRecordIO(file, #variable);  \
  }

#define WriteAsDoubleSequencePB(variable) {     \
    DoubleSequencePB pb;                        \
    SerializeDequeToProtoBuf(variable, &pb);    \
    MRML_WriteRecord(file, #variable, pb);      \
  }

#define WriteAsDoublePB(variable) {             \
    DoublePB pb;                                \
    SerializeDoubleToProtoBuf(variable, &pb);   \
    MRML_WriteRecord(file, #variable, pb);      \
  }

#define WriteAsInt32PB(variable) {              \
    Int32PB pb;                                 \
    SerializeInt32ToProtoBuf(variable, &pb);    \
    MRML_WriteRecord(file, #variable, pb);      \
  }

#define WriteFilterAsDoubleSequencePB(variable) {       \
    DoubleSequencePB pb;                                \
    variable.SerializeToProtoBuf(&pb);                  \
    MRML_WriteRecord(file, #variable, pb);              \
  }

template <class RealVector>
void LearnerStates<RealVector>::SaveIntoRecordFile(MRMLFS_File* file) const {
  WriteAsRealVectorPB(x_);
  WriteAsRealVectorPB(new_x_);
  WriteAsRealVectorPB(grad_);
  WriteAsRealVectorPB(new_grad_);
  WriteAsRealVectorPB(dir_);
  s_list_.WriteAsRecords(file, "s_list_");
  y_list_.WriteAsRecords(file, "y_list_");
  WriteAsDoubleSequencePB(ro_list_);
  WriteAsDoubleSequencePB(alphas_);
  WriteAsDoublePB(value_);
  WriteAsDoublePB(old_value_);
  WriteAsDoublePB(dir_deriv_);
  WriteAsDoublePB(step_fraction_);
  WriteAsDoublePB(degrade_factor_);
  WriteAsDoublePB(l1weight_);
  WriteAsDoublePB(convergence_tolerance_);
  WriteAsInt32PB(iteration_);
  WriteAsInt32PB(line_search_step_);
  WriteAsInt32PB(max_line_search_steps_);
  WriteAsInt32PB(max_iterations_);
  WriteAsInt32PB(memory_size_);
  WriteFilterAsDoubleSequencePB(improvement_filter_);
}

#undef WriteAsRealVectorPB
#undef WriteAsDoubleSequencePB
#undef WriteAsDoublePB
#undef WriteAsInt32PB
#undef WriteFilterAsDoubleSequencePB


#define ReadAsRealVectorPB(variable) {                      \
    int32 vec_size;                                         \
    variable.ParseFromRecordIO(file, #variable, vec_size);  \
  }

#define ReadAsDoubleSequencePB(variable) {      \
    DoubleSequencePB pb;                        \
    MRML_ReadRecord(file, &key, &pb);           \
    CHECK_EQ(key, #variable);                   \
    ParseDequeFromProtoBuf(pb, &variable);      \
  }

#define ReadAsDoublePB(variable) {              \
    DoublePB pb;                                \
    MRML_ReadRecord(file, &key, &pb);           \
    CHECK_EQ(key, #variable);                   \
    ParseDoubleFromProtoBuf(pb, &variable);     \
  }

#define ReadAsInt32PB(variable) {               \
    Int32PB pb;                                 \
    MRML_ReadRecord(file, &key, &pb);           \
    CHECK_EQ(key, #variable);                   \
    ParseInt32FromProtoBuf(pb, &variable);      \
  }

#define ReadFilterAsDoubleSequencePB(variable) {        \
    DoubleSequencePB pb;                                \
    MRML_ReadRecord(file, &key, &pb);                   \
    CHECK_EQ(key, #variable);                           \
    variable.ParseFromProtoBuf(pb);                     \
  }

template <class RealVector>
void LearnerStates<RealVector>::LoadFromRecordFile(MRMLFS_File* file) {
  string key;
  ReadAsRealVectorPB(x_);
  ReadAsRealVectorPB(new_x_);
  ReadAsRealVectorPB(grad_);
  ReadAsRealVectorPB(new_grad_);
  ReadAsRealVectorPB(dir_);
  s_list_.ReadAsRecords(file, "s_list_");
  y_list_.ReadAsRecords(file, "y_list_");
  ReadAsDoubleSequencePB(ro_list_);
  ReadAsDoubleSequencePB(alphas_);
  ReadAsDoublePB(value_);
  ReadAsDoublePB(old_value_);
  ReadAsDoublePB(dir_deriv_);
  ReadAsDoublePB(step_fraction_);
  ReadAsDoublePB(degrade_factor_);
  ReadAsDoublePB(l1weight_);
  ReadAsDoublePB(convergence_tolerance_);
  ReadAsInt32PB(iteration_);
  ReadAsInt32PB(line_search_step_);
  ReadAsInt32PB(max_line_search_steps_);
  ReadAsInt32PB(max_iterations_);
  ReadAsInt32PB(memory_size_);
  ReadFilterAsDoubleSequencePB(improvement_filter_);
}

#undef ReadAsRealVectorPB
#undef ReadAsDoubleSequencePB
#undef ReadAsDoublePB
#undef ReadAsInt32PB
#undef ReadFilterAsDoubleSequencePB


template <class RealVector>
ostream& operator<<(ostream& out, const LearnerStates<RealVector>& s) {
  out << "x : " << s.x_ << "\n"
      << "new_x : " << s.new_x_ << "\n"
      << "grad : " << s.grad_ << "\n"
      << "new_grad : " << s.new_grad_ << "\n"
      << "dir : " << s.dir_ << "\n"
      << "s_list : " << s.s_list_ << "\n"
      << "y_list : " << s.y_list_ << "\n"
      << "ro_list : " << s.ro_list_ << "\n"
      << "alphas : " << s.alphas_ << "\n"
      << "value : " << s.value_ << "\n"
      << "old_value : " << s.old_value_ << "\n"
      << "dir_deriv : " << s.dir_deriv_ << "\n"
      << "step_fraction : " << s.step_fraction_ << "\n"
      << "degrade_factor : " << s.degrade_factor_ << "\n"
      << "l1weight : " << s.l1weight_ << "\n"
      << "convergence_tolerace : " << s.convergence_tolerance_ << "\n"
      << "iteration : " << s.iteration_ << "\n"
      << "line_search_step : " << s.line_search_step_ << "\n"
      << "max_line_search_steps : " << s.max_line_search_steps_ << "\n"
      << "max_iterations : " << s.max_iterations_ << "\n"
      << "memory_size : " << s.memory_size_ << "\n"
      << "improvement_filter : " << s.improvement_filter_ << "\n";
  return out;
}

}  // namespace logistic_regression

#endif  // MRML_LASSO_LEARNER_STATES_H_
