

//
#include <string>

#include "gtest/gtest.h"

#include "base/common.h"
#include "mrml/mrml_filesystem.h"

using std::string;

DEFINE_string(remote_path, "",
              "The path specified for testing CreateAndReadSFTPFile");

class MRMLFS_FileTest : public ::testing::Test {
 public:
  static void ParseLocalFile();
  static void ParseSFTPFile();
  static void ParseLocalSFTPFile();
  static void CreateAndReadLocalFile();
  static void CreateAndReadSFTPFile();
};

void MRMLFS_FileTest::ParseLocalFile() {
  static const char* kFilename = "/tmp/a.h";
  static const char* kFilenameWithProtocol = "file:///tmp/a.h";

  MRMLFS_File::FilenameFields fields;

  MRMLFS_File::ParseFilename(kFilename, &fields);
  EXPECT_EQ(fields.protocol, "file://");
  EXPECT_EQ(fields.path, kFilename);

  MRMLFS_File::ParseFilename(kFilenameWithProtocol, &fields);
  EXPECT_EQ(fields.protocol, "file://");
  EXPECT_EQ(fields.path, kFilename);
}

void MRMLFS_FileTest::ParseSFTPFile() {
  static const char* kFilename1 = "sftp://user:password@host:/tmp/a.h";
  static const char* kFilename2 = "sftp://user:password@host#36000:/tmp/a.h";

  MRMLFS_File::FilenameFields fields;
  MRMLFS_File::ParseFilename(kFilename1, &fields);
  EXPECT_EQ(fields.protocol, "sftp://");
  EXPECT_EQ(fields.username, "user");
  EXPECT_EQ(fields.password, "password");
  EXPECT_EQ(fields.hostname, "host");
  EXPECT_EQ(fields.port, 22);
  EXPECT_EQ(fields.path, "/tmp/a.h");

  MRMLFS_File::ParseFilename(kFilename2, &fields);
  EXPECT_EQ(fields.protocol, "sftp://");
  EXPECT_EQ(fields.username, "user");
  EXPECT_EQ(fields.password, "password");
  EXPECT_EQ(fields.hostname, "host");
  EXPECT_EQ(fields.port, 36000);
  EXPECT_EQ(fields.path, "/tmp/a.h");
}


void MRMLFS_FileTest::ParseLocalSFTPFile() {
  static const char* kFilename1 = "sftp://user:password@127.0.0.1:/tmp/a.h";
  static const char* kFilename2 = "sftp://user:password@localhost:/tmp/a.h";

  MRMLFS_File::FilenameFields fields;

  MRMLFS_File::ParseFilename(kFilename1, &fields);
  EXPECT_EQ(fields.protocol, "file://");
  EXPECT_EQ(fields.path, "/tmp/a.h");

  MRMLFS_File::ParseFilename(kFilename2, &fields);
  EXPECT_EQ(fields.protocol, "file://");
  EXPECT_EQ(fields.path, "/tmp/a.h");
}

void MRMLFS_FileTest::CreateAndReadLocalFile() {
  static const char* kFilename = "/tmp/testCreateAndReadLocalFile";
  static const char kContent[] = "apple";

  MRMLFS_File file;
  CHECK(file.Open(kFilename, false));
  file.Write(kContent, sizeof(kContent));
  file.Close();

  CHECK(file.Open(kFilename, true));
  char buffer[sizeof(kContent) + 1];
  EXPECT_EQ(sizeof(kContent), file.Read(buffer, sizeof(kContent)));
  EXPECT_EQ(string(kContent), buffer);
  file.Close();
}

void MRMLFS_FileTest::CreateAndReadSFTPFile() {
  CHECK(!FLAGS_remote_path.empty());

  static const char kContent[] = "apple";

  MRMLFS_File file;
  CHECK(file.Open(FLAGS_remote_path, false));
  file.Write(kContent, sizeof(kContent));
  file.Close();

  CHECK(file.Open(FLAGS_remote_path, true));
  char buffer[sizeof(kContent) + 1];
  EXPECT_EQ(sizeof(kContent), file.Read(buffer, sizeof(kContent)));
  EXPECT_EQ(string(kContent), buffer);
  file.Close();
}

TEST_F(MRMLFS_FileTest, ParseLocalFile) {
  MRMLFS_FileTest::ParseLocalFile();
}

TEST_F(MRMLFS_FileTest, ParseSFTPFile) {
  MRMLFS_FileTest::ParseSFTPFile();
}

TEST_F(MRMLFS_FileTest, ParseLocalSFTPFile) {
  MRMLFS_FileTest::ParseLocalSFTPFile();
}

TEST_F(MRMLFS_FileTest, CreateAndReadLocalFile) {
  MRMLFS_FileTest::CreateAndReadLocalFile();
}

TEST_F(MRMLFS_FileTest, CreateAndReadSFTPFile) {
  MRMLFS_FileTest::CreateAndReadSFTPFile();
}
