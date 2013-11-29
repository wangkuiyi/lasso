

//
#include "strutil/split_string.h"

#include "base/common.h"

using std::string;
using std::vector;
using std::set;

// In most cases, delim contains only one character.  In this case, we
// use CalculateReserveForVector to count the number of elements
// should be reserved in result vector, and thus optimize SplitStringUsing.
static int CalculateReserveForVector(const string& full, const char* delim) {
  int count = 0;
  if (delim[0] != '\0' && delim[1] == '\0') {
    // Optimize the common case where delim is a single character.
    char c = delim[0];
    const char* p = full.data();
    const char* end = p + full.size();
    while (p != end) {
      if (*p == c) {  // This could be optimized with hasless(v,1) trick.
        ++p;
      } else {
        while (++p != end && *p != c) {
          // Skip to the next occurence of the delimiter.
        }
        ++count;
      }
    }
  }
  return count;
}

void SplitStringUsing(const string& full,
                      const char* delim,
                      vector<string>* result) {
  CHECK(delim != NULL);
  CHECK(result != NULL);
  result->reserve(CalculateReserveForVector(full, delim));
  back_insert_iterator< vector<string> > it(*result);
  SplitStringToIteratorUsing(full, delim, it);
}

void SplitStringToSetUsing(const string& full,
                           const char* delim,
                           set<string>* result) {
  CHECK(delim != NULL);
  CHECK(result != NULL);
  simple_insert_iterator<set<string> > it(result);
  SplitStringToIteratorUsing(full, delim, it);
}
