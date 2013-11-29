

//
#ifndef MRML_LASSO_TERMINATION_FLAG_H_
#define MRML_LASSO_TERMINATION_FLAG_H_

#include <fstream>  // NOLINT: TODO(yiwang) use fopen to create the flag file.
#include "mrml-lasso/learner_states.h"

namespace logistic_regression {

// TerminationFlag creates a text file on local filesystem to
// indicate the termination of a training procedure.  The content of
// the file is the model parameters.
template <class LearnerStates>
class TerminationFlag {
 public:
  static bool SetLocally(const char* termination_filename,
                         const char* termination_reason,
                         const LearnerStates* states) {
    std::ofstream output(termination_filename);
    if (!output.is_open()) {
      LOG(ERROR) << "Cannot create termination flag file: "
                 << termination_filename;
      return false;
    }

    output << termination_reason << "\n";
    if (states != NULL) {
      output << "x = " << states->x() << "\n"
             << "new_x = " << states->new_x() << "\n";
    }
    return true;
  }
};

}  // namespace logistic_regression

#endif  // MRML_LASSO_TERMINATION_FLAG_H_
