#pragma once
#include <chrono>

using hrclock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
namespace ch = std::chrono;

void waitForSignal(uint seconds)
{
  auto start = std::chrono::high_resolution_clock::now();
  uint elapsedTime = 0;
  while ((elapsedTime < seconds) && !interruptedBySignal)
  {
    elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::high_resolution_clock::now() - start).count();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  
}


void waitPreciseInterval(const TimePoint& start, uint desiredMicroseconds)
{
  uint elapsedTime = ch::duration_cast<ch::microseconds>(hrclock::now() - start).count();
  if (elapsedTime < desiredMicroseconds)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(desiredMicroseconds - elapsedTime));
  }
  else
  {
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
