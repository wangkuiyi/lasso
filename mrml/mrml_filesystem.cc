

//
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "boost/regex.hpp"

#include "base/common.h"
#include "strutil/split_string.h"
#include "mrml/mrml_filesystem.h"

using std::string;
using std::vector;

static const char* kFileProtocol = "file://";
static const char* kSFTPProtocol = "sftp://";
static const int   kDefaultSFTPPort = 22;

void MRMLFS_File::FilenameFields::Clear() {
  protocol.clear();
  username.clear();
  password.clear();
  hostname.clear();
  port = 0;
  path.clear();
}

/*static*/
bool MRMLFS_File::ParseFilename(const string& filename,
                                FilenameFields* f) {
  using boost::match_results;
  using boost::regex;
  using boost::regex_match;

  // The regular expression that can match the following:
  //    sftp://username:password@hostname#port:path
  //    sftp://username:password@hostname:path
  //    sftp://username@hostname#port:path
  //    sftp://hostname#port:path
  //    sftp://hostname:path
  //    file://hostname:path
  //    path
  regex expression("([^:]*://)?([^:@]+(:[^@]*)?@)?([^#]*(#[^:]*)?:)?(.*)");

  match_results<string::const_iterator> what;
  f->Clear();

  if (regex_match(filename, what, expression)) {
    f->protocol = what[1];

    string usrpwd = what[2];
    usrpwd = usrpwd.substr(0, usrpwd.size() - 1);
    vector<string> usr_pwd;
    SplitStringUsing(usrpwd, ":", &usr_pwd);
    if (usr_pwd.size() > 0)
      f->username = usr_pwd[0];
    if (usr_pwd.size() > 1)
      f->password = usr_pwd[1];

    string hostport = what[4];
    hostport = hostport.substr(0, hostport.size() - 1);
    vector<string> host_port;
    SplitStringUsing(hostport, "#", &host_port);
    if (host_port.size() > 0)
      f->hostname = host_port[0];
    if (host_port.size() > 1)
      f->port = atoi(host_port[1].c_str());

    f->path     = what[6];
  } else {
    LOG(ERROR) << "Cannot parsing filename: " << filename;
    return false;
  }

  if (f->port == 0) {
    f->port = 22;
  }

  if (f->protocol.empty()) {
    f->protocol = kFileProtocol;
  }

  if (f->hostname == "127.0.0.1" || f->hostname == "localhost") {
    f->protocol = kFileProtocol;
  }

  return true;
}

// NOTE: this function is not thread-safe.
static const char* GetUsername() {
  static char login_name[1024];
  getlogin_r(login_name, sizeof(login_name));
  return login_name;
}

// NOTE: this function is not thread-safe.
static const char* GetHomeDir(const char* login_name) {
  struct passwd pwent;
  struct passwd* ppwent;
  char strings[1024];
  getpwnam_r(login_name, &pwent, strings, sizeof(strings), &ppwent);
  if (ppwent == NULL) {
    LOG(ERROR) << "getpwnam_r failed with login name: " << login_name;
    return NULL;
  }
  return pwent.pw_dir;
}

MRMLFS_File::MRMLFS_File() {
  local_file_ = NULL;
  socket_fd_ = 0;
  ssh2_session_ = NULL;
  sftp_session_ = NULL;
  sftp_handle_ = NULL;
}

MRMLFS_File::MRMLFS_File(const string& filename, bool for_read) {
  local_file_ = NULL;
  socket_fd_ = 0;
  ssh2_session_ = NULL;
  sftp_session_ = NULL;
  sftp_handle_ = NULL;

  if (!Open(filename, for_read)) {
    LOG(ERROR) << "Cannot open: " << filename;
  }
}

MRMLFS_File::~MRMLFS_File() {
  if (IsOpen()) {
    Close();
  }
}

bool MRMLFS_File::IsOpen() const {
  return local_file_ != NULL || sftp_handle_ != NULL;
}

bool MRMLFS_File::Open(const std::string& filename, bool for_read) {
  FilenameFields fields;
  ParseFilename(filename, &fields);
  if (fields.protocol == kFileProtocol) {
    return OpenLocalFile(fields, for_read);
  } else if (fields.protocol == kSFTPProtocol) {
    return OpenSFTPFile(fields, for_read);
  }
  LOG(ERROR) << "Unknown protocol: " << fields.protocol;
  return false;
}

bool MRMLFS_File::OpenLocalFile(const MRMLFS_File::FilenameFields& fields,
                                bool for_read) {
  CHECK(local_file_ == NULL);
  local_file_ = fopen(fields.path.c_str(), for_read ? "r" : "w+");
  return local_file_ != NULL;
}

