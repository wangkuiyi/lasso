

//
#include <sstream>
#include <string>
#include "mrml-lasso/vector_types.h"
#include "mrml-lasso/logistic_regression.pb.h"

namespace logistic_regression {

void SparseRealVector::SerializeToProtoBuf(RealVectorPB* pb) const {
  pb->Clear();
  for (const_iterator iter = begin(); iter != end(); ++iter) {
    RealVectorPB::Element* e = pb->add_element();
    e->set_index(iter->first);
    e->set_value(iter->second);
  }
}

void SparseRealVector::ParseFromProtoBuf(const RealVectorPB& pb) {
  clear();
  for (int i = 0; i < pb.element_size(); ++i) {
    const RealVectorPB::Element& e = pb.element(i);
    this->set(e.index(), e.value());
  }
}

void SparseRealVector::SerializeToRecordIO(MRMLFS_File* file, 
                                           const std::string& key_base) const {
  int32 vec_size = 0;
  int32 vec_dim = 0;
  for (const_iterator iter = begin(); iter != end(); ++iter) {
    ++vec_size;
    if (vec_dim < iter->first)
      vec_dim = iter->first;
  }
  Int32PB int_pb;
  int_pb.set_value(vec_dim);
  MRML_WriteRecord(file, key_base + ".dim", int_pb);
  int_pb.set_value(vec_size);
  MRML_WriteRecord(file, key_base + ".size", int_pb);
  
  const_iterator iter = begin();
  int fragment_num = vec_size/kMessageSize +
                     ((vec_size % kMessageSize == 0) ? 0 : 1);
  for (int i = 0; i < fragment_num; ++i) {
    RealVectorPB vec_pb;
    for (int j = 0; iter != end() && j < kMessageSize; ++iter) {
      RealVectorPB::Element* e = vec_pb.add_element();
      e->set_index(iter->first);
      e->set_value(iter->second);
      ++j;
    }
    MRML_WriteRecord(file, key_base, vec_pb);
  }
}

void SparseRealVector::ParseFromRecordIO(MRMLFS_File* file, 
                                         const std::string& key_base,
                                         int32& vec_size) {
  clear();
  std::string key;
  Int32PB int_pb;
  MRML_ReadRecord(file, &key, &int_pb);
  CHECK_EQ(key, key_base + ".dim");
  MRML_ReadRecord(file, &key, &int_pb);
  CHECK_EQ(key, key_base + ".size");
  vec_size = int_pb.value();
  CHECK_LE(0, vec_size);
  
  int fragment_num = vec_size/kMessageSize +
                     ((vec_size % kMessageSize == 0) ? 0 : 1);
  for (int i = 0; i < fragment_num; ++i) {
    RealVectorPB vec_pb;
    MRML_ReadRecord(file, &key, &vec_pb);
    CHECK_EQ(key, key_base);
    for (int j = 0; j < vec_pb.element_size(); ++j) {
      const RealVectorPB::Element& e = vec_pb.element(j);
      this->set(e.index(), e.value());
    }
  }
}

void DenseRealVector::SerializeToProtoBuf(RealVectorPB* pb) const {
  pb->Clear();
  pb->set_dim(size());
  for (size_t i = 0; i < size(); ++i) {
    const double& value = (*this)[i];
    if (value != 0) {
      RealVectorPB::Element* e = pb->add_element();
      e->set_index(i);
      e->set_value(value);
    }
  }
}

void DenseRealVector::ParseFromProtoBuf(const RealVectorPB& pb) {
  clear();
  resize(pb.dim(), 0);       // Note: zerolize is required.
  for (int i = 0; i < pb.element_size(); ++i) {
    const RealVectorPB::Element& e = pb.element(i);
    (*this)[e.index()] = e.value();
  }
}

void DenseRealVector::SerializeToRecordIO(MRMLFS_File* file,
                                          const std::string& key_base) const {
  int32 vec_size = 0;
  for (size_t i = 0; i < size(); ++i) {
    const double& value = (*this)[i];
    if (value != 0)
      ++vec_size;
  }
  Int32PB int_pb;
  int_pb.set_value(size());
  MRML_WriteRecord(file, key_base + ".dim", int_pb);
  int_pb.set_value(vec_size);
  MRML_WriteRecord(file, key_base + ".size", int_pb);
  
  int fragment_num = size()/kMessageSize +
                     ((size() % kMessageSize == 0) ? 0 : 1);
  for (int i = 0; i < fragment_num; ++i) {
    RealVectorPB vec_pb;
    for (size_t j = i * kMessageSize; j < (i + 1) * kMessageSize
         && j < size(); ++j) {
      const double& value = (*this)[j];
      if (value != 0) {
        RealVectorPB::Element* e = vec_pb.add_element();
        e->set_index(j);
        e->set_value(value);
      }
    }
    MRML_WriteRecord(file, key_base, vec_pb);
  }
}

void DenseRealVector::ParseFromRecordIO(MRMLFS_File* file, 
                                        const std::string& key_base,
                                        int32& vec_size) {
  clear();
  std::string key;
  Int32PB int_pb;
  MRML_ReadRecord(file, &key, &int_pb);
  CHECK_EQ(key, key_base + ".dim");
  int32 dim = int_pb.value();
  CHECK_LE(0, dim);
  resize(dim, 0);       // Note: zerolize is required.

  MRML_ReadRecord(file, &key, &int_pb);
  CHECK_EQ(key, key_base + ".size");
  vec_size = int_pb.value();
  CHECK_LE(0, vec_size);

  int fragment_num = size()/kMessageSize +
                     ((size() % kMessageSize == 0) ? 0 : 1);
  for (int i = 0; i < fragment_num; ++i) {
    RealVectorPB vec_pb;
    MRML_ReadRecord(file, &key, &vec_pb);
    CHECK_EQ(key, key_base);
    for (int j = 0; j < vec_pb.element_size(); ++j)
    {
      const RealVectorPB::Element& e = vec_pb.element(j);
      (*this)[e.index()] = e.value();
    } 
  }
}
  
double DotProduct(const SparseRealVector& sv, const DenseRealVector& dv) {
  double ret = 0;
  for (SparseRealVector::const_iterator i = sv.begin(); i != sv.end(); ++i) {
    if (i->first < dv.size()) {
      ret += dv[i->first] * i->second;
    } else {
      break;
    }
  }
  return ret;
}

void AddScaled(DenseRealVector* dv, const SparseRealVector& sv, double f) {
  for (SparseRealVector::const_iterator i = sv.begin(); i != sv.end(); ++i) {
    while (dv->size() <= i->first) {
      dv->push_back(0);
    }
    (*dv)[i->first] += i->second * f;
  }
}

}  // namespace logistic_regression
