

//
// This is a copy from

//   trunk/src/common/system/concurrency/condition_variable_test.cpp
//
#include "gtest/gtest.h"
#include "system/condition_variable.h"

TEST(ConditionVariable, Init) {
    ConditionVariable cond;
}

TEST(ConditionVariable, Wait) {
    ConditionVariable event;
    event.Signal();
}

TEST(ConditionVariable, Release) {
}

