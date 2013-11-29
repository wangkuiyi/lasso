

//
#include <mpi.h>
#include <stdio.h>
#include <sys/utsname.h>                // For uname

#include <map>
#include <new>
#include <set>
#include <string>

#include "boost/program_options/option.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/filesystem.hpp"

#include "gflags/gflags.h"

#include "base/common.h"
#include "sorted_buffer/sorted_buffer.h"
#include "sorted_buffer/sorted_buffer_iterator.h"
#include "strutil/stringprintf.h"

#include "hash/simple_hash.h"
#include "mrml/mr.h"
#include "mrml/mrml_reader.h"
#include "mrml/mrml_recordio.h"
#include "mrml/mrml.pb.h"

using sorted_buffer::SortedBuffer;
using sorted_buffer::SortedBufferIteratorImpl;
using std::map;
using std::string;

typedef ::google::protobuf::Message ProtoMessage;

//-----------------------------------------------------------------------------
// Error message and other constant text.
//-----------------------------------------------------------------------------
static const char* kForbidOutputToShardInMapOnlyMode =
    "OutputToShard and OutputToAllShards are forbidden in map-only mode";

//-----------------------------------------------------------------------------
// Constants used by MRML:
//-----------------------------------------------------------------------------

const int kMapOutputTag = 1;
const int kDefaultMapOutputSize = 32 * 1024 * 1024;  // 32 MB
const int kDefaultReduceInputBufferSize = 256;       // 256 MB
const int kMaxInputLineLength = 16 * 1024;           // 16 KB
//-----------------------------------------------------------------------------
// MRML mapper and reducer creators
//-----------------------------------------------------------------------------

typedef map<string, MRML_MapperCreator>  MRMLMapperCreatorRegistory;
typedef map<string, MRML_ReducerCreator> MRMLReducerCreatorRegistory;

MRMLMapperCreatorRegistory& GetMRMLMapperCreators() {
  static MRMLMapperCreatorRegistory creators;
  return creators;
}
MRMLReducerCreatorRegistory& GetMRMLReducerCreators() {
  static MRMLReducerCreatorRegistory creators;
  return creators;
}

MRML_MapperRegisterer::MRML_MapperRegisterer(const string& class_name,
                                             MRML_MapperCreator creator) {
  GetMRMLMapperCreators()[class_name] = creator;
}
MRML_ReducerRegisterer::MRML_ReducerRegisterer(const string& class_name,
                                               MRML_ReducerCreator creator) {
  GetMRMLReducerCreators()[class_name] = creator;
}

MRML_Mapper* MRML_CreateMapper(const string& mapper_name) {
  MRMLMapperCreatorRegistory::iterator iter =
      GetMRMLMapperCreators().find(mapper_name);
  return (iter == GetMRMLMapperCreators().end()) ? NULL : (*(iter->second))();
}

MRML_Reducer* MRML_CreateReducer(const string& reducer_name) {
  MRMLReducerCreatorRegistory::iterator iter =
      GetMRMLReducerCreators().find(reducer_name);
  return (iter == GetMRMLReducerCreators().end()) ? NULL : (*(iter->second))();
}

//-----------------------------------------------------------------------------
// MR mapper and reducer creators
//-----------------------------------------------------------------------------

typedef map<string, MR_MapperCreator>   MRMapperCreatorRegistory;
typedef map<string, MR_ReducerCreator>  MRReducerCreatorRegistory;

MRMapperCreatorRegistory& GetMRMapperCreators() {
  static MRMapperCreatorRegistory creators;
  return creators;
}

MRReducerCreatorRegistory& GetMRReducerCreators() {
  static MRReducerCreatorRegistory creators;
  return creators;
}

MR_MapperRegisterer::MR_MapperRegisterer(const string& class_name,
                                         MR_MapperCreator creator) {
  GetMRMapperCreators()[class_name] = creator;
}

MR_ReducerRegisterer::MR_ReducerRegisterer(const string& class_name,
                                           MR_ReducerCreator creator) {
  GetMRReducerCreators()[class_name] = creator;
}

MR_Mapper* MR_CreateMapper(const string& reducer_name) {
  MRMapperCreatorRegistory::iterator iter =
      GetMRMapperCreators().find(reducer_name);
  return (iter == GetMRMapperCreators().end()) ? NULL : (*(iter->second))();
}

