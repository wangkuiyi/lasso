

//
// Define the class template DenseVectorImpl and operations required
// by class template Learner.
//
#ifndef MRML_LASSO_DENSE_VECTOR_TMPL_H_
#define MRML_LASSO_DENSE_VECTOR_TMPL_H_

#include <vector>

namespace logistic_regression {
using std::ostream;
using std::vector;

template <class ValueType>
class DenseVectorTmpl : public vector<ValueType> {
 public:
  typedef typename vector<ValueType>::const_iterator const_iterator;
  typedef typename vector<ValueType>::iterator iterator;

  DenseVectorTmpl(size_t size, const ValueType& init)
      : vector<ValueType>(size, init) {}

  DenseVectorTmpl()
      : vector<ValueType>() {}
};

// Scale(v,c) : v <- v * c
template <class ValueType, class ScaleType>
void Scale(DenseVectorTmpl<ValueType>* v,
           const ScaleType& c) {
  for (size_t i = 0; i < v->size(); ++i) {
    (*v)[i] *= c;
  }
}

// ScaleInto(u,v,c) : u <- v * c
template <class ValueType, class ScaleType>
void ScaleInto(DenseVectorTmpl<ValueType>* u,
               const DenseVectorTmpl<ValueType>& v,
               const ScaleType& c) {
  CHECK_EQ(v.size(), u->size());
  CHECK_LT(0, v.size());
  for (size_t i = 0; i < v.size(); ++i) {
    (*u)[i] = v[i] * c;
  }
}

// AddScaled(u,v,c) : u <- u + v * c
template <class ValueType, class ScaleType>
void AddScaled(DenseVectorTmpl<ValueType>* u,
               const DenseVectorTmpl<ValueType>& v,
               const ScaleType& c) {
  CHECK_EQ(v.size(), u->size());
  CHECK_LT(0, v.size());
  for (size_t i = 0; i < v.size(); ++i) {
    (*u)[i] += v[i] * c;
  }
}

// AddScaledInto(w,u,v,c) : w <- u + v * c
template <class ValueType, class ScaleType>
void AddScaledInto(DenseVectorTmpl<ValueType>* w,
                   const DenseVectorTmpl<ValueType>& u,
                   const DenseVectorTmpl<ValueType>& v,
                   const ScaleType& c) {
  CHECK_EQ(u.size(), v.size());
  CHECK_EQ(u.size(), w->size());
  CHECK_LT(0, u.size());
  for (size_t i = 0; i < u.size(); ++i) {
    (*w)[i] = u[i] + v[i] * c;
  }
}

// DotProduct(u,v) : r <- dot(u, v)
template <class ValueType>
ValueType DotProduct(const DenseVectorTmpl<ValueType>& v1,
                     const DenseVectorTmpl<ValueType>& v2) {
  CHECK_EQ(v1.size(), v2.size());
  ValueType ret = 0;
  for (size_t i = 0; i < v1.size(); ++i) {
    ret += v1[i] * v2[i];
  }
  return ret;
}

// Output a sparse vector in human readable format.
template <class ValueType>
ostream& operator<< (ostream& output,
                     const DenseVectorTmpl<ValueType>& vec) {
  output << "[ ";
  for (size_t i = 0; i < vec.size(); ++i) {
    if (vec[i] != 0)  // to keep the format the same with sparse
      output << i << ":" <<vec[i] << " ";
  }
  output << "]";
  return output;
}

}  // namespace logistic_regression

#endif  // MRML_LASSO_DENSE_VECTOR_TMPL_H_
