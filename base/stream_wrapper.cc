


//
#include "base/stream_wrapper.h"

#include <cstring>
#include <fstream>
#include <iostream>

namespace stream_wrapper {

ostream_wrapper::ostream_wrapper(const char* filename)
    : output_stream_(0) {
  if (std::strcmp(filename, "-") == 0)
    output_stream_ = &std::cout;
  else
    output_stream_ = new std::ofstream(filename);
}

ostream_wrapper::~ostream_wrapper() {
  if (output_stream_ != &std::cout) delete output_stream_;
}

istream_wrapper::istream_wrapper(const char* filename)
    : input_stream_(0) {
  if (std::strcmp(filename, "-") == 0)
    input_stream_ = &std::cin;
  else
    input_stream_ = new std::ifstream(filename, std::ios::binary|std::ios::in);
}

istream_wrapper::~istream_wrapper() {
  if (input_stream_ != &std::cin) delete input_stream_;
}

}