MR_Reducer* MR_CreateReducer(const string& reducer_name) {
  MRReducerCreatorRegistory::iterator iter =
      GetMRReducerCreators().find(reducer_name);
  return (iter == GetMRReducerCreators().end()) ? NULL : (*(iter->second))();
}

//-----------------------------------------------------------------------------
// Global variables of MRML:
//-----------------------------------------------------------------------------

static MRML_Mapper* g_mapper = NULL;
static MRML_Reducer* g_reducer = NULL;

//-----------------------------------------------------------------------------
// Command line flags supported by MRML:
//-----------------------------------------------------------------------------

DEFINE_bool(
    mrml_map_only, false,
    "In map-only mapreduce tasks, there is no reduce workers.");
DEFINE_int32(mrml_num_map_workers, 0,
             "The number of map workers.");
DEFINE_int32(mrml_num_reduce_workers, 0,
             "The number of reduce workers");
DEFINE_string(mrml_input_filebase, "",
              "Specify input filebase. Map worker id reads from "
              "<mrml_input_filebase>-000id-of-00num.");
DEFINE_string(mrml_output_filebase, "",
              "Specify output filebase. Reduce worker id writes to "
              "<mrml_output_filebase>-000id-of-00num.");
DEFINE_string(mrml_input_format, "",
              "Input file format: which can be \"text\" or \"recordio\".");
DEFINE_string(mrml_output_format, "",
              "Output file format, which can be \"text\" or \"recordio\".");
DEFINE_string(mrml_mapper_class, "",
              "Specify the mapper class name, which must have been registered "
              "using REGISTER_MAPPER or REGISTER_MR_MAPPER in your .cc file.");
DEFINE_string(mrml_reducer_class, "",
              "Specify the reducer class name, which must have ben registered "
              "using REGISTER_REDUCER in your .cc file.");
DEFINE_string(mrml_log_filebase, "",
              "Map workers log into <mrml_log_filebase>-mapper-*, whereas "
              "Reduce workers log into <mrml_log_filebase>-reducer-* ");
DEFINE_int32(mrml_multipass_map, 1,
             "By default, a map worker scans and processes a map input shard "
             "for one pass.  If this flag is set with a positive value, the "
             "worker can go over the input shard for more than one pass.  "
             "GetCurrentPass() returns the current (zero-based) pass.");
DEFINE_int32(mrml_max_map_output_size, kDefaultMapOutputSize,
             "By default, the max size of a map output is 32MB.  Use this "
             " option to specify a smaller or larger buffer size.");
DEFINE_bool(mrml_batch_reduction, false,
            "MRML uses by default an efficient incremental reduction "
            "solution, but if there is a large number of unique map output "
            "keys, it is necessary to set this flag and use traditional batch "
            "reduction.");
DEFINE_string(mrml_reduce_input_buffer_filebase, "",
              "The filebase of disk swap files used in batch reduction.");
DEFINE_int32(mrml_reduce_input_buffer_size, kDefaultReduceInputBufferSize,
             "The size of each reduce input buffer swap file in MB.");

//-----------------------------------------------------------------------------
// Map-only output:
//-----------------------------------------------------------------------------
FILE* g_map_only_output;
FILE* g_reduce_output;

//-----------------------------------------------------------------------------
// Map-output counter:
//
// MRML_Mapper::Output and MRML_Mapper::OutputToShard will increase
// this counter once per invocation.  MRML_Mapper::OutputToAllShards
// increases this counter by the number of reduce workers.
// ----------------------------------------------------------------------------
static int g_count_map_output = 0;

//-----------------------------------------------------------------------------
// Count map pass
//-----------------------------------------------------------------------------
static int g_current_map_pass = 0;

//-----------------------------------------------------------------------------
// Command line flags left after MRML_Initialize doing confg options parsing.
//-----------------------------------------------------------------------------

std::vector<string> g_cmdline_args;

//-----------------------------------------------------------------------------
// The buffer used by a reduce worker process to recieve a map output.
//-----------------------------------------------------------------------------
char* g_map_output_recieve_buffer = NULL;

//-----------------------------------------------------------------------------
// MRML forward declarations:
//-----------------------------------------------------------------------------

void MRML_InitializeLogDestinations();

//-----------------------------------------------------------------------------
// MRML implementation:
//-----------------------------------------------------------------------------

