

//
// This file exports the MD5 hashing algorithm.
//
#ifndef HASH_MD5_HASH_H_
#define HASH_MD5_HASH_H_

#include <string>

#include "base/common.h"

uint64 MD5Hash(const unsigned char *s, const unsigned int len);
uint64 MD5Hash(const std::string& s);

#endif  // HASH_MD5_HASH_H_
