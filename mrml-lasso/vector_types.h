

//
// Here defines classes DenseRealVector and SparseRealVector, which
// will be used in class templates: RealVectorPtrDeque and
// LearnerStates.
//
// Note that both classes serialize to the same protocol message
// RealVectorPB; this allows LearnerStates<DenseRealVector> and
// LearnerStates<SparseRealVector> share the same protocol message
// LearnerStatesPB.
//
#ifndef MRML_LASSO_VECTOR_TYPES_H_
#define MRML_LASSO_VECTOR_TYPES_H_

#include <vector>

#include "base/common.h"
#include "mrml/mrml_filesystem.h"
#include "mrml/mrml_recordio.h"
#include "mrml-lasso/sparse_vector_tmpl.h"
#include "mrml-lasso/dense_vector_tmpl.h"

namespace logistic_regression {

const int kMessageSize = 4000000;

class RealVectorPB;

typedef uint32 IndexType;     // The index type in SparseRealVector

//---------------------------------------------------------------------------
// DenseRealVector, a vector<double> realization of DenseVectorTmpl.
//---------------------------------------------------------------------------
class DenseRealVector : public DenseVectorTmpl<double> {
 public:
  typedef std::vector<double>::const_iterator const_iterator;
  typedef std::vector<double>::iterator iterator;

  DenseRealVector(size_t size, const double& init)
      : DenseVectorTmpl<double>(size, init) {}
  DenseRealVector()
      : DenseVectorTmpl<double>() {}

  void SerializeToProtoBuf(RealVectorPB* pb) const;
  void SerializeToRecordIO(MRMLFS_File* file, 
                           const std::string& key_base) const;
  void ParseFromProtoBuf(const RealVectorPB& pb);
  void ParseFromRecordIO(MRMLFS_File* file,
                         const std::string& key_base, int32& vec_size);
};

//---------------------------------------------------------------------------
// SparseRealVector, a map<uint32, double> realization of SparseVectorTmpl.
//---------------------------------------------------------------------------
class SparseRealVector : public SparseVectorTmpl<IndexType, double> {
 public:
  typedef SparseVectorTmpl<uint32, double>::const_iterator const_iterator;
  typedef SparseVectorTmpl<uint32, double>::iterator iterator;
  void SerializeToProtoBuf(RealVectorPB* pb) const;
  void SerializeToRecordIO(MRMLFS_File* file,
                           const std::string& key_base) const;
  void ParseFromProtoBuf(const RealVectorPB& pb);
  void ParseFromRecordIO(MRMLFS_File* file,
                         const std::string& key_base, int32& vec_size);
};


//---------------------------------------------------------------------------
// Vector operations that accepts a dense vector and a sparse
// vector.  Note that operations defined in
// sparse/dense-vector-impl.h accept either sparse or dense operands.
//---------------------------------------------------------------------------
double DotProduct(const SparseRealVector& sv, const DenseRealVector& dv);
void AddScaled(DenseRealVector* dv, const SparseRealVector& sv, double f);

}  // namespace logistic_regression

#endif  // MRML_LASSO_VECTOR_TYPES_H_
