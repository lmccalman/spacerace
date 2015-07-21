#pragma once

#include <chrono>
#include <atomic>
#include "zmq.hpp"

std::atomic<bool> interruptedBySignal;

struct Sockets
{
  zmq::socket_t log;
  zmq::socket_t state;
  // zmq::socket_t control;
};

namespace ch = std::chrono;
using hrclock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

