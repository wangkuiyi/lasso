

//
#include <string.h>

#include <fstream>
#include <string>

#include "gtest/gtest.h"

#include "sorted_buffer/memory_piece.h"

namespace sorted_buffer {

TEST(MemoryPieceIOTest, MemoryPieceIO) {
  static const char* kTmpFile = "/tmp/testMemoryPieceIO";

  std::string s("apple");
  std::string empty("");
  char buffer[] = "1234orange";
  char buffer2[sizeof(PieceSize)];

  MemoryPiece p0(&empty);
  MemoryPiece p1(&s);
  MemoryPiece p2(buffer2, 0);
  MemoryPiece p3(buffer, strlen("orange"));

  FILE* output = fopen(kTmpFile, "w+");
  CHECK(output != NULL);
  EXPECT_TRUE(WriteMemoryPiece(output, p0));
  EXPECT_TRUE(WriteMemoryPiece(output, p1));
  EXPECT_TRUE(WriteMemoryPiece(output, p2));
  EXPECT_TRUE(WriteMemoryPiece(output, p3));
  fclose(output);

  FILE* input = fopen(kTmpFile, "r");
  CHECK(input != NULL);
  std::string p;
  EXPECT_TRUE(ReadMemoryPiece(input, &p));
  EXPECT_TRUE(p.empty());
  EXPECT_TRUE(ReadMemoryPiece(input, &p));
  EXPECT_EQ(p, "apple");
  EXPECT_TRUE(ReadMemoryPiece(input, &p));
  EXPECT_TRUE(p.empty());
  EXPECT_TRUE(ReadMemoryPiece(input, &p));
  EXPECT_EQ(p, "orange");
  EXPECT_TRUE(!ReadMemoryPiece(input, &p));
  fclose(input);
}

TEST(MemoryPieceIOTest, ReadFromEmptyFile) {
  static const char* kTmpFile = "/tmp/testReadFromEmptyFile";

  // Create an empty file.
  FILE* output = fopen(kTmpFile, "w+");
  CHECK(output != NULL);
  fclose(output);

  FILE* input = fopen(kTmpFile, "r");
  CHECK(input != NULL);
  std::string p;
  EXPECT_TRUE(!ReadMemoryPiece(input, &p));
  fclose(input);
}

}  // namespace sorted_buffer
