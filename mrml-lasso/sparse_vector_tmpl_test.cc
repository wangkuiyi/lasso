

//
#include <string>

#include "base/common.h"
#include "gtest/gtest.h"
#include "mrml-lasso/sparse_vector_tmpl.h"

using logistic_regression::SparseVectorTmpl;

typedef SparseVectorTmpl<uint32, double> RealVector;

TEST(SparseVectorTmplTest, SquareBrackets) {
  RealVector v;
  v.set(101, 1);
  EXPECT_EQ(v[101], 1);
  EXPECT_EQ(v[102], 0);
  EXPECT_EQ(v.has(101), true);
  EXPECT_EQ(v.has(102), false);
}

TEST(SparseVectorTmplTest, Set) {
  RealVector v;
  EXPECT_EQ(v.size(), 0);
  v.set(101, 0);
  EXPECT_EQ(v.size(), 0);
  EXPECT_EQ(v.has(101), false);
  v.set(101, 1);
  EXPECT_EQ(v.size(), 1);
  EXPECT_EQ(v.has(101), true);
  EXPECT_EQ(v[101], 1);
  v.set(101, 2);
  EXPECT_EQ(v.size(), 1);
  EXPECT_EQ(v.has(101), true);
  EXPECT_EQ(v[101], 2);
  v.set(101, 0);
  EXPECT_EQ(v.size(), 0);
  EXPECT_EQ(v.has(101), false);
}

TEST(SparseVectorTmplTest, Scale) {
  RealVector v;
  v.set(101, 2);
  v.set(102, 4);
  Scale(&v, 0.5);
  EXPECT_EQ(v.size(), 2);
  EXPECT_EQ(v[101], 1);
  EXPECT_EQ(v[102], 2);
}

TEST(SparseVectorTmplTest, ScaleInto) {
  RealVector u, v;
  u.set(200, 2);
  v.set(101, 2);
  v.set(102, 4);
  ScaleInto(&u, v, 0.5);
  EXPECT_EQ(u.size(), 2);
  EXPECT_EQ(u[101], 1);
  EXPECT_EQ(u[102], 2);
}

TEST(SparseVectorTmplTest, AddScaled) {
  RealVector u, v;
  u.set(200, 2);
  v.set(101, 2);
  v.set(102, 4);
  AddScaled(&u, v, 0.5);
  EXPECT_EQ(u.size(), 3);
  EXPECT_EQ(u[200], 2);
  EXPECT_EQ(u[101], 1);
  EXPECT_EQ(u[102], 2);
}

TEST(SparseVectorTmplTest, AddScaledInto) {
  RealVector w, u, v;
  w.set(200, 100);
  u.set(101, 2);
  u.set(102, 4);
  u.set(301, 8);
  u.set(302, 100);
  v.set(101, 2);
  v.set(103, 6);
  v.set(301, 8);
  AddScaledInto(&w, u, v, 0.5);
  EXPECT_EQ(w.size(), 5);
  EXPECT_EQ(w[101], 3);
  EXPECT_EQ(w[102], 4);
  EXPECT_EQ(w[103], 3);
  EXPECT_EQ(w[301], 12);
  EXPECT_EQ(w[302], 100);
}

TEST(SparseVectorTmplTest, DotProduct) {
  RealVector v, u, w;
  v.set(101, 2);
  v.set(102, 4);
  v.set(301, 9);
  v.set(302, 100);
  u.set(101, 2);
  u.set(103, 6);
  u.set(301, 9);
  w.set(200, 10);
  EXPECT_EQ(DotProduct(v, u), 85);
  EXPECT_EQ(DotProduct(u, v), 85);
  EXPECT_EQ(DotProduct(v, w), 0);
  EXPECT_EQ(DotProduct(u, w), 0);
}


