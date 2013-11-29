

//
// MemoryPiece encapsulate either a pointer to a std::string, or a
// pointer to a memory block with size = sizeof(PieceSize) +
// block_size, where the first sizeof(PieceSize) bytes saves the value
// of block_size.  The main purpose of MemoryPiece is to save string
// in raw memory allocated from user-defined memory pool.  The reason
// of encapsulating std::string is to make it possible to compare a
// std::string with a MemoryPiece that encapsulate a memory-block.
//
// MemoryPieceLessThan is a binary comparator for sorting MemoryPieces
// in lexical order.
//
// ReadMemoryPiece and WriteMemoryPiece supports (local) file IO of
// MemoryPieces.
//
#ifndef SORTED_BUFFER_MEMORY_PIECE_H_
#define SORTED_BUFFER_MEMORY_PIECE_H_

#include <stdio.h>
#include <functional>
#include <string>

#include "base/common.h"

namespace sorted_buffer {

typedef uint32 PieceSize;


// Represent either a piece of memory, which is prepended by a
// PieceSize, or a std::string object.
class MemoryPiece {
  friend std::ostream& operator<< (std::ostream&, const MemoryPiece& p);

 public:
  MemoryPiece() : piece_(NULL), string_(NULL) {}
  MemoryPiece(char* piece, PieceSize size) { Set(piece, size); }
  explicit MemoryPiece(std::string* string) { Set(string); }

  void Set(char* piece, PieceSize size) {
    CHECK_LE(0, size);
    CHECK_NOTNULL(piece);
    // TODO(charlieyan): fix bug here
    piece_ = piece;
    *reinterpret_cast<PieceSize*>(piece_) = size;
    string_ = NULL;
  }

  void Set(std::string* string) {
    CHECK_NOTNULL(string);
    string_ = string;
    piece_ = NULL;
  }

  void Clear() {
    piece_ = NULL;
    string_ = NULL;
  }

  bool IsSet() const { return IsString() || IsPiece(); }
  bool IsString() const { return string_ != NULL; }
  bool IsPiece() const { return piece_ != NULL; }

  const char* Piece() const { return piece_; }

  char* Data() {
    return IsPiece() ? piece_ + sizeof(PieceSize) :
        (IsString() ? const_cast<char*>(string_->data()) : NULL);
  }

  const char* Data() const {
    return IsPiece() ? piece_ + sizeof(PieceSize) :
        (IsString() ? string_->data() : NULL);
  }

  size_t Size() const {
    return IsPiece() ? *reinterpret_cast<PieceSize*>(piece_) :
        (IsString() ? string_->size() : 0);
  }

 private:
  char* piece_;
  std::string* string_;
};


// Compare two MemoryPiece objects in lexical order.
struct MemoryPieceLessThan : public std::binary_function<const MemoryPiece&,
                                                         const MemoryPiece&,
                                                         bool> {
  bool operator() (const MemoryPiece& x, const MemoryPiece& y) const;
};

bool MemoryPieceEqual(const MemoryPiece& x, const MemoryPiece& y);

bool WriteMemoryPiece(FILE* output, const MemoryPiece& piece);
bool ReadMemoryPiece(FILE* input, std::string* piece);

}  // namespace sorted_buffer

#endif  // SORTED_BUFFER_MEMORY_PIECE_H_