bool MRML_Initialize(int argc, char** argv) {
  // Initialize MPI.
  MPI_Init(&argc, &argv);

  // Parse command line flags, leaving argc unchanged, but rearrange
  // the arguments in argv so that the flags are all at the beginning.
  google::ParseCommandLineFlags(&argc, &argv, false);

  // Initialize log and set log destination file.
  MRML_InitializeLogDestinations();

  // Collect unparsed options into g_cmdline_args, which may be parsed
  // by mappers and reducers for application-specific options.
  namespace po = boost::program_options;
  try {
    po::options_description desc("MRML program options");
    po::parsed_options parsed = po::command_line_parser(argc, argv).
                                options(desc).allow_unregistered().run();
    po::variables_map vm;
    po::store(parsed, vm);
    po::notify(vm);
    g_cmdline_args =
        po::collect_unrecognized(parsed.options, po::include_positional);
    LOG(INFO) << "g_cmdline_args size = " << g_cmdline_args.size();
  } catch(const po::error& e) {
    LOG(ERROR) << "Error in parsing command line options: " << e.what();
    return false;
  }

  // Check the number of workers. Be caucious with FLAGS_mrml_map_only.
  int num_workers = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &num_workers);
  if (FLAGS_mrml_map_only &&
      (num_workers != FLAGS_mrml_num_map_workers ||
       FLAGS_mrml_num_reduce_workers != 0)) {
    LOG(FATAL) << "In case of map-only, num_map_workers ("
               << FLAGS_mrml_num_map_workers
               << ") must equal to num_workers (" << num_workers
               << "), and num_reduce_workers ("
               << FLAGS_mrml_num_reduce_workers
               << ") must be 0.";
  }
  if (!FLAGS_mrml_map_only &&
      (num_workers !=
       FLAGS_mrml_num_map_workers + FLAGS_mrml_num_reduce_workers)) {
    LOG(FATAL) << "The sum of num_map_workers (" << FLAGS_mrml_num_map_workers
               << ") and num_reduce_workers (" << FLAGS_mrml_num_reduce_workers
               << ") does not equal to num_workers (" << num_workers
               << ").";
  }

  // General flag validity checking.
  CHECK(!FLAGS_mrml_input_filebase.empty());
  CHECK(!FLAGS_mrml_output_filebase.empty());
  CHECK_LT(0, FLAGS_mrml_multipass_map);
  CHECK_LT(0, FLAGS_mrml_max_map_output_size);

  // Check flags related with batch reduction.
  if (FLAGS_mrml_batch_reduction && !FLAGS_mrml_map_only) {
    CHECK(!FLAGS_mrml_reduce_input_buffer_filebase.empty());
    if (boost::filesystem::exists(SortedBuffer::SortedFilename(
            FLAGS_mrml_reduce_input_buffer_filebase, 0))) {
      LOG(FATAL) << "Please delete existing reduce input buffer files: "
                 << FLAGS_mrml_reduce_input_buffer_filebase << "* ";
    }
    CHECK_LE(1, FLAGS_mrml_reduce_input_buffer_size);    // 1 MB at least
    CHECK_GE(2 * 1024 * 1024,
             FLAGS_mrml_reduce_input_buffer_size);       // 2TB at most
    FLAGS_mrml_reduce_input_buffer_size *= 1024 * 1024;  // unit in MB.
  }

  // Set input file format.
  if (FLAGS_mrml_input_format != "text" &&
      FLAGS_mrml_input_format != "recordio") {
    FLAGS_mrml_input_format = "text";
    LOG(ERROR) << "Unknown input_format: " << FLAGS_mrml_input_format
               << ". Use the default format: text";
  }

  // Set output file format.
  if (FLAGS_mrml_output_format != "text" &&
      FLAGS_mrml_output_format != "recordio") {
    FLAGS_mrml_output_format = "text";
    LOG(ERROR) << "Unknown output_format: " << FLAGS_mrml_output_format
               << ". Use the default format: text";
  }

  // Create mapper instance.
  g_mapper = FLAGS_mrml_batch_reduction ?
             MR_CreateMapper(FLAGS_mrml_mapper_class) :
             MRML_CreateMapper(FLAGS_mrml_mapper_class);
  if (g_mapper == NULL) {
    LOG(FATAL) << "Cannot create: " << FLAGS_mrml_mapper_class;
  }

  // Create reducer instance (if not map-only mode).
  if (!FLAGS_mrml_map_only) {
    g_reducer = FLAGS_mrml_batch_reduction ?
                MR_CreateReducer(FLAGS_mrml_reducer_class) :
                MRML_CreateReducer(FLAGS_mrml_reducer_class);
    if (g_reducer == NULL) {
      LOG(FATAL) << "Cannot create: " << FLAGS_mrml_reducer_class;
    }
  }

  return true;
}