bool MRMLFS_File::OpenSFTPFile(const MRMLFS_File::FilenameFields& fields,
                               bool for_read) {
  CHECK_EQ(socket_fd_, 0);
  CHECK(ssh2_session_ == NULL);
  CHECK(sftp_session_ == NULL);
  CHECK(sftp_handle_  == NULL);

  // Resolve hostname into sockaddr.
  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;    // TODO(yiwang): need to support IPv6?
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;
  addrinfo* resolved_addrinfo;
  if (0 != getaddrinfo(fields.hostname.c_str(),  // using hostname
                       NULL,                     // rather than service name
                       &hints,
                       &resolved_addrinfo)) {
    LOG(ERROR) << "getaddrinfo Failed with hostname: " << fields.hostname;
    return false;
  }

  // Force socket connect to given host and port 22.
  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in* in = reinterpret_cast<sockaddr_in*>(resolved_addrinfo->ai_addr);
  in->sin_port = htons(fields.port);
  if (connect(socket_fd_,
              resolved_addrinfo->ai_addr,
              sizeof(struct sockaddr_in)) != 0) {
    LOG(ERROR) << "Socket API failed to connect to "
               << resolved_addrinfo->ai_canonname;
    return false;
  }

  // Create a session instance
  if ((ssh2_session_ = libssh2_session_init()) == NULL) {
    LOG(ERROR) << "libssh2_session_init failed.";
    return false;
  }

  // Since we have set non-blocking, tell libssh2 we are blocking.
  libssh2_session_set_blocking(ssh2_session_, 1);

  // Start it up. This will trade welcome banners, exchange keys, and
  // setup crypto, compression, and MAC layers
  if (libssh2_session_startup(ssh2_session_, socket_fd_) != 0) {
    LOG(ERROR) << "Failure establishing SSH session.";
    return -1;
  }

  const char* username =
    fields.username.empty() ? GetUsername() : fields.username.c_str();

  if (fields.password.empty()) {
    // If no password specified, autheticate using public/private key pair.
    string home_dir = GetHomeDir(username);
    string public_keyfile =  home_dir + "/.ssh/id_rsa.pub";
    string private_keyfile = home_dir + "/.ssh/id_rsa";
    if (libssh2_userauth_publickey_fromfile(ssh2_session_,
                                            username,
                                            public_keyfile.c_str(),
                                            private_keyfile.c_str(),
                                            "")) {
      LOG(ERROR) << "Failed authetication using public/private key pair:"
                 << public_keyfile << "/" << private_keyfile;
      return false;
    }
  } else {
    // If there is password specified, using the password for authetication.
    if (libssh2_userauth_password(ssh2_session_,
                                  username,
                                  fields.password.c_str())) {
      LOG(ERROR) << "Failed authentication with username: " << fields.username;
      return false;
    }
  }

  // Initialize an SFTP session.
  if ((sftp_session_ = libssh2_sftp_init(ssh2_session_))
      == NULL) {
    LOG(ERROR) << "Unable to init SFTP session";
    return false;
  }

  // Request a file via SFTP.
  if ((sftp_handle_ = libssh2_sftp_open(sftp_session_,
                                        fields.path.c_str(),
                                        (for_read ? LIBSSH2_FXF_READ :
                                         (LIBSSH2_FXF_WRITE|
                                          LIBSSH2_FXF_CREAT|
                                          LIBSSH2_FXF_TRUNC)),
                                        (for_read ? 0 :
                                         (LIBSSH2_SFTP_S_IRUSR|
                                          LIBSSH2_SFTP_S_IWUSR|
                                          LIBSSH2_SFTP_S_IRGRP|
                                          LIBSSH2_SFTP_S_IROTH)))) == NULL) {
    LOG(ERROR) << "Unable to open SFTP file: " << fields.path
               << (for_read ? " for read" : " for write");
    return false;
  }

  freeaddrinfo(resolved_addrinfo);
  return true;
}

void MRMLFS_File::Close() {
  if (local_file_ != NULL) {
    fclose(local_file_);
    local_file_ = NULL;
  } else if (sftp_handle_ != NULL) {
    CHECK_NE(socket_fd_, 0);
    CHECK(sftp_session_ != NULL);
    CHECK(ssh2_session_ != NULL);

    libssh2_sftp_close(sftp_handle_);
    libssh2_sftp_shutdown(sftp_session_);
    libssh2_session_disconnect(ssh2_session_, "shutdown");
    libssh2_session_free(ssh2_session_);
    close(socket_fd_);

    socket_fd_    = 0;
    ssh2_session_ = NULL;
    sftp_session_ = NULL;
    sftp_handle_  = NULL;
  } else {
    LOG(ERROR) << "No file opened yet.";
  }
}

size_t MRMLFS_File::Read(char* buffer, size_t size) {
  if (local_file_ != NULL) {
    return fread(buffer, sizeof(buffer[0]), size, local_file_);
  } else if (sftp_handle_ != NULL) {
    return libssh2_sftp_read(sftp_handle_, buffer, size);
  } else {
    LOG(ERROR) << "File has not been opened yet.";
    return -1;
  }
}

size_t MRMLFS_File::Write(const char* buffer, size_t size) {
  if (local_file_ != NULL) {
    return fwrite(buffer, sizeof(buffer[0]), size, local_file_);
  } else if (sftp_handle_ != NULL) {
    return libssh2_sftp_write(sftp_handle_, buffer, size);
  } else {
    LOG(ERROR) << "File has not been opened yet.";
    return -1;
  }
}

