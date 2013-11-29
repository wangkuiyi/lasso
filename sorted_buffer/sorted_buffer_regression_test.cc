

//
#include <map>
#include <string>

#include "base/random.h"
#include "gtest/gtest.h"
#include "sorted_buffer/sorted_buffer_iterator.h"
#include "sorted_buffer/sorted_buffer.h"

using std::map;
using std::string;
using sorted_buffer::SortedBufferIteratorImpl;
using sorted_buffer::SortedBuffer;

TEST(SortedBufferTest, SortedBuffer) {
  LOG(INFO) << "Running ...";

  static const char* kTmpFile = "/tmp/sorted_buffer_regtext_file";
  static const int kMaxFileSize = 1024 * 1024;
  static const int kNumMaxOutputs = 5 * 1024 * 1024;

  static const string kVocabulary[] =
      { "BBM:", "Bayesian", "Browsing", "Model", "from",
        "Petabyte", "scale", "Data"};
  static const int kVocabularySize =
      sizeof(kVocabulary) / sizeof(kVocabulary[0]);

  MTRandom rng;

  std::map<string, int> ground_truth;
  SortedBuffer buffer(kTmpFile, kMaxFileSize);
  for (int i = 0; i < kNumMaxOutputs; ++i) {
    const string& word = kVocabulary[rng.RandInt(kVocabularySize)];
    buffer.Insert(word, "1");
    ++ground_truth[word];
  }
  buffer.Flush();

  map<string, int>::const_iterator i = ground_truth.begin();
  SortedBufferIteratorImpl* iter =
      reinterpret_cast<SortedBufferIteratorImpl*>(buffer.CreateIterator());
  for (; !(iter->FinishedAll()); iter->NextKey()) {
    int count = 0;
    for (; !(iter->Done()); iter->Next()) {
      ++count;
    }
    EXPECT_EQ(iter->key(), i->first);
    EXPECT_EQ(count, i->second);
    ++i;
  }
  delete iter;
}