void MRML_Finalize() {
  if (g_mapper != NULL) {
    delete g_mapper;
    g_mapper = NULL;
  }
  if (g_reducer != NULL) {
    delete g_reducer;
    g_reducer = NULL;
  }
  // After all, finalize MPI.
  MPI_Finalize();
}

bool MRML_AmIMapWorker() {
  int worker_index = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &worker_index);
  return worker_index < FLAGS_mrml_num_map_workers;
}

int MRML_MapWorkerId() {
  int worker_index = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &worker_index);
  CHECK(worker_index < FLAGS_mrml_num_map_workers);
  return worker_index;
}

int MRML_ReduceWorkerId() {
  int worker_index = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &worker_index);
  CHECK(worker_index >= FLAGS_mrml_num_map_workers);
  return worker_index - FLAGS_mrml_num_map_workers;
}

string MRML_InputFilename() {
  return StringPrintf("%s-%05d-of-%05d",
                      FLAGS_mrml_input_filebase.c_str(),
                      MRML_MapWorkerId(),
                      FLAGS_mrml_num_map_workers);
}

string MRML_OutputFilename() {
  return StringPrintf(
      "%s-%05d-of-%05d",
      FLAGS_mrml_output_filebase.c_str(),
      FLAGS_mrml_map_only ? MRML_MapWorkerId() : MRML_ReduceWorkerId(),
      (FLAGS_mrml_map_only ?
       FLAGS_mrml_num_map_workers : FLAGS_mrml_num_reduce_workers));
}

string MRML_GetHostName() {
  struct utsname buf;
  if (0 != uname(&buf)) {
    *buf.nodename = '\0';
  }
  return string(buf.nodename);
}

string MRML_GetUserName() {
  const char* username = getenv("USER");
  return username != NULL ? username : getenv("USERNAME");
}

void MRML_InitializeLogDestinations() {
  // Get filename_prefix = FLAGS_mrml_log_filebase + worker_type_and_id +
  //                           node_name + username + log_type
  CHECK(!FLAGS_mrml_log_filebase.empty());
  string filename_prefix = StringPrintf(
      "%s-%s-%05d-of-%05d.%s.%s",
      FLAGS_mrml_log_filebase.c_str(),
      (MRML_AmIMapWorker() ? "mapper" : "reducer"),
      (MRML_AmIMapWorker() ? MRML_MapWorkerId() : MRML_ReduceWorkerId()),
      (MRML_AmIMapWorker() ?
       FLAGS_mrml_num_map_workers : FLAGS_mrml_num_reduce_workers),
      MRML_GetHostName().c_str(),
      MRML_GetUserName().c_str());

  InitializeLogger(filename_prefix + ".INFO",
                   filename_prefix + ".WARN",
                   filename_prefix + ".ERRO");
}

string MRML_ReduceInputBufferFilebase() {
  CHECK(!MRML_AmIMapWorker());       // This must be a reduce worker.
  return StringPrintf("%s-reducer-%05d-of-%05d",
                      FLAGS_mrml_reduce_input_buffer_filebase.c_str(),
                      MRML_ReduceWorkerId(),
                      FLAGS_mrml_num_reduce_workers);
}

void MRML_MapWorkerNotifyFinished() {
  // For map-only tasks, no need to notify reducer workers that a
  // mapper worker has finished its work.
  if (FLAGS_mrml_map_only)
    return;

  MapOutput mo;
  mo.set_map_worker(MRML_MapWorkerId());
  string smo;
  mo.SerializeToString(&smo);

  for (int r = FLAGS_mrml_num_map_workers;
       r < FLAGS_mrml_num_map_workers + FLAGS_mrml_num_reduce_workers;
       ++r) {
    MPI_Send(const_cast<char*>(smo.data()), smo.size(), MPI_CHAR, r,
             kMapOutputTag, MPI_COMM_WORLD);
  }
}

