

//
#include "mrml-lasso/logistic_regression.pb.h"
#include "mrml-lasso/test_utils.h"
#include "mrml-lasso/vector_types.h"

using logistic_regression::RealVectorTestUtil;
using logistic_regression::DenseRealVector;
using logistic_regression::SparseRealVector;
using logistic_regression::RealVectorPB;

TEST(VectorTypesTest, DenseVectorSerialization) {
  RealVectorTestUtil<DenseRealVector> u;
  DenseRealVector v;
  u.Construct(&v);

  RealVectorPB pb;
  v.SerializeToProtoBuf(&pb);
  u.CheckProtoBuf(pb);

  v.clear();
  v.ParseFromProtoBuf(pb);
  u.Check(v);
}

TEST(VectorTypesTest, SparseVectorSerialization) {
  RealVectorTestUtil<SparseRealVector> u;
  SparseRealVector v;
  u.Construct(&v);

  RealVectorPB pb;
  v.SerializeToProtoBuf(&pb);
  u.CheckProtoBuf(pb);

  v.ParseFromProtoBuf(pb);
  u.Check(v);
}

TEST(VectorTypesTest, HybridDotProduct) {
  RealVectorTestUtil<SparseRealVector> su;
  SparseRealVector sv;
  su.Construct(&sv);

  RealVectorTestUtil<DenseRealVector> du;
  DenseRealVector dv;
  du.Construct(&dv);

  EXPECT_EQ(0, DotProduct(sv, dv));
}

TEST(VectorTypesTest, HybridAddScaled) {
  RealVectorTestUtil<SparseRealVector> su;
  SparseRealVector sv;
  su.Construct(&sv);

  RealVectorTestUtil<DenseRealVector> du;
  DenseRealVector dv;
  du.Construct(&dv);

  AddScaled(&dv, sv, 2);

  EXPECT_EQ(4, dv.size());
  EXPECT_EQ(10, dv[0]);
  EXPECT_EQ(20, dv[1]);
  EXPECT_EQ(30, dv[2]);
  EXPECT_EQ(60, dv[3]);
}
