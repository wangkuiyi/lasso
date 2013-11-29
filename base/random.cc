


//
#include "base/random.h"

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/*static*/
uint32 Random::GetTickCount() {
  struct timeval t;
  gettimeofday(&t, NULL);
  t.tv_sec %= (24 * 60 * 60);  // one day ticks 24*60*60
  uint32 tick_count = t.tv_sec * 1000 + t.tv_usec / 1000;
  return tick_count;
}

void CRuntimeRandom::SeedRNG(int seed) {
  if (seed >= 0) {
    seed_ = seed;
  } else {
    seed_ = getpid() * time(NULL);  // BUG(yiwang): should also times thread id
  }
}

void MTRandom::SeedRNG(int seed) {
  if (seed >= 0) {
    uniform_01_rng_.base().seed(seed);
  } else {
    uniform_01_rng_.base().seed(GetTickCount());
  }
}

