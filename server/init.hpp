#pragma once

#include "types.hpp"
#include <csignal>
#include "easylogging++.h"


void handleSignal(int sig)
{
  LOG(INFO) << "signal " << sig << " caught...";
  interruptedBySignal = true;
}

void initialiseSignalHandler()
{
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
}

