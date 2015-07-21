#pragma once
#include <chrono>

namespace ch = std::chrono;
using hrclock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;


void waitPreciseInterval(const TimePoint& start, uint desiredMicroseconds)
{
  uint elapsedTime = ch::duration_cast<ch::microseconds>(hrclock::now() - start).count();
  if (elapsedTime < desiredMicroseconds)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(desiredMicroseconds - elapsedTime));
  }
  else
  {
    LOG(INFO) << "WARNING: preciseWait is running late";
  }
}

bool hasRoughIntervalPassed(const TimePoint& start, uint desiredSeconds, uint evalEveryN)
{
  static uint n = 0;
  bool done = false;
  if (n % evalEveryN == 0)
  {
    n = 0;
    uint elapsedTime = ch::duration_cast<ch::seconds>(hrclock::now() - start).count();
    done = elapsedTime > desiredSeconds;
  }
  n++;
  return done;
}