void MRML_WriteText(FILE* output_stream,
                    const string& key, const string& value) {
  fprintf(output_stream, "%s\n", value.c_str());
}

int MRML_Mapper::Shard(const string& key, int num_reduce_workers) {
  return JSHash(key) % num_reduce_workers;
}

void MRML_Mapper::Output(const string& key,
                         const string& value) {
  // TODO(yiwang): Not to use KeyValuePair in transmitting map output,
  // thus to save the cost of duplicated key/value.
  if (IsMapOnly()) {
    if (FLAGS_mrml_output_format == "text") {
      MRML_WriteText(g_map_only_output, key, value);
    } else if (FLAGS_mrml_output_format == "recordio") {
      MRML_WriteRecord(g_map_only_output, key, value);
    }
  } else {
    MapOutput mo;
    mo.set_key(key);
    mo.set_value(value);
    string smo;
    mo.SerializeToString(&smo);

    MPI_Send(const_cast<char*>(smo.data()), smo.size(), MPI_CHAR,
             FLAGS_mrml_num_map_workers +
             g_mapper->Shard(key, FLAGS_mrml_num_reduce_workers),
             kMapOutputTag, MPI_COMM_WORLD);
  }
  ++g_count_map_output;
}

void MRML_Mapper::OutputToShard(int reduce_shard,
                                const string& key,
                                const string& value) {
  // TODO(yiwang): Not to use KeyValuePair in transmitting map output,
  // thus to save the cost of duplicated key/value.
  if (IsMapOnly()) {
    LOG(FATAL) << kForbidOutputToShardInMapOnlyMode;
  } else {
    CHECK_GE(reduce_shard, 0);
    CHECK_LT(reduce_shard, FLAGS_mrml_num_reduce_workers);

    MapOutput mo;
    mo.set_key(key);
    mo.set_value(value);
    string smo;
    mo.SerializeToString(&smo);

    MPI_Send(const_cast<char*>(smo.data()), smo.size(), MPI_CHAR,
             FLAGS_mrml_num_map_workers + reduce_shard,
             kMapOutputTag, MPI_COMM_WORLD);
  }
  ++g_count_map_output;
}

void MRML_Mapper::Output(const string& key,
                         const ProtoMessage& value_pb) {
  if (IsMapOnly()) {
    string value;
    value_pb.SerializeToString(&value);

    if (FLAGS_mrml_output_format == "text") {
      MRML_WriteText(g_map_only_output, key, value);
    } else if (FLAGS_mrml_output_format == "recordio") {
      MRML_WriteRecord(g_map_only_output, key, value);
    }
  } else {
    MapOutput mo;
    mo.set_key(key);
    string* value = mo.mutable_value();
    value_pb.SerializeToString(value);
    string smo;
    mo.SerializeToString(&smo);

    MPI_Send(const_cast<char*>(smo.data()), smo.size(), MPI_CHAR,
             FLAGS_mrml_num_map_workers +
             g_mapper->Shard(key, FLAGS_mrml_num_reduce_workers),
             kMapOutputTag, MPI_COMM_WORLD);
  }
  ++g_count_map_output;
}

void MRML_Mapper::OutputToShard(int reduce_shard,
                                const string& key,
                                const ProtoMessage& value_pb) {
  if (IsMapOnly()) {
    LOG(FATAL) << kForbidOutputToShardInMapOnlyMode;
  } else {
    CHECK_GE(reduce_shard, 0);
    CHECK_LT(reduce_shard, FLAGS_mrml_num_reduce_workers);

    MapOutput mo;
    mo.set_key(key);
    string* value = mo.mutable_value();
    value_pb.SerializeToString(value);
    string smo;
    mo.SerializeToString(&smo);

    MPI_Send(const_cast<char*>(smo.data()), smo.size(), MPI_CHAR,
             FLAGS_mrml_num_map_workers + reduce_shard,
             kMapOutputTag, MPI_COMM_WORLD);
  }
  ++g_count_map_output;
}

