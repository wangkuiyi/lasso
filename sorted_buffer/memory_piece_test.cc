

//
#include <gtest/gtest.h>

#include "sorted_buffer/memory_piece.h"

using sorted_buffer::MemoryPiece;
using sorted_buffer::PieceSize;

TEST(MemoryPieceTest, SetMemoryPiece) {
  MemoryPiece p;
  CHECK(!p.IsSet());

  char buffer[1024];
  p.Set(buffer, 100);
  CHECK(p.IsSet());
  CHECK_EQ(p.Piece(), buffer);
  CHECK_EQ(p.Size(), 100);
  CHECK_EQ(p.Data(), buffer + sizeof(PieceSize));
}

TEST(MemoryPieceTest, ConstructNULLPiece) {
  char buffer[sizeof(PieceSize)];
  MemoryPiece p(buffer, 0);
  CHECK(p.IsSet());
  CHECK_EQ(p.Piece(), buffer);
  CHECK_EQ(p.Size(), 0);
  CHECK_EQ(p.Data(), buffer + sizeof(PieceSize));
}

TEST(MemoryPieceTest, ConstructMemoryPiece) {
  char buffer[1024];
  MemoryPiece p(buffer, 100);
  CHECK(p.IsSet());
  CHECK_EQ(p.Piece(), buffer);
  CHECK_EQ(p.Size(), 100);
  CHECK_EQ(p.Data(), buffer + sizeof(PieceSize));
}
