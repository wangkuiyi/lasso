

//
#include <math.h>

#include <limits>

#include "base/common.h"
#include "mrml/mrml.h"
#include "mrml/mrml_filesystem.h"
#include "mrml/mrml_recordio.h"

#include "mrml-lasso/learner_states.h"

namespace logistic_regression {

void SerializeDoubleToProtoBuf(const double& v, DoublePB* pb) {
  pb->set_value(v);
}

void ParseDoubleFromProtoBuf(const DoublePB& pb, double* v) {
  *v = pb.value();
}

void SerializeInt32ToProtoBuf(int32 v, Int32PB* pb) {
  pb->set_value(v);
}

void ParseInt32FromProtoBuf(const Int32PB& pb, int32* v) {
  *v = pb.value();
}

void SerializeDequeToProtoBuf(const std::deque<double>& deque,
                              DoubleSequencePB* pb) {
  pb->Clear();
  for (int i = 0; i < deque.size(); ++i) {
    pb->add_value(deque[i]);
  }
}

void ParseDequeFromProtoBuf(const DoubleSequencePB& pb,
                            std::deque<double>* deque) {
  deque->clear();
  for (int i = 0; i < pb.value_size(); ++i) {
    deque->push_back(pb.value(i));
  }
}

ostream& operator<<(ostream& out, const deque<double>& double_deque) {
  for (size_t s = 0; s < double_deque.size(); ++s) {
    out << s << ":" << double_deque[s] << " ";
  }
  return out;
}

ostream& operator<<(ostream& out, const ImprovementFilter& filter) {
  out << filter.value_history_;
  return out;
}

void ImprovementFilter::SerializeToProtoBuf(DoubleSequencePB* pb) const {
  SerializeDequeToProtoBuf(value_history_, pb);
}

void ImprovementFilter::ParseFromProtoBuf(const DoubleSequencePB& pb) {
  ParseDequeFromProtoBuf(pb, &value_history_);
}

double ImprovementFilter::GetImprovement(double new_value) {
  double ret = std::numeric_limits<double>::infinity();

  if (value_history_.size() > kNumIterationsToAverage) {
    double previous_value = value_history_.front();
    if (value_history_.size() == 2 * kNumIterationsToAverage) {
      value_history_.pop_front();
    }
    double average_improvement =
        (previous_value - new_value) / value_history_.size();
    double relative_average_improvement =
        average_improvement / fabs(new_value);
    ret = relative_average_improvement;
  }

  value_history_.push_back(new_value);

  return ret;
}

}  // namespace logsitic_regression
