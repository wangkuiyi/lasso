
//

//
#ifndef MRML_MRML_H_
#define MRML_MRML_H_

#include <string>
#include <sstream>
#include <vector>

using std::string;

namespace google {
namespace protobuf {
class Message;
}
}

//-----------------------------------------------------------------------------
//
// Currently supported input/output file format.  Refer to recordio.hh
// for more details with RecordIO format.
//
//-----------------------------------------------------------------------------
enum MRML_FileFormat {Text, RecordIO};

//-----------------------------------------------------------------------------
//
// Mapper class
//
// *** Basic API ***
//
// In MRML, for each map input shard file, a map worker (OS process)
// is created.  The worker creates an object of a derived-class of
// MRML_Mapper, then invokes its member functions in the following
// procedure:
//
//  1. Before processing the first key-value pair in the map input
//     shard, it invokes Start().  You may want to override Start() to
//     do shard-specific initializations.
//  2. For each key-value pair in the shard, it invokes Map().
//  3. After all map input pairs were processed, it invokes Flush().
//
// *** Multi-pass Map ***
//
// A unique feature of MRML (differs from Google MapReduce and Hadoop)
// is that above procedure may be repeated for multiple time, if the
// command line paramter --mrml_multipass_map is set with a positive
// value.  In this case, derived classes can invoke GetCurrentPass()
// to get the current (zero-based) pass id.
//
// *** Combiner ***
//
// By overriding Start() and Flush(), MRML programs can know map input
// shard boundary.  This is similar to Google MapReduce API, but
// differs from Hadoop API.  An advantage of knowing this boundary is
// that we can implement the combiner pattern within mapper as
// following procedure:
//
//  1. Start() sets up a data structure for save intermediate results.
//  2. Map() aggregates intermediate result into above data structure,
//     rather than sending it out using Output.
//  3. Flush() invokes Output and send out above data structure.
//
// For an example, please refer to class WordCountMapperWithCombiner
// defined wordcount.cc.
//
// *** Periodic Flush ***
//
// If command line option mrml_periodic_flush is set with a positive
// integer, the map worker invokes Flush() once every
// <mrml_periodic_flush> map input pairs were processed, and after all
// pairs were processed.  This allows limiting memory consumption in
// Start()/Flush()-based combining.
//
// *** Sharding ***
//
// As both Google MapReduce API and Hadoop API, programmers can
// specify to where a map output goes by overriding Shard().  In
// addition, similar to Google API (but differs from Hadoop API),
// programmers can also invoke OutputToShard() with a parameter
// specifying the target reduce shard.
//
// *** Output to All Shards ***
//
// A unique feature of MRML is OutputToAllShards(), which allows a map
// output goes to all shards.  Some machine learning algorithms (e.g.,
// AD-LDA) might find this API useful.
//
// *** NOTE ***
//
// OutputToShard and OutputToAllShards are forbidden in map-only mode.
//
//-----------------------------------------------------------------------------
class MRML_Mapper {
 public:
  virtual ~MRML_Mapper() {}

  virtual void Start() {}
  virtual void Map(const string& key, const string& value) = 0;
  virtual void Flush() {}
  virtual int Shard(const string& key, int num_reduce_shards);

 protected:
  virtual void Output(const string& key, const string& value);
  virtual void Output(const string& key,
                      const ::google::protobuf::Message& value_pb);

  virtual void OutputToShard(int reduce_shard,
                             const string& key, const string& value);
  virtual void OutputToShard(int reduce_shard,
                             const string& key,
                             const ::google::protobuf::Message& value_pb);

  virtual void OutputToAllShards(const string& key, const string& value);
  virtual void OutputToAllShards(const string& key,
                                 const ::google::protobuf::Message& value_pb);

  MRML_FileFormat GetInputFormat() const;
  MRML_FileFormat GetOutputFormat() const;
  int GetNumMapPasses() const;
  int GetNumReduceShards() const;
  const std::vector<string>& GetConfig() const;
  bool IsMapOnly() const;
  int GetCurrentPass() const;
};

