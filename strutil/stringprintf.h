

//
// This code comes from the re2 project host on Google Code
// (http://code.google.com/p/re2/), in particular, the following source file
// http://code.google.com/p/re2/source/browse/util/stringprintf.cc
//
#ifndef STRUTIL_STRINGPRINTF_H_
#define STRUTIL_STRINGPRINTF_H_

#include <string>

std::string StringPrintf(const char* format, ...);
void SStringPrintf(std::string* dst, const char* format, ...);
void StringAppendF(std::string* dst, const char* format, ...);

#endif  // STRUTIL_STRINGPRINTF_H_
