

//
// This is a copy from

//   trunk/src/common/system/concurrency/condition_variable.hpp
//
#ifndef SYSTEM_CONDITION_VARIABLE_H_
#define SYSTEM_CONDITION_VARIABLE_H_

#ifndef _WIN32
#if __unix__
#include <pthread.h>
#endif
#endif

#include <assert.h>
#include "system/mutex.h"

class ConditionVariable {
 public:
  ConditionVariable();
  ~ConditionVariable();

  void Signal();
  void Broadcast();

  bool Wait(Mutex* inMutex, int inTimeoutInMilSecs);
  void Wait(Mutex* inMutex);

 private:
#ifdef _WIN32
  HANDLE m_hCondition;
  unsigned int m_nWaitCount;
#elif __unix__
  pthread_cond_t m_hCondition;
#endif
  static void CheckError(const char* context, int error);
};

#endif  // SYSTEM_CONDITION_VARIABLE_H_

