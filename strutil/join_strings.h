

//
#ifndef STRUTIL_JOIN_STRINGS_H_
#define STRUTIL_JOIN_STRINGS_H_

#include <string>

template <class ConstForwardIterator>
void JoinStrings(const ConstForwardIterator& begin,
                 const ConstForwardIterator& end,
                 const std::string& delimiter,
                 std::string* output) {
  output->clear();
  for (ConstForwardIterator iter = begin; iter != end; ++iter) {
    if (iter != begin) {
      output->append(delimiter);
    }
    output->append(*iter);
  }
}

template <class ConstForwardIterator>
std::string JoinStrings(const ConstForwardIterator& begin,
                        const ConstForwardIterator& end,
                        const std::string& delimiter) {
  std::string output;
  JoinStrings(begin, end, delimiter, &output);
  return output;
}

template <class Container>
std::string JoinStrings(const Container& container,
                        const std::string& delimiter = " ") {
  return JoinStrings(container.begin(), container.end(), delimiter);
}

#endif  // STRUTIL_JOIN_STRINGS_H_
