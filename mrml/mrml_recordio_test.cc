

//
#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "base/common.h"
#include "mrml/mrml_filesystem.h"
#include "mrml/mrml_recordio.h"
#include "mrml/mrml.pb.h"

using std::string;

DEFINE_string(username, "", "Username for test IO of a remote SFTP file.");
DEFINE_string(password, "", "Password for test IO of a remote SFTP file.");
DEFINE_string(hostname, "", "Hostname for test IO of a remote SFTP file.");
DEFINE_string(path, "", "Path for test IO of a remote SFTP file.");

static const char* kTestKey = "a key";
static const char* kTestValue = "a value";

static void CheckWriteReadConsistency(const string& filename) {
  KeyValuePair pair;
  pair.set_key(kTestKey);
  pair.set_value(kTestValue);

  MRMLFS_File file(filename, false);
  CHECK(file.IsOpen());
  MRML_WriteRecord(&file, kTestKey, kTestValue);
  MRML_WriteRecord(&file, kTestKey, pair);
  file.Close();

  CHECK(file.Open(filename, true));
  string key, value;
  CHECK(MRML_ReadRecord(&file, &key, &value));
  EXPECT_EQ(key, kTestKey);
  EXPECT_EQ(value, kTestValue);

  pair.Clear();
  CHECK(MRML_ReadRecord(&file, &key, &pair));
  EXPECT_EQ(pair.key(), kTestKey);
  EXPECT_EQ(pair.value(), kTestValue);

  EXPECT_TRUE(!MRML_ReadRecord(&file, &key, &value));
}

TEST(MRMLRecordIOTest, LocalRecordIO) {
  static const char* kFilename = "/tmp/testLocalRecordIO";
  CheckWriteReadConsistency(kFilename);
}

TEST(MRMLRecordIOTest, SFTPRecordIO) {
  CHECK(!FLAGS_username.empty());
  CHECK(!FLAGS_password.empty());
  CHECK(!FLAGS_hostname.empty());
  CHECK(!FLAGS_path.empty());
  string filename = "sftp://" + FLAGS_username + ":" + FLAGS_password + "@" +
    FLAGS_hostname + ":" + FLAGS_path;
  CheckWriteReadConsistency(filename);
}