void MRML_Mapper::OutputToAllShards(const string& key, const string& value) {
  if (IsMapOnly()) {
    LOG(FATAL) << kForbidOutputToShardInMapOnlyMode;
  } else {
    MapOutput mo;
    mo.set_key(key);
    mo.set_value(value);
    string smo;
    mo.SerializeToString(&smo);

    for (int r = 0; r < FLAGS_mrml_num_reduce_workers; ++r) {
      MPI_Send(const_cast<char*>(smo.data()), smo.size(), MPI_CHAR,
               FLAGS_mrml_num_map_workers + r,
               kMapOutputTag, MPI_COMM_WORLD);
    }
  }
  g_count_map_output += GetNumReduceShards();
}

void MRML_Mapper::OutputToAllShards(const string& key,
                                    const ProtoMessage& value_pb) {
  if (IsMapOnly()) {
    LOG(FATAL) << kForbidOutputToShardInMapOnlyMode;
  } else {
    MapOutput mo;
    mo.set_key(key);
    string* value = mo.mutable_value();
    value_pb.SerializeToString(value);
    string smo;
    mo.SerializeToString(&smo);

    for (int r = 0; r < FLAGS_mrml_num_reduce_workers; ++r) {
      MPI_Send(const_cast<char*>(smo.data()), smo.size(), MPI_CHAR,
               FLAGS_mrml_num_map_workers + r,
               kMapOutputTag, MPI_COMM_WORLD);
    }
  }
  g_count_map_output += GetNumReduceShards();
}

void MRML_MapWork() {
  // In map-only mode, map workers take the responsibility to create
  // the output shard file.
  if (FLAGS_mrml_map_only) {
    LOG(INFO) << "As in a map-only task, I also write to "
              << MRML_OutputFilename();
    g_map_only_output = fopen(MRML_OutputFilename().c_str(), "w+");
    if (g_map_only_output == NULL) {
      LOG(FATAL) << "Cannot open reduce output shard file: "
                 << MRML_OutputFilename();
    }
  }

  // Clear counters.
  g_count_map_output = 0;
  int count_map_input = 0;
  int count_flush = 0;

  // Do one or more passes of mapping.
  for (int pass = 0; pass < FLAGS_mrml_multipass_map; ++pass) {
    g_current_map_pass = pass;  // so MRML_Mapper::GetCurrentPass() works.
    g_mapper->Start();

    LOG(INFO) << "I read from " << MRML_InputFilename() << " in pass " << pass;
    MRML_Reader* reader =
        ((FLAGS_mrml_input_format == "text") ?
         reinterpret_cast<MRML_Reader*>(
             new MRML_TextReader(MRML_InputFilename(), kMaxInputLineLength)) :
         reinterpret_cast<MRML_Reader*>(
             new MRML_RecordReader(MRML_InputFilename())));
    string key, value;

    while (true) {
      if (!reader->Read(&key, &value)) {
        break;
      }

      g_mapper->Map(key, value);
      ++count_map_input;

      if ((count_map_input % 1000) == 0) {
        LOG(INFO) << "Processed " << count_map_input << " records.";
      }
    }

    g_mapper->Flush();
    ++count_flush;
    delete reader;
  }

  LOG(INFO) << "Finished mapping input shard: " << MRML_InputFilename() << "\n"
            << " count_map_input = " << count_map_input << "\n"
            << " count_flush = " << count_flush << "\n"
            << " count_map_output = " << g_count_map_output;

  // Important to tell reduce workers to terminate.
  MRML_MapWorkerNotifyFinished();
}

