

//
#include "sorted_buffer/sorted_buffer_iterator.h"

#include "base/varint32.h"
#include "sorted_buffer/memory_piece.h"
#include "sorted_buffer/sorted_buffer.h"

namespace sorted_buffer {

SortedBufferIteratorImpl::SortedBufferIteratorImpl(const std::string& filebase,
                                                   int num_files) {
  Initialize(filebase, num_files);
}

SortedBufferIteratorImpl::~SortedBufferIteratorImpl() {
  Clear();
}

void SortedBufferIteratorImpl::Initialize(const std::string& filebase,
                                          int num_files) {
  CHECK_LE(0, num_files);
  filebase_ = filebase;

  for (int i = 0; i < num_files; ++i) {
    SortedStringFile* file = new SortedStringFile;
    files_.push_back(file);

    file->index = i;
    file->input =
        fopen(SortedBuffer::SortedFilename(filebase, i).c_str(), "r");
    if (file->input == NULL) {
      LOG(FATAL) << "Cannot open file: "
                 << SortedBuffer::SortedFilename(filebase, i);
    }
    CHECK(LoadKey(file));
    CHECK(LoadValue(file));
  }

  RelocateMergeSource();
}

const std::string& SortedBufferIteratorImpl::key() const {
  return current_key_;
}

const std::string& SortedBufferIteratorImpl::value() const {
  return merge_source_->top_value;
}

void SortedBufferIteratorImpl::Next() {
  if (!LoadValue(merge_source_)) {
    SortedStringFile* equal = FindNextMergeSourceWithEqualKey();
    if (equal != NULL) {
      if (LoadKey(merge_source_)) {
        LoadValue(merge_source_);
      }
      merge_source_ = equal;
    } else {
      --(merge_source_->num_rest_values);
    }
  }
}

void SortedBufferIteratorImpl::DiscardRestValues() {
  while (merge_source_->num_rest_values >= 0) {
    Next();
  }
}

bool SortedBufferIteratorImpl::Done() const {
  return merge_source_->num_rest_values < 0;
}

void SortedBufferIteratorImpl::NextKey() {
  DiscardRestValues();
  CHECK(Done());
  if (LoadKey(merge_source_)) {
    LoadValue(merge_source_);
  }
  RelocateMergeSource();
}

bool SortedBufferIteratorImpl::FinishedAll() const {
  return merge_source_ == NULL;
}

bool SortedBufferIteratorImpl::LoadValue(SortedStringFile* file) {
  if (file->num_rest_values > 0) {
    --(file->num_rest_values);
    if (!ReadMemoryPiece(file->input, &(file->top_value))) {
      LOG(FATAL) << "Error loading value for "
                 << "key = " << file->top_key << " file = "
                 << SortedBuffer::SortedFilename(filebase_, file->index);
    }
    return true;
  }
  return false;
}

bool SortedBufferIteratorImpl::LoadKey(SortedStringFile* file) {
  if (!ReadMemoryPiece(file->input, &(file->top_key))) {
    --(file->num_rest_values);  // Negative value means "end-of-sorted_buffer".
    return false;
  }
  if (!ReadVarint32(file->input,
                    reinterpret_cast<uint32*>(&(file->num_rest_values)))) {
    LOG(FATAL) << "Error load num_rest_values from: "
               << SortedBuffer::SortedFilename(filebase_, file->index);
  }
  if (file->num_rest_values <= 0) {
    LOG(FATAL) << "Zero num_rest_values loaded from "
               << SortedBuffer::SortedFilename(filebase_, file->index);
  }
  return true;
}

void SortedBufferIteratorImpl::RelocateMergeSource() {
  // If two files have the same top_key, the one with smaller index
  // value is located.
  merge_source_ = NULL;
  for (SSFileList::iterator i = files_.begin(); i != files_.end(); ++i) {
    if ((merge_source_ == NULL || (*i)->top_key < merge_source_->top_key) &&
        (*i)->num_rest_values >= 0) {
      merge_source_ = *i;
    }
  }
  if (merge_source_ != NULL) {
    current_key_ = merge_source_->top_key;
  }
}

SortedBufferIteratorImpl::SortedStringFile*
SortedBufferIteratorImpl::FindNextMergeSourceWithEqualKey() {
  for (SSFileList::iterator i = files_.begin(); i != files_.end(); ++i) {
    if (*i != merge_source_ &&
        (*i)->num_rest_values >= 0 &&
        (*i)->top_key == merge_source_->top_key) {
      return *i;
    }
  }
  return NULL;
}

void SortedBufferIteratorImpl::Clear() {
  for (SSFileList::iterator i = files_.begin(); i != files_.end(); ++i) {
    fclose((*i)->input);
    delete *i;
  }
  files_.clear();
}

}  // namespace sorted_buffer
