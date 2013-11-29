

//
// This file provides MRMLFS_File, an interface to remote file access
// through SFTP protocol.
//
#ifndef MRML_MRML_FILESYSTEM_H_
#define MRML_MRML_FILESYSTEM_H_

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string>


class MRMLFS_File {
  friend class MRMLFS_FileTest;

 public:
  enum Type { Local, SFTP };

  MRMLFS_File();
  MRMLFS_File(const std::string& filename, bool for_read);
  virtual ~MRMLFS_File();

  // This function opens a local file or a remote file (through SFTP).
  // The filename must be in either one of the following formats:
  //
  // 1. sftp://<username>:<password>@<hostname>:<local-path>
  //
  //    This format denotes a remote SFTP file protocol.  If <hostname>
  //    is "localhost" or "127.0.0.1", the filename will be deprecated
  //    into a local file, i.e., only local-path is reserved.
  //
  // 2. <local-path>
  //
  //    This format denotes a local file.
  //
  // If |for_read| is ture, the file will be opened in read-only
  // mode. Otherwise, it will be truncated and overwritten.
  //
  bool Open(const std::string& filename, bool for_read);

  bool IsOpen() const;

  // Generally, this function returns the total number of bytes
  // successfully read.  Particularly, for SFTP file, it invokes
  // libssh2_sftp_read directly, and for local file, it invokes fread
  // directly.
  size_t Read(char* buffer, size_t size);

  // Generally, this function returns the actual number of bytes
  // written or negative on failure. If this number differs from the
  // count parameter, it indicates an error.  Particularly, for SFTP
  // file, it invokes libssh2_sftp_write directly, and for local file,
  // it invokes fwrite directly.
  size_t Write(const char* buffer, size_t size);

  void Close();

 protected:
  // Fields consisting of a filename.
  struct FilenameFields {
    std::string protocol;
    std::string username;
    std::string password;
    std::string hostname;
    int port;
    std::string path;

    FilenameFields() : port(0) {}
    ~FilenameFields() { Clear(); }
    void Clear();
  };

  // Fields for accessing a local file:
  FILE*                local_file_;

  // Fields for accessing a SFTP file:
  int                  socket_fd_;
  LIBSSH2_SESSION*     ssh2_session_;
  LIBSSH2_SFTP*        sftp_session_;
  LIBSSH2_SFTP_HANDLE* sftp_handle_;

  static bool ParseFilename(const std::string& filename, FilenameFields* f);
  bool OpenLocalFile(const FilenameFields& f, bool for_read);
  bool OpenSFTPFile(const FilenameFields& f, bool for_read);
};

#endif  // MRML_MRML_FILESYSTEM_H_
