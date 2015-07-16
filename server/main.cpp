#include <boost/log/trivial.hpp>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>
#include "zmq.hpp"


std::atomic<bool> interruptedBySignal;

void handleSignal(int sig)
{
  BOOST_LOG_TRIVIAL(debug) << "Caught signal of type " << sig;
  interruptedBySignal = true;
}

void initialiseSignalHandler()
{
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
}


int main(int ac, char* av[])
{
  zmq::context_t context(1);

  initialiseSignalHandler();
  BOOST_LOG_TRIVIAL(info) << "An informational severity message";


  zmq::socket_t logSocket(context, ZMQ_PUB);
  zmq::socket_t stateSocket(context, ZMQ_PUB);
  zmq::socket_t controlSocket(context, ZMQ_PULL);


  logSocket.bind("tcp://*:5555");
  logSocket.bind("tcp://*:5556");
  logSocket.bind("tcp://*:5557");


  while(!interruptedBySignal)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  return 0;

}
