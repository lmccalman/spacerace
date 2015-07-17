#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>
#include "zmq.hpp"
#include "json.hpp"
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

std::atomic<bool> interruptedBySignal;
using json = nlohmann::json;

void handleSignal(int sig)
{
  interruptedBySignal = true;
}

void initialiseSignalHandler()
{
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
}


int main(int ac, char* av[])
{
  zmq::context_t* context = new zmq::context_t(1);

  initialiseSignalHandler();

  LOG(INFO) << "Initialising sockets";

  zmq::socket_t logSocket(*context, ZMQ_PUB);
  zmq::socket_t stateSocket(*context, ZMQ_PUB);
  zmq::socket_t controlSocket(*context, ZMQ_PULL);


  logSocket.bind("tcp://*:5555");
  stateSocket.bind("tcp://*:5556");
  controlSocket.bind("tcp://*:5557");


  while(!interruptedBySignal)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // load a map
    // spend 2 minutes getting connections and replying with the map, and
    // sending ready signals
    // start the match
    //
    // while (game is running)
    //  update game state
    //  broadcast game state
    //  receive control inputs
    // send finish message

  }
  
  LOG(INFO) << "Closing sockets";

  logSocket.close();
  stateSocket.close();
  controlSocket.close();

  LOG(INFO) << "Deleting context";
  delete context;

  LOG(INFO) << "Thankyou for playing spacerace!";
  return 0;

}