//-----------------------------------------------------------------------------
//
// Reducer class (providing a MRML-specific API)
//
// *** Combiner ***
//
// MRML_Reducer has two member functions: Start() and Flush():
//
//  1. Before processing a reduce shard, invokes Start();
//  2. For each reduce input (consisting a key and one or more
//     values), invokes BeginReduce(), PartialReduce(), and
//     EndReduce().
//  3. After processing a reduce shard, invokes Flush();
//
// This is similar to MRML_Mapper, and allows the implementation of
// combiner pattern in reduce class.
//
// *** Incremental Reduce ***
//
// In standard MapReduce API, thre is a Reduce() function which takes
// all values associated with a key, and programmers override this
// function to process all these values as a whole.  However, in
// practice (both Google MapReduce and Hadoop), the access to these
// values are constrained to be through a forward-only iterator.  This
// constraint, in logic, is equivalent to _incremental reduce_.  In
// MRML, we represent incremental reduce by three member functions:
//
//  1. void* BeginReduce(key, value): Given the first value in a
//     reduce input, returns a pointer to intermediate reducing
//     result.
//
//  2. void PartialReduce(key, value, partial_result): For each of the
//     rest values (except for the first value) in current reduce
//     input, update intermediate reducing result.
//
//  3. void EndReduce(partial_result, output): When this function is
//     invoked, partial_result points the _final_ result.  This
//     function should output the final result into a string, which,
//     together with the key of the current reduce input, will be save
//     as a reduce output pair.
//
//-----------------------------------------------------------------------------
class MRML_Reducer {
 public:
  virtual ~MRML_Reducer() {}
  virtual void Start() {}
  virtual void* /*partial_result*/ BeginReduce(const string& key,
                                               const string& value) = 0;
  virtual void PartialReduce(const string& key,
                             const string& value,
                             void* partial_result) = 0;
  virtual void EndReduce(const string& key,
                         void* partial_result) = 0;
  virtual void Flush() {}

 protected:
  const std::vector<string>& GetConfig() const;
  MRML_FileFormat GetOutputFormat() const;

  virtual void Output(const string& key, const string& value);
};

//-----------------------------------------------------------------------------
//
// Predefined mappers and reducers
//
//-----------------------------------------------------------------------------

class IdentityMapper : public MRML_Mapper {
 public:
  virtual void Map(const string& key, const string& value) {
    Output(key, value);
  }
};

template <typename ValueType>
class SumReducer : public MRML_Reducer {
 public:
  virtual void* BeginReduce(const string& key, const string& value) {
    std::istringstream is(value);
    ValueType* count = new ValueType;
    is >> *count;
    return count;
  }
  virtual void PartialReduce(const string& key, const string& value,
                             void* partial_sum) {
    std::istringstream is(value);
    ValueType count = 0;
    is >> count;
    *static_cast<ValueType*>(partial_sum) += count;
  }
  virtual void EndReduce(const string& key, void* final_sum) {
    ValueType* p = static_cast<ValueType*>(final_sum);
    std::ostringstream os;
    os << *p;
    Output(key, os.str());
    delete p;
  }
};

class SumIntegerReducer : public SumReducer<int> {};
class SumFloatReducer : public SumReducer<float> {};
class SumDoubleReducer : public SumReducer<double> {};

class IdentityReducer : public MRML_Reducer {
 public:
  virtual void* BeginReduce(const string& key, const string& value) {
    Output(key, value);
    return NULL;
  }
  virtual void PartialReduce(const string& key, const string& value,
                             void* partial_result) {
    Output(key, value);
  }
  virtual void EndReduce(const string& key, void* result) {
  }
};

//-----------------------------------------------------------------------------
//
// MRML mapper/reducer registering mechanism.  Each user-defined
// mapper, say UserDefinedMapper, must be registerred using
// REGISTER_MAPPER(UserDefinedMapper); in a .cc file.  Similarly, Each
// user-defined reducer must be registered using
// REGISTER_REDUCER(UserDefinedReducer); This allows the MRML runtime
// (in particualr, mrml-main.cc) to create instances of
// mappers/reducers according to their names given as command line
// parameters (--mrml_mapper_class and --mrml_reducer_class).
//
// If a reducer class is derived from MR_Reducer, instead of MRML_Reducer,
//
//-----------------------------------------------------------------------------

typedef MRML_Mapper* (*MRML_MapperCreator)();
typedef MRML_Reducer* (*MRML_ReducerCreator)();

class MRML_MapperRegisterer {
 public:
  MRML_MapperRegisterer(const string& class_name, MRML_MapperCreator p);
};

class MRML_ReducerRegisterer {
 public:
  MRML_ReducerRegisterer(const string& class_name, MRML_ReducerCreator p);
};

#define REGISTER_MAPPER(mapper_name)                                    \
  MRML_Mapper* mapper_name##_creator() { return new mapper_name; }      \
  MRML_MapperRegisterer g_mapper_reg##mapper_name(#mapper_name,         \
                                                  mapper_name##_creator)

#define REGISTER_REDUCER(reducer_name)                                  \
  MRML_Reducer* reducer_name##_creator() { return new reducer_name; }   \
  MRML_ReducerRegisterer g_reducer_reg##reducer_name(#reducer_name,     \
                                                     reducer_name##_creator)

#endif  // MRML_MRML_H_
