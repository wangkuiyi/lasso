

//
#include "strutil/stringprintf.h"

#include "gtest/gtest.h"

TEST(StringPrintf, normal) {
  using std::string;
  EXPECT_EQ(StringPrintf("%d", 1), string("1"));
  string target;
  SStringPrintf(&target, "%d", 1);
  EXPECT_EQ(target, string("1"));
  StringAppendF(&target, "%d", 2);
  EXPECT_EQ(target, string("12"));
}
