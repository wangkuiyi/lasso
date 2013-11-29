


#include "system/filepattern.h"

#include "base/common.h"

FilepatternMatcher::FilepatternMatcher(const std::string& filepattern) {
  glob_return_ = glob(filepattern.c_str(),
                      GLOB_MARK |        // Append a slash to each path which
                                         // corresponds to a directory.
                      GLOB_TILDE_CHECK | // Carry out tilde expansion.  If
                                         // the username is invalid, or the
                                         // home directory cannot be
                                         // determined, glob() returns
                                         // GLOB_NOMATCH to indicate an error.
                      GLOB_BRACE,        // Enable brace expressions.
                      FilepatternMatcher::ErrorFunc,
                      &glob_result_);

  if (glob_return_ != 0) {
    switch (glob_return_) {
      case GLOB_NOSPACE:
        LOG(ERROR) << "Run out of memory in file pattern matching.";
        break;
      case GLOB_ABORTED:
        LOG(ERROR) << "Encouterred read error in file pattern matching.";
        break;
      case GLOB_NOMATCH:
        LOG(ERROR) << "Filepattern " << filepattern << " matches no file.";
        break;
    }
  }
}

bool FilepatternMatcher::NoError() const {
  return glob_return_ == 0;
}

int FilepatternMatcher::NumMatched() const {
  return glob_result_.gl_pathc;
}

const char* FilepatternMatcher::Matched(int i) const {
  CHECK_LE(0, i);
  CHECK_LT(i, NumMatched());
  return glob_result_.gl_pathv[i];
}

FilepatternMatcher::~FilepatternMatcher() {
  globfree(&glob_result_);
}

/*static*/
int FilepatternMatcher::ErrorFunc(const char* epath, int eerrno) {
  LOG(ERROR) << "Failed at matching: " << epath << " due to error: " << eerrno;
  return 0;  // Retuns 0 to let glob() attempts carry on despite errors.
}
