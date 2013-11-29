

//
#include "gtest/gtest.h"

#include "strutil/strcodec.h"

TEST(StrCodecTest, testFastCodecInt32) {
  std::string str;
  EncodeInt32(0, &str);
  EXPECT_EQ(0, DecodeInt32(str));

  EncodeInt32(1, &str);
  EXPECT_EQ(1, DecodeInt32(str));

  EncodeInt32(-1, &str);
  EXPECT_EQ(-1, DecodeInt32(str));

  EncodeInt32(0xffffffff, &str);
  EXPECT_EQ(0xffffffff, DecodeInt32(str));
}

TEST(StrCodecTest, testFastCodecUint64) {
  std::string str;
  EncodeUint64(0, &str);
  EXPECT_EQ(0, DecodeUint64(str));

  EncodeUint64(1, &str);
  EXPECT_EQ(1, DecodeUint64(str));

  EncodeUint64(0xffffffffffffffffLLU, &str);
  EXPECT_EQ(0xffffffffffffffffLLU, DecodeUint64(str));
}

TEST(StrCodecTest, testInt32ToKey) {
  std::string key;
  Int32ToKey(0, &key);
  EXPECT_EQ(key, "0000000000");

  Int32ToKey(1, &key);
  EXPECT_EQ(key, "0000000001");
}

TEST(StrCodecTest, testKeyToInt32) {
  std::string key;
  Int32ToKey(0, &key);
  EXPECT_EQ(KeyToInt32(key), 0);

  Int32ToKey(1, &key);
  EXPECT_EQ(KeyToInt32(key), 1);
}
