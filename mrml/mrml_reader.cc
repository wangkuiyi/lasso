

//
#include "mrml/mrml_reader.h"

#include <stdio.h>

#include "base/common.h"
#include "base/logging.h"
#include "mrml/mrml_recordio.h"
#include "strutil/stringprintf.h"

static void OpenFileOrDie(const std::string& filename, FILE** input_stream) {
  *input_stream = fopen(filename.c_str(), "r");
  if (*input_stream == NULL) {
    LOG(FATAL) << "Cannot open file: " << filename;
  }
}

//-----------------------------------------------------------------------------
// Implementation of MRML_TextReader
//-----------------------------------------------------------------------------

MRML_TextReader::MRML_TextReader(const std::string& filename,
                                 int max_line_length)
    : max_line_length_(max_line_length),
      line_num_(0),
      reading_a_long_line_(false),
      input_filename_(filename) {
  OpenFileOrDie(filename, &input_stream_);
  try {
    CHECK_LT(1, max_line_length_);  // At least 1 for '\0' appended by fgets.
    line_ = new char[max_line_length_];
  } catch(std::bad_alloc&) {
    line_ = NULL;
    LOG(FATAL) << "Cannot allocate line input buffer.";
  }
}

MRML_TextReader::~MRML_TextReader() {
  if (input_stream_ != NULL) {
    fclose(input_stream_);
    input_stream_ = NULL;
  }
  if (line_ != NULL) {
    delete [] line_;
    line_ = NULL;
  }
}

bool MRML_TextReader::Read(std::string* key, std::string* value) {
  SStringPrintf(key, "%s-%010lld",
                input_filename_.c_str(), ftell(input_stream_));
  value->clear();

  if (fgets(line_, max_line_length_, input_stream_) == NULL) {
    return false;  // Either ferror or feof. Anyway, returns false to
                   // notify the caller no further reading operations.
  }

  int read_size = strlen(line_);
  if (line_[read_size - 1] != '\n') {
    LOG(ERROR) << "Encountered a too-long line (line_num = " << line_num_
               << ").  May return one or more empty values while skipping "
               << " this long line.";
    reading_a_long_line_ = true;
    return true;  // Skip the current part of a long line.
  } else {
    ++line_num_;
    if (reading_a_long_line_) {
      reading_a_long_line_ = false;
      return true;  // Skip the last part of a long line.
    }
  }

  if (line_[read_size - 1] == '\n') {
    line_[read_size - 1] = '\0';
    if (line_[read_size - 2] == '\r') {  // Handle DOS text format.
      line_[read_size - 2] = '\0';
    }
  }
  value->assign(line_);
  return true;
}

//-----------------------------------------------------------------------------
// Implementation of MRML_RecordReader
//-----------------------------------------------------------------------------

MRML_RecordReader::MRML_RecordReader(const std::string& filename)
    : input_filename_(filename) {
  OpenFileOrDie(filename, &input_stream_);
}

MRML_RecordReader::~MRML_RecordReader() {
  if (input_stream_ != NULL) {
    fclose(input_stream_);
    input_stream_ = NULL;
  }
}

bool MRML_RecordReader::Read(std::string* key, std::string* value) {
  return MRML_ReadRecord(input_stream_, key, value);
}
