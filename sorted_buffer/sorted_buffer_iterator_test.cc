

//
#include "sorted_buffer/sorted_buffer_iterator.h"

#include "gtest/gtest.h"

#include "base/common.h"
#include "base/varint32.h"
#include "sorted_buffer/sorted_buffer.h"

namespace sorted_buffer {

TEST(SortedBufferIteratorTest, SortedBufferIterator) {
  // The following code snippet that generates a series of two disk
  // block files are copied from sorted_buffer_test.cc
  //
  static const std::string kTmpFilebase("/tmp/testSortedBufferIterator");
  static const int kInMemBufferSize = 40;  // Can hold two key-value pairs
  static const std::string kSomeStrings[] = {
    "applee", "applee", "applee", "papaya" };
  static const std::string kValue("123456");
  {
    SortedBuffer buffer(kTmpFilebase, kInMemBufferSize);
    for (int k = 0; k < sizeof(kSomeStrings)/sizeof(kSomeStrings[0]); ++k) {
      buffer.Insert(kSomeStrings[k], kValue);
    }
    buffer.Flush();
  }

  int i = 0;
  for (SortedBufferIteratorImpl iter(kTmpFilebase, 2); !iter.FinishedAll();
       iter.NextKey()) {
    for (; !iter.Done(); iter.Next()) {
      EXPECT_EQ(iter.key(), kSomeStrings[i]);
      EXPECT_EQ(iter.value(), kValue);
      ++i;
    }
  }
}

}  // namespace sorted_buffer