void MRML_ReduceWork() {
  LOG(INFO) << "I work in "
            << (FLAGS_mrml_batch_reduction ? "batch " : "incremental ")
            << "reduction mode";
  LOG(INFO) << "I write to " << MRML_OutputFilename();

  g_reduce_output = fopen(MRML_OutputFilename().c_str(), "w+");
  if (g_reduce_output == NULL) {
    LOG(FATAL) << "Cannot open reduce output shard file: "
               << MRML_OutputFilename();
  }

  g_reducer->Start();

  // Once a map worker finished processing its input shard, it will
  // send a `finished' message to all reduce workers.  A reduce worker
  // collects such messages into finished_map_worker. After `finished'
  // messages from all map workers are collected, the reduce worker quits.
  std::set<int> finished_map_workers;

  // In order to implement the classical MapReduce API, which defines
  // reduce operation in a ``batch'' way -- reduce is invoked after
  // all reduce values were collected for a map output key.  we
  // employes Berkeley DB to sort and store map outputs arrived in
  // this reduce worker.  Berkeley DB is in response to keep a small
  // memory footprint and does external sort using disk.
  SortedBuffer* reduce_input_buffer = NULL;
  //
  // MRML supports in addition ``incremental'' reduction, where
  // reduce() accepts an intermediate reduce result (represented by a
  // void*, and is NULL for the first value in a reduce input comes)
  // and a reduce value.  It should update the intermediate result
  // using the value.
  typedef map<string, void*> PartialReduceResults;
  PartialReduceResults* partial_reduce_results = NULL;


  // Initialize partial reduce results, or reduce input buffer.
  if (!FLAGS_mrml_batch_reduction) {
    partial_reduce_results = new PartialReduceResults;
  } else {
    try {
      LOG(INFO) << "Creating reduce input buffer ... filebase = "
                << MRML_ReduceInputBufferFilebase()
                << ", buffer file size cap = "
                << FLAGS_mrml_reduce_input_buffer_size;
      reduce_input_buffer = new SortedBuffer(
          MRML_ReduceInputBufferFilebase(),
          FLAGS_mrml_reduce_input_buffer_size);
    } catch(const std::bad_alloc&) {
      LOG(FATAL) << "Insufficient memory for creating reduce input buffer.";
    }
    LOG(INFO) << "Succeeded creating reduce input buffer.";
  }

  // Allocate map outputs recieving buffer.
  LOG(INFO) << "Creating map output recieving buffer ...";
  CHECK_LT(0, FLAGS_mrml_max_map_output_size);
  try {
    g_map_output_recieve_buffer = new char[FLAGS_mrml_max_map_output_size];
  } catch(const std::bad_alloc&) {
    LOG(FATAL) << "Insufficient memory for map output buffer";
  }
  LOG(INFO) << "Succeeded creating map output recieving buffer.";

  // Loop over map outputs arrived in this reduce worker.
  LOG(INFO) << "Start recieving and processing arriving map outputs ...";
  MPI_Status status;
  int32 recieved_bytes = 0;
  int32 count_reduce = 0;
  int32 count_map_output = 0;

  while (true) {
    MPI_Recv(g_map_output_recieve_buffer,
             FLAGS_mrml_max_map_output_size,
             MPI_CHAR,
             MPI_ANY_SOURCE,
             kMapOutputTag, MPI_COMM_WORLD, &status);

    MPI_Get_count(&status, MPI_CHAR, &recieved_bytes);

    if (recieved_bytes >= FLAGS_mrml_max_map_output_size) {
      LOG(FATAL) << "MPI_Recieving a proto message with size (at least) "
                 << recieved_bytes
                 << ", which >= FLAGS_mrml_max_map_output_size ("
                 << FLAGS_mrml_max_map_output_size << ")."
                 << "You can modify FLAGS_mrml_max_map_output_size defined in "
                 << __FILE__;
    }

    MapOutput mo;
    CHECK(mo.ParseFromArray(g_map_output_recieve_buffer,
                            recieved_bytes));

    if (mo.has_map_worker()) {
      finished_map_workers.insert(mo.map_worker());
      if (finished_map_workers.size() >= FLAGS_mrml_num_map_workers) {
        LOG(INFO) << "Finished recieving and procesing arriving map outputs";
        break;  // Break the while (true) loop.
      }
    } else {
      CHECK(mo.has_key());
      CHECK(mo.has_value());
      ++count_map_output;

      if (!FLAGS_mrml_batch_reduction) {
        // Begin a new reduce, which insert a partial result, or does
        // partial reduce, which updates a partial result.
        PartialReduceResults::iterator iter =
            partial_reduce_results->find(mo.key());
        if (iter == partial_reduce_results->end()) {
          (*partial_reduce_results)[mo.key()] =
              g_reducer->BeginReduce(mo.key(), mo.value());
        } else {
          g_reducer->PartialReduce(mo.key(), mo.value(), iter->second);
        }

        if ((count_map_output % 5000) == 0) {
          LOG(INFO) << "Processed " << count_map_output << " map outputs.";
        }
      } else {
        // Insert the map output into disk buffer.
        reduce_input_buffer->Insert(mo.key(), mo.value());
      }
    }
  }

  if (g_map_output_recieve_buffer != NULL) {
    delete g_map_output_recieve_buffer;
    g_map_output_recieve_buffer = NULL;
  }

  // Invoke EndReduce in incremental reduction mode, or invoke Reduce
  // in batch reduction mode.
  if (!FLAGS_mrml_batch_reduction) {
    LOG(INFO) << "Finalizing incremental reduction ...";
    for (PartialReduceResults::const_iterator iter =
             partial_reduce_results->begin();
         iter != partial_reduce_results->end(); ++iter) {
      g_reducer->EndReduce(iter->first, iter->second);
      // Note: deleting of iter->second must be performed by the user
      // program in EndReduce, because mrml.cc does not know the type of
      // ReducePartialResult defined by the user program.
      ++count_reduce;
    }
    LOG(INFO) << "Succeeded finalizing incremental reduction.";
  } else {
    reduce_input_buffer->Flush();
    LOG(INFO) << "Start batch reduction ...";
    SortedBufferIteratorImpl* reduce_input_iterator =
        reinterpret_cast<SortedBufferIteratorImpl*>(
            reduce_input_buffer->CreateIterator());
    MR_Reducer* mr_reducer = reinterpret_cast<MR_Reducer*>(g_reducer);
    for (count_reduce = 0; !(reduce_input_iterator->FinishedAll());
         reduce_input_iterator->NextKey(), ++count_reduce) {
      mr_reducer->Reduce(reduce_input_iterator->key(), reduce_input_iterator);
      if (count_reduce > 0 && (count_reduce % 5000) == 0) {
        LOG(INFO) << "Invoked " << count_reduce << " reduce()s.";
      }
    }
    LOG(INFO) << "Finished batch reduction.";
    if (reduce_input_iterator != NULL) {
      delete reduce_input_iterator;
    }
  }

  LOG(INFO) << " count_reduce = " << count_reduce << "\n"
            << " count_map_output = " << count_map_output << "\n";

  // Free partial_reduce_results or reduce_inputs
  if (!FLAGS_mrml_batch_reduction) {
    LOG(INFO) << "Releasing partial_reduce_results ...";
    delete partial_reduce_results;
    partial_reduce_results = NULL;
    LOG(INFO) << "Finished releasing_reduce_results.";
  } else {
    LOG(INFO) << "Removing reduce input files ...";
    reduce_input_buffer->RemoveBufferFiles();
    delete reduce_input_buffer;
    LOG(INFO) << "Finished removing reduce input files.";
  }

  g_reducer->Flush();
}

