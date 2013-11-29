


#include "base/varint32.h"

#include <fstream>

#include "gtest/gtest.h"

#include "base/common.h"

TEST(Varint32Test, WriteAndReadVarint32) {
  static const char* kTmpFile = "/tmp/varint32_test.tmp";

  uint32 kTestValues[] = { 0, 1, 0xff, 0xffff, 0xffffffff };

  FILE* output = fopen(kTmpFile, "w+");
  for (int i = 0; i < sizeof(kTestValues)/sizeof(kTestValues[0]); ++i) {
    if (!WriteVarint32(output, kTestValues[i])) {
      LOG(FATAL) << "Error on WriteVarint32 with value= " << kTestValues[i];
    }
  }
  fclose(output);

  FILE* input = fopen(kTmpFile, "r");
  for (int i = 0; i < sizeof(kTestValues)/sizeof(kTestValues[0]); ++i) {
    uint32 value;
    if (!ReadVarint32(input, &value)) {
      LOG(FATAL) << "Error on ReadVarint32 with value = " << kTestValues[i];
    }
    EXPECT_EQ(kTestValues[i], value);
  }
  fclose(input);
}
