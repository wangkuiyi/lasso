

//
#include "sorted_buffer/sorted_buffer.h"

#include "base/common.h"
#include "base/varint32.h"
#include "gtest/gtest.h"

namespace sorted_buffer {

class SortedBufferTest : public ::testing::Test {};

TEST_F(SortedBufferTest, OneFlushFile) {
  static const std::string kTmpFilebase("/tmp/testOneFlushFile");
  static const int kInMemBufferSize = 1024;
  static const std::string kSomeStrings[] = {
    "applee", "banana", "orange", "papaya" };

  {
    SortedBuffer buffer(kTmpFilebase, kInMemBufferSize);
    for (int k = 0; k < sizeof(kSomeStrings)/sizeof(kSomeStrings[0]); ++k) {
      for (int v = 0; v <= k; ++v) {
        buffer.Insert(kSomeStrings[k], kSomeStrings[v]);
      }
    }
    EXPECT_EQ(200, buffer.Allocator()->AllocatedSize());
    buffer.Flush();
    EXPECT_EQ(buffer.NumFiles(), 1);
  }

  std::string filename = kTmpFilebase + "-0000000000";
  FILE* input = fopen(filename.c_str(), "r");
  CHECK(input != NULL);

  std::string piece;
  uint32 num_values;
  for (int k = 0; k < sizeof(kSomeStrings)/sizeof(kSomeStrings[0]); ++k) {
    EXPECT_TRUE(ReadMemoryPiece(input, &piece)) << "k = " << k;
    EXPECT_EQ(piece, kSomeStrings[k]);
    EXPECT_TRUE(ReadVarint32(input, &num_values));
    EXPECT_EQ(num_values, k + 1);

    for (int v = 0; v <= k; ++v) {
      EXPECT_TRUE(ReadMemoryPiece(input, &piece));
      EXPECT_EQ(piece, kSomeStrings[v]);
    }
  }

  fclose(input);
}

TEST_F(SortedBufferTest, MultipleFlushFiles) {
  static const std::string kTmpFilebase("/tmp/testMultipleFlushFiles");
  static const int kInMemBufferSize = 40;  // Can hold two key-value pairs
  static const std::string kSomeStrings[] = {
    "applee", "banana", "applee", "papaya" };
  static const std::string kValue("123456");

  {
    SortedBuffer buffer(kTmpFilebase, kInMemBufferSize);
    for (int k = 0; k < sizeof(kSomeStrings)/sizeof(kSomeStrings[0]); ++k) {
      buffer.Insert(kSomeStrings[k], kValue);
    }
    buffer.Flush();
    EXPECT_EQ(buffer.NumFiles(), 2);
  }
}

}  // namespace sorted_buffer
