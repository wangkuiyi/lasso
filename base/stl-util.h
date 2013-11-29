

//
// This file contains facilities that enhance the STL.
//
#ifndef BASE_STL_UTIL_H_
#define BASE_STL_UTIL_H_

// Delete elements (in pointer type) in a STL container like vector,
// list, and deque.
template <class Container>
void STLDeleteElementsAndClear(Container* c) {
  for (typename Container::iterator iter = c->begin();
       iter != c->end(); ++iter) {
    if (*iter != NULL) {
      delete *iter;
    }
  }
  c->clear();
}

// Delete elements (in pointer type) in a STL associative container
// like map and hash_map.
template <class AssocContainer>
void STLDeleteValuesAndClear(AssocContainer* c) {
  for (typename AssocContainer::iterator iter = c->begin();
       iter != c->end(); ++iter) {
    if (iter->second != NULL) {
      delete iter->second;
    }
  }
  c->clear();
}

#endif  // BASE_STL_UTIL_H_