void MRML_Reducer::Output(const string& key, const string& value) {
  if (FLAGS_mrml_output_format == "text") {
    MRML_WriteText(g_reduce_output, key, value);
  } else if (FLAGS_mrml_output_format == "recordio") {
    MRML_WriteRecord(g_reduce_output, key, value);
  }
}

int MRML_Mapper::GetNumReduceShards() const {
  return FLAGS_mrml_map_only ? 0 : FLAGS_mrml_num_reduce_workers;
}

const std::vector<string>& MRML_Mapper::GetConfig() const {
  return g_cmdline_args;
}

const std::vector<string>& MRML_Reducer::GetConfig() const {
  return g_cmdline_args;
}

bool MRML_Mapper::IsMapOnly() const {
  return FLAGS_mrml_map_only;
}

int MRML_Mapper::GetCurrentPass() const {
  return g_current_map_pass;
}

int MRML_Mapper::GetNumMapPasses() const {
  return FLAGS_mrml_multipass_map;
}

MRML_FileFormat MRML_Mapper::GetInputFormat() const {
  return FLAGS_mrml_input_format == "text" ? Text : RecordIO;
}

MRML_FileFormat MRML_Mapper::GetOutputFormat() const {
  if (!FLAGS_mrml_map_only) {
    LOG(WARNING) << "Mapper checked output format when not in map-only mode.";
  }
  return FLAGS_mrml_output_format == "text" ? Text : RecordIO;
}

MRML_FileFormat MRML_Reducer::GetOutputFormat() const {
  return FLAGS_mrml_output_format == "text" ? Text : RecordIO;
}

//-----------------------------------------------------------------------------
// Pre-defined mappers and reducers
//-----------------------------------------------------------------------------

REGISTER_MAPPER(IdentityMapper);

REGISTER_REDUCER(IdentityReducer);
REGISTER_REDUCER(SumIntegerReducer);
REGISTER_REDUCER(SumFloatReducer);
REGISTER_REDUCER(SumDoubleReducer);
