

//
// Define the mappers and reducers using class template Learner.
//
#ifndef MRML_LASSO_MRML_MAPPERS_AND_REDUCERS_H_
#define MRML_LASSO_MRML_MAPPERS_AND_REDUCERS_H_

#include <string>

#include "base/common.h"
#include "strutil/split_string.h"
#include "mrml/mrml.h"
#include "mrml-lasso/learner.h"
#include "mrml-lasso/learner_sparse_impl.h"
#include "mrml-lasso/learner_dense_impl.h"
#include "mrml-lasso/command_line_options.h"

namespace logistic_regression {

// All map outputs have the same key, and will be reduced by a
// unique reduce worker.  This ensures the gradient and value is
// computed by summation over all training instances.
extern const char* kUniqueKey;

// ComputeGradientMapper computes the value and the gradient of the
// logistic loss function (without the regularization term).
template <class RealVector>
class ComputeGradientMapper : public MRML_Mapper {
 public:
  ComputeGradientMapper();
  void Map(const std::string& key, const std::string& value);
  void Flush();

 private:
  RealVector feature_weights_;  // The model parameters.
  DenseRealVector combined_gradient_;
  double combined_loss_;

  SparseRealVector partial_gradient_;
  LearnerStates<RealVector> states_;

  CommandLineOptions options_;
};


// Given the value and gradient of the logistic loss function computed
// by ComputeGradientMapper, UpdateModelReducer either (i) initializes
// the model, (ii) determine a gradient descent direction, or (iii)
// does a line search prob step.
template <class RealVector>
class UpdateModelReducer : public MRML_Reducer {
 public:
  UpdateModelReducer() { options_.Parse(GetConfig()); }
  void* BeginReduce(const std::string& key, const std::string& value);
  void PartialReduce(const std::string& key, const std::string& value,
                     void* partial_result);
  void EndReduce(const std::string& key, void* partial_result);

 private:
  struct PartialReduceInfo {
    std::string word_;
    Learner<RealVector> *learner;
    double value;
    RealVector gradient;
  };

  RealVector initial_x_;
  RealVector partial_gradient_;
  CommandLineOptions options_;
};

class ComputeDenseGradientMapper
    : public ComputeGradientMapper<DenseRealVector> {
};

class UpdateDenseModelReducer
    : public UpdateModelReducer<DenseRealVector> {
};

class ComputeSparseGradientMapper
    : public ComputeGradientMapper<SparseRealVector> {
};

class UpdateSparseModelReducer
    : public UpdateModelReducer<SparseRealVector> {
};

}  // namespace logistic_regression

#endif  // MRML_LASSO_MRML_MAPPERS_AND_REDUCERS_H_
