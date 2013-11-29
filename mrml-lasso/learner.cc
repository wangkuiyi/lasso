

//

namespace logistic_regression {

const char* kErrorNonDescentDirection =
    "ERROR: UpdateDir chose a non-descent direction,  "
    "the line search will break, so we stop here. The "
    "likely reason is bug in gradient computation.";

const char* kEnoughLongLineSearch =
    "WARNING: We have done enough number of steps in "
    "line search, and have to stop.";

const char* kEnoughNumberOfIterations =
    "WARNING: We have done enough number of iterations.";

}  // namespace logistic_regression
