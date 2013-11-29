


#include "sorted_buffer/memory_allocator.h"

#include "base/common.h"
#include "sorted_buffer/memory_piece.h"

namespace sorted_buffer {

//-----------------------------------------------------------------------------
// Implementation of NaiveMemoryAllocator
//-----------------------------------------------------------------------------

NaiveMemoryAllocator::NaiveMemoryAllocator(
    const int pool_size)
    : pool_size_(pool_size),
      allocated_size_(0) {
  CHECK_LT(0, pool_size);
  try {
    pool_ = new char[pool_size];
  } catch(std::bad_alloc&) {
    pool_ = NULL;
    LOG(FATAL) << "Insufficient memory to initialize NaiveMemoryAlloctor with "
               << "pool size = " << pool_size;
  }
}

NaiveMemoryAllocator::~NaiveMemoryAllocator() {
  if (pool_ != NULL) {
    delete [] pool_;
  }
  pool_ = NULL;
  pool_size_ = 0;
  allocated_size_ = 0;
}

bool NaiveMemoryAllocator::Allocate(PieceSize size,
                                    MemoryPiece* piece) {
  CHECK(IsInitialized());
  if (Have(size)) {
    piece->Set(pool_ + allocated_size_, size);
    allocated_size_ += size + sizeof(PieceSize);
    return true;
  }
  piece->Clear();
  return false;
}

bool NaiveMemoryAllocator::Have(PieceSize size) const {
  return size + sizeof(PieceSize) + allocated_size_ <= pool_size_;
}

bool NaiveMemoryAllocator::Have(PieceSize key_length, PieceSize value_length) {
  return allocated_size_ + key_length + value_length + 2 * sizeof(PieceSize)
      <= pool_size_;
}

void NaiveMemoryAllocator::Reset() {
  allocated_size_ = 0;
}

std::ostream& operator<< (std::ostream& output, const MemoryPiece& p) {
  output << "(" << p.Size() << ") ";
  if (p.IsSet()) {
    for (int i = 0; i < p.Size(); ++i) {
      output << p.Data()[i];
    }
  } else {
    output << "[not set]";
  }
  return output;
}

}  // namespace sorted_buffer
