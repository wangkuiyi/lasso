

//
// Define the interface of Reader and two standard readers: TextReader
// and RecordReader.
//
#ifndef MRML_MRML_READER_H_
#define MRML_MRML_READER_H_

#include <stdio.h>

#include <string>

// The interface implemented by ``real'' readers.
class MRML_Reader {
 public:
  virtual ~MRML_Reader() {}

  // Returns false to indicate that the current read failed and no
  // further reading operations should be performed.
  virtual bool Read(std::string* key, std::string* value) = 0;

 protected:
  FILE* input_stream_;
};

// Read from a text file, using stdio.h API.
// - The key returned by Read() is "filename-offset", the value
//   returned by Read is the content of a line.
// - The value might be empty if it is reading a too long line.
// - The '\r' (if there is any) and '\n' at the end of a line are
//   removed.
class MRML_TextReader : public MRML_Reader {
 public:
  explicit MRML_TextReader(const std::string& filename,
                           int max_line_length);
  virtual ~MRML_TextReader();
  virtual bool Read(std::string* key, std::string* value);

 private:
  int max_line_length_;
  char* line_;                   // input line buffer
  int line_num_;                 // count line number
  bool reading_a_long_line_;     // is reading a lone line
  std::string input_filename_;
  FILE* input_stream_;
};

// Read from a MRML RecordIO file, using MRML_RecordIO API.
class MRML_RecordReader : public MRML_Reader {
 public:
  explicit MRML_RecordReader(const std::string& filename);
  virtual ~MRML_RecordReader();
  virtual bool Read(std::string* key, std::string* value);

 private:
  std::string input_filename_;
  FILE* input_stream_;
};

#endif  // MRML_MRML_READER_H_
