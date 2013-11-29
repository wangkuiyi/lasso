

//
// This file defines the logistic regression inference engine.

#ifndef MRML_LASSO_PREDICTION_ENGINE_H_
#define MRML_LASSO_PREDICTION_ENGINE_H_

#include <map>
#include <vector>
#include <string>

namespace logistic_regression {

class PredictionEngine {
 public:
  explicit PredictionEngine(const std::string& model_file_name);
  ~PredictionEngine();

  /*
   * 函数名称: Predict
   * 函数功能: 计算一组特征的逻辑回归值
   * 函数输入: vector<int>& feature_names 特征名称
   *           vector<double>& feature_values 特征值
   *           double& ret 特征的逻辑回归值
   * 函数输出: -1, 失败
   *            0, 成功
   */
  int Predict(const std::vector<int>& feature_names,
              const std::vector<double>& feature_values,
              double& ret);
 private:
  std::map<int, double> feature_weights_;
};

}  // namespace logistic_regression

#endif  // MRML_LASSO_PREDICTION_ENGINE_H_
