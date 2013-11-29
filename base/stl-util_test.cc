

//
#include <vector>
#include <map>

#include "gtest/gtest.h"

#include "base/common.h"
#include "base/stl-util.h"

TEST(STLUtilTest, DeleteElementsInVector) {
  std::vector<int*> v;
  v.push_back(new int(10));
  v.push_back(new int(20));

  EXPECT_EQ(2, v.size());
  EXPECT_EQ(10, *(v[0]));
  EXPECT_EQ(20, *(v[1]));

  STLDeleteElementsAndClear(&v);

  EXPECT_EQ(0, v.size());
}

TEST(STLUtilTest, DeleteElementsInMap) {
  std::map<int, int*> m;
  m[100] = new int(10);
  m[200] = new int(20);

  EXPECT_EQ(2, m.size());
  EXPECT_EQ(10, *(m[100]));
  EXPECT_EQ(20, *(m[200]));

  STLDeleteValuesAndClear(&m);

  EXPECT_EQ(0, m.size());
}
