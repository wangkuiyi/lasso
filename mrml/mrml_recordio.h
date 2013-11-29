
//
// The interface to accessing MRML RecordIO files.
//
#ifndef MRML_MRML_RECORDIO_H_
#define MRML_MRML_RECORDIO_H_

#include <stdio.h>

#include <string>

namespace google {
  namespace protobuf {
    class Message;
  }
}

class MRMLFS_File;

bool MRML_ReadRecord(FILE* input,
                     std::string* key,
                     std::string* value);
bool MRML_ReadRecord(FILE* input,
                     std::string* key,
                     ::google::protobuf::Message* value);
bool MRML_ReadRecord(MRMLFS_File* input,
                     std::string* key,
                     std::string* value_pb);
bool MRML_ReadRecord(MRMLFS_File* input,
                     std::string* key,
                     ::google::protobuf::Message* value_pb);

bool MRML_WriteRecord(FILE* output,
                      const std::string& key,
                      const std::string& value);
bool MRML_WriteRecord(FILE* output,
                      const std::string& key,
                      const ::google::protobuf::Message& value);
bool MRML_WriteRecord(MRMLFS_File* output,
                      const std::string& key,
                      const std::string& value);
bool MRML_WriteRecord(MRMLFS_File* output,
                      const std::string& key,
                      const ::google::protobuf::Message& value);

#endif  // MRML_MRML_RECORDIO_H_
