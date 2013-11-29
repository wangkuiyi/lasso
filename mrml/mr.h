

//
#ifndef MRML_MR_H_
#define MRML_MR_H_

#include <string>

#include "mrml/mrml.h"
#include "sorted_buffer/sorted_buffer_iterator.h"

//-----------------------------------------------------------------------------
//
// In addition to the MRML_Mapper, MRML_Reducer API, MR_Mapper and
// MR_Reducer API provides the ability of large scale processing; in
// particular, without the limitation on the number of unique map
// output keys.
//
// From the perspective of API, the ability of incremental reduce is
// removed, and the API is therefore identical to the standard Google
// MapReduce API).
//
// *** Batch Reduction ***
//
// The initial design of MRML is to provide a way which makes it
// possible for map workers and reduce workers work simultaneously.
// The design was realized by MRML_Reducer class.  However, this
// design has a limit on the number of unique map output keys, which
// would becomes a servere problem in applications like parallel
// training of language models.  Therefore, we add MR_Redcuer, a
// reducer API which is identical to that published in Google papers.
//
// If you derive your reducer class from MR_Reducer, instead of
// MRML_Reducer, please remember to register it using
// REGISTER_MR_REDUCER instead of REGISTER_REDUCER.
//
//-----------------------------------------------------------------------------

// MR_Mapper is exactly MRML_Mapper.
typedef MRML_Mapper MR_Mapper;

typedef sorted_buffer::SortedBufferIterator ReduceInputIterator;

class MR_Reducer : public MRML_Reducer {
 public:
  virtual ~MR_Reducer() {}

  // The new API:
  virtual void Start() {}
  virtual void Reduce(const string& key, ReduceInputIterator* values) = 0;
  virtual void Flush() {}

 private:
  // Forbids the old API:
  virtual void* BeginReduce(const string&, const string&) { return NULL; }
  virtual void PartialReduce(const string&, const string&, void*) {}
  virtual void EndReduce(const string&, void*) {}
};

//-----------------------------------------------------------------------------
// Mapper/reducer registering and creating mechanism.
//-----------------------------------------------------------------------------

typedef MR_Mapper* (*MR_MapperCreator)();
typedef MR_Reducer* (*MR_ReducerCreator)();

class MR_MapperRegisterer {
 public:
  MR_MapperRegisterer(const string& class_name, MR_MapperCreator p);
};

class MR_ReducerRegisterer {
 public:
  MR_ReducerRegisterer(const string& class_name, MR_ReducerCreator p);
};

#define REGISTER_MR_MAPPER(mapper_name)                                 \
  MR_Mapper* mapper_name##_creator() { return new mapper_name; }        \
  MR_MapperRegisterer g_mapper_reg##mapper_name(#mapper_name,           \
                                                mapper_name##_creator)

#define REGISTER_MR_REDUCER(reducer_name)                               \
  MR_Reducer* reducer_name##_creator() { return new reducer_name; }     \
  MR_ReducerRegisterer g_reducer_reg##reducer_name(#reducer_name,       \
                                                   reducer_name##_creator)

#endif  // MRML_MR_H_
