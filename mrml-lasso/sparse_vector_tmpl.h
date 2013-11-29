

//
// Define the class template DenseVectorImpl and operations required
// by class template Learner.
//
#ifndef MRML_LASSO_SPARSE_VECTOR_TMPL_H_
#define MRML_LASSO_SPARSE_VECTOR_TMPL_H_

#include <map>

namespace logistic_regression {

using std::map;
using std::pair;
using std::ostream;

// We use std::map as the container of a sparse vector.  Because
// std::map implements a sorting tree (RB-tree), so its iterator
// accesses elements in known order of keys.  This is important for
// sparse vector operations like dot-product and add-multi-into.
// The ValueType must be a numerical type supporting const 0.
template <class KeyType, class ValueType>
class SparseVectorTmpl : public map<KeyType, ValueType> {
 public:
  typedef typename map<KeyType, ValueType>::const_iterator const_iterator;
  typedef typename map<KeyType, ValueType>::iterator iterator;

  // We constrain operator[] a read-only operation to prevent
  // accidential insert of elements.
  const ValueType& operator[](const KeyType& key) const {
    const_iterator iter = this->find(key);
    if (iter == this->end()) {
      return zero_;
    }
    return iter->second;
  }

  // Set a value at given key.  If value==0, an exisiting key-value
  // pair is removed.  If value!=0, the value is set or inserted.
  // This function also serves as a convenient form of insert(), no
  // need to use std::pair.
  void set(const KeyType& key, const ValueType& value) {
    iterator iter = this->find(key);
    if (iter != this->end()) {
      if (IsZero(value)) {
        this->erase(iter);
      } else {
        iter->second = value;
      }
    } else {
      if (!IsZero(value)) {
        this->insert(pair<KeyType, ValueType>(key, value));
      }
    }
  }

  bool has(const KeyType& key) const {
    return this->find(key) != this->end();
  }

 protected:
  static const ValueType zero_;

  static bool IsZero(const ValueType& value) {
    // Once, we used zero-judgement like:
    //   static double kEpsilon = 1e-12;
    //   return (value - zero_) * (value - zero_) < kEpsilon;
    // however, this does not work as exactly as (value==0).
    return value == 0;
  }
};


template <class KeyType, class ValueType>
const ValueType SparseVectorTmpl<KeyType, ValueType>::zero_(0);


// Scale(v,c) : v <- v * c
template <class KeyType, class ValueType, class ScaleType>
void Scale(SparseVectorTmpl<KeyType, ValueType>* v,
           const ScaleType& c) {
  typedef SparseVectorTmpl<KeyType, ValueType> SV;
  for (typename SV::iterator i = v->begin(); i != v->end(); ++i) {
    i->second *= c;
  }
}

// ScaleInto(u,v,c) : u <- v * c
template <class KeyType, class ValueType, class ScaleType>
void ScaleInto(SparseVectorTmpl<KeyType, ValueType>* u,
               const SparseVectorTmpl<KeyType, ValueType>& v,
               const ScaleType& c) {
  typedef SparseVectorTmpl<KeyType, ValueType> SV;
  u->clear();
  for (typename SV::const_iterator i = v.begin(); i != v.end(); ++i) {
    u->set(i->first, i->second * c);
  }
}

// AddScaled(u,v,c) : u <- u + v * c
template <class KeyType, class ValueType, class ScaleType>
void AddScaled(SparseVectorTmpl<KeyType, ValueType>* u,
               const SparseVectorTmpl<KeyType, ValueType>& v,
               const ScaleType& c) {
  typedef SparseVectorTmpl<KeyType, ValueType> SV;
  for (typename SV::const_iterator i = v.begin(); i != v.end(); ++i) {
    u->set(i->first, (*u)[i->first] + i->second * c);
  }
}

// AddScaledInto(w,u,v,c) : w <- u + v * c
template <class KeyType, class ValueType, class ScaleType>
void AddScaledInto(SparseVectorTmpl<KeyType, ValueType>* w,
                   const SparseVectorTmpl<KeyType, ValueType>& u,
                   const SparseVectorTmpl<KeyType, ValueType>& v,
                   const ScaleType& c) {
  typedef SparseVectorTmpl<KeyType, ValueType> SV;
  w->clear();
  typename SV::const_iterator i = u.begin();
  typename SV::const_iterator j = v.begin();
  while (i != u.end() && j != v.end()) {
    if (i->first == j->first) {
      w->set(i->first, i->second + j->second * c);
      ++i;
      ++j;
    } else if (i->first < j->first) {
      w->set(i->first, i->second);
      ++i;
    } else {
      w->set(j->first, j->second * c);
      ++j;
    }
  }
  while (i != u.end()) {
    w->set(i->first, i->second);
    ++i;
  }
  while (j != v.end()) {
    w->set(j->first, j->second * c);
    ++j;
  }
}

// DotProduct(u,v) : r <- dot(u, v)
template <class KeyType, class ValueType>
ValueType DotProduct(const SparseVectorTmpl<KeyType, ValueType>& v1,
                     const SparseVectorTmpl<KeyType, ValueType>& v2) {
  typedef SparseVectorTmpl<KeyType, ValueType> SV;
  typename SV::const_iterator i = v1.begin();
  typename SV::const_iterator j = v2.begin();
  ValueType ret = 0;
  while (i != v1.end() && j != v2.end()) {
    if (i->first == j->first) {
      ret += i->second * j->second;
      ++i;
      ++j;
    } else if (i->first < j->first) {
      ++i;
    } else {
      ++j;
    }
  }
  return ret;
}

// Output a sparse vector in human readable format.
template <class KeyType, class ValueType>
ostream& operator<<(ostream& output,
                    const SparseVectorTmpl<KeyType, ValueType>& vec) {
  typedef SparseVectorTmpl<KeyType, ValueType> SV;
  output << "[ ";
  for (typename SV::const_iterator i = vec.begin(); i != vec.end(); ++i) {
    output << i->first << ":" << i->second << " ";
  }
  output << "]";
  return output;
}

}  // namespace logistic_regression

#endif  // MRML_LASSO_SPARSE_VECTOR_TMPL_H_
