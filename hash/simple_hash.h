

//
// This source file is mainly copied from http://www.partow.net, with
// the following Copyright information.
/*
 **************************************************************************
 *                                                                        *
 *          General Purpose Hash Function Algorithms Library              *
 *                                                                        *
 * Author: Arash Partow - 2002                                            *
 * URL: http://www.partow.net                                             *
 * URL: http://www.partow.net/programming/hashfunctions/index.html        *
 *                                                                        *
 * Copyright notice:                                                      *
 * Free use of the General Purpose Hash Function Algorithms Library is    *
 * permitted under the guidelines and in accordance with the most current *
 * version of the Common Public License.                                  *
 * http://www.opensource.org/licenses/cpl1.0.php                          *
 *                                                                        *
 **************************************************************************
*/
#ifndef HASH_SIMPLE_HASH_H_
#define HASH_SIMPLE_HASH_H_

#include <string>

#include "base/common.h"

typedef unsigned int (*HashFunction)(const std::string&);

unsigned int RSHash(const std::string& str);
unsigned int JSHash(const std::string& str);
unsigned int PJWHash(const std::string& str);
unsigned int ELFHash(const std::string& str);
unsigned int BKDRHash(const std::string& str);
unsigned int SDBMHash(const std::string& str);
unsigned int DJBHash(const std::string& str);
unsigned int DEKHash(const std::string& str);
unsigned int BPHash(const std::string& str);
unsigned int FNVHash(const std::string& str);
unsigned int APHash(const std::string& str);

#endif  // HASH_SIMPLE_HASH_H_
