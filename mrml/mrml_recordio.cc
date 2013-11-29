

//
#include "mrml/mrml_recordio.h"

#include <string>

#include "base/common.h"
#include "mrml/mrml.pb.h"
#include "mrml/mrml_filesystem.h"

using std::string;

typedef ::google::protobuf::Message ProtoMessage;

const int kMRMLRecordIOMaxRecordSize = 64*1024*1024;  // 64 MB

//-----------------------------------------------------------------------------
// We are going to support to kinds of input streams: (1) FILE*, and
// (2) MRMLFS_File.  However, these two streams have difference
// interface.  Here we define the adaptor to unify their interfaces.
//-----------------------------------------------------------------------------
template <class StreamType>
class IOStreamAdaptor {
 public:
  explicit IOStreamAdaptor(StreamType* is) : is_(is) {}
  bool Read(char* buffer, size_t size);
  bool Write(char* buffer, size_t size);
 protected:
  StreamType* is_;
};

template <>
bool IOStreamAdaptor<FILE>::Read(char* buffer, size_t size) {
  fread(buffer, size, 1, is_);
  return (feof(is_) == 0) && (ferror(is_) == 0);
}

template <>
bool IOStreamAdaptor<MRMLFS_File>::Read(char* buffer, size_t size) {
  return is_->Read(buffer, size) == size;
}

template <>
bool IOStreamAdaptor<FILE>::Write(char* buffer, size_t size) {
  size_t ntimes = fwrite(buffer, size, 1, is_);
  return (ntimes == 1) && (ferror(is_) == 0);
}

template <>
bool IOStreamAdaptor<MRMLFS_File>::Write(char* buffer, size_t size) {
  return is_->Write(buffer, size) == size;
}

//-----------------------------------------------------------------------------
// We are going to save the read value into either (1) an std::string
// object or (2) a protocol message.  The following template unifies
// the interface to the saving operation.
//-----------------------------------------------------------------------------
template <class ValueType>
class SaveValue {
 public:
  SaveValue(KeyValuePair* pair, ValueType* value);
};

template <>
SaveValue<string>::SaveValue(KeyValuePair* pair, string* value) {
  value->swap(*pair->mutable_value());
}

template <>
SaveValue<ProtoMessage>::SaveValue(KeyValuePair* pair, ProtoMessage* msg) {
  msg->ParseFromString(pair->value());
}

//-----------------------------------------------------------------------------
// Given IOStreamAdaptor and SaveValue, the following function
// template implements the reading of a key-vlaue pair.
//-----------------------------------------------------------------------------
template <class StreamType, class ValueType>
bool ReadRecord(StreamType* input_stream,
                string* key,
                ValueType* value) {
  IOStreamAdaptor<StreamType> is(input_stream);

  uint32 encoded_msg_size;
  if (!is.Read(reinterpret_cast<char*>(&encoded_msg_size),
               sizeof(encoded_msg_size))) {
    // Do not LOG(ERROR) here, because users do not want to be
    // bothered when they invoke this function in the form of
    // while(MRML_ReadRecord(...)) { ... }, i.e., to probe the end of
    // a record file.
    return false;
  }

  if (encoded_msg_size > kMRMLRecordIOMaxRecordSize) {
    LOG(FATAL) << "Failed to read a proto message with size = "
               << encoded_msg_size
               << ", which is larger than kMRMLRecordIOMaxRecordSize ("
               << kMRMLRecordIOMaxRecordSize << ")."
               << "You can modify kMRMLRecordIOMaxRecordSize defined in "
               << __FILE__;
  }

  static char buffer[kMRMLRecordIOMaxRecordSize];
  if (!is.Read(buffer, encoded_msg_size)) {
    LOG(ERROR) << "Failed in reading a protocol buffer message.";
    return false;
  }

  KeyValuePair pair;
  CHECK(pair.ParseFromArray(buffer, encoded_msg_size));
  key->swap(*pair.mutable_key());
  SaveValue<ValueType>(&pair, value);
  return true;
}

//-----------------------------------------------------------------------------
// Interfaces defined in mrml_recordio.h are invocations of
// realizations of function template ReadRecord.
//-----------------------------------------------------------------------------

bool MRML_ReadRecord(FILE* input, string* key, string* value) {
  return ReadRecord<FILE, string>(input, key, value);
}

bool MRML_ReadRecord(FILE* input, string* key, ProtoMessage* value) {
  return ReadRecord<FILE, ProtoMessage>(input, key, value);
}

bool MRML_ReadRecord(MRMLFS_File* input, string* key, string* value) {
  return ReadRecord<MRMLFS_File, string>(input, key, value);
}

bool MRML_ReadRecord(MRMLFS_File* input, string* key, ProtoMessage* value) {
  return ReadRecord<MRMLFS_File, ProtoMessage>(input, key, value);
}

//-----------------------------------------------------------------------------
// This template saves (1) a string or (2) a protoco message into a
// key-value pair and encode it.
//-----------------------------------------------------------------------------
template <class ValueType>
class EncodeKeyValuePair {
 public:
  EncodeKeyValuePair(const string& key, const ValueType& value, string* pair);
};

template <>
EncodeKeyValuePair<string>::EncodeKeyValuePair(const string& key,
                                               const string& value,
                                               string* encoded_pair) {
  KeyValuePair pair;
  pair.set_key(key);
  pair.set_value(value);
  pair.SerializeToString(encoded_pair);
}

template <>
EncodeKeyValuePair<ProtoMessage>::EncodeKeyValuePair(const string& key,
                                                     const ProtoMessage& value,
                                                     string* encoded_pair) {
  KeyValuePair pair;
  pair.set_key(key);
  value.SerializeToString(pair.mutable_value());
  pair.SerializeToString(encoded_pair);
}

//-----------------------------------------------------------------------------
// This function template implements the saving of a key-value pair.
//-----------------------------------------------------------------------------
template <class OutputStream, class ValueType>
bool WriteRecord(OutputStream* output_stream,
                 const string& key,
                 const ValueType& value) {
  IOStreamAdaptor<OutputStream> os(output_stream);

  string encoded_msg;
  EncodeKeyValuePair<ValueType>(key, value, &encoded_msg);

  uint32 msg_size = encoded_msg.size();
  if (!os.Write(reinterpret_cast<char*>(&msg_size), sizeof(msg_size))) {
    LOG(ERROR) << "Failed in writing a record size.";
    return false;
  }

  if (!os.Write(const_cast<char*>(encoded_msg.data()), msg_size)) {
    LOG(ERROR) << "Failed in writing a record.";
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
// Interfaces defined in mrml_recordio.h are invocations of
// realizations of function template ReadRecord.
//-----------------------------------------------------------------------------

bool MRML_WriteRecord(FILE* output,
                      const string& key,
                      const string& value) {
  return WriteRecord<FILE, string>(output, key, value);
}

bool MRML_WriteRecord(MRMLFS_File* output,
                      const string& key,
                      const string& value) {
  return WriteRecord<MRMLFS_File, string>(output, key, value);
}

bool MRML_WriteRecord(FILE* output,
                      const string& key,
                      const ProtoMessage& value) {
  return WriteRecord<FILE, ProtoMessage>(output, key, value);
}

bool MRML_WriteRecord(MRMLFS_File* output,
                      const string& key,
                      const ProtoMessage& value) {
  return WriteRecord<MRMLFS_File, ProtoMessage>(output, key, value);
}

