#include <mutex>
#include <future>
#include <utility>
#include <atomic>
#include <set>
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include "zmq.hpp"
#include "json.hpp"
#include "easylogging++.h"
#include "network.hpp"

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

// Note that the mutex is intended to lock both the mapData and the ids
void collectClients(zmq::context_t& context, std::set<std::string>& ids,
    std::mutex& mutex, const std::string& mapData)
{
  uint msWait = 1;
  zmq::socket_t socket(context, ZMQ_ROUTER);
  socket.bind("tcp://*:5557");
  std::vector<zmq::pollitem_t> pollList = { {&socket, 0, ZMQ_POLLIN, 0} };
  
  auto newMsg = [&]() {return pollList[0].revents & ZMQ_POLLIN;};
        
  while (!interruptedBySignal)
  {
    zmq::poll(&(pollList[0]), pollList.size(), msWait);
    while (newMsg())
    {
      auto msg = receive(socket);
      if (msg.size() == 3)
      {
        std::lock_guard<std::mutex> lock(mutex);
        if (!ids.count(msg[2]))
        {
          ids.insert(msg[2]);
          LOG(INFO) << "New Player! ID: " << msg[2];
          send(socket, {msg[0], "", mapData});
        }
        else
        {
          LOG(INFO) << "Player trying to join with existing ID";
          send(socket,{msg[0], "", "ERROR: ID Taken. Please try something else."});
        }
      }
      else
      {
          LOG(INFO) << "Client sent mal-formed connection message";
          send(socket,{msg[0], "", "ERROR: Please connect with single msg frame as your ID"});
      }
    }
  }
}

int main(int ac, char* av[])
{
  int major, minor, patch;
  zmq::version (&major, &minor, &patch); printf ("Current ØMQ version is %d.%d.%d\n", major, minor, patch);
  zmq::context_t* context = new zmq::context_t(1);

  initialiseSignalHandler();

  LOG(INFO) << "Initialising sockets";

  zmq::socket_t logSocket(*context, ZMQ_PUB);
  zmq::socket_t stateSocket(*context, ZMQ_PUB);

  logSocket.bind("tcp://*:5555");
  stateSocket.bind("tcp://*:5556");

  LOG(INFO) << "Starting main loop";

  std::mutex mapDataMutex;
  std::string nextMapData = "nextMapData";
  std::string currentMapData = "currentMapData";
  std::set<std::string> nextPlayers;
  std::set<std::string> currentPlayers;
  
  std::future<void> controlThread = std::async(std::launch::async, collectClients, 
      std::ref(*context), 
      std::ref(nextPlayers),
      std::ref(mapDataMutex), 
      std::cref(nextMapData));

  while(!interruptedBySignal)
  {
    // waiting period between games
    std::this_thread::sleep_for(std::chrono::seconds(30));

    //get the game started
    {
      std::lock_guard<std::mutex> lock(mapDataMutex);
      currentPlayers.clear();
      std::swap(currentPlayers, nextPlayers);
      currentMapData = nextMapData;
      nextMapData = "somenewmapdata";
    }

    //now play the game
    // finish off the game
  }

  // auto start = std::chrono::high_resolution_clock::now();
  // uint elapsedTime = 0;
  // while (elapsedTime < timeoutSecs)
    
  // elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(
      // std::chrono::high_resolution_clock::now() - start).count();

  controlThread.wait();
  
  LOG(INFO) << "Closing sockets";

  logSocket.close();
  stateSocket.close();

  LOG(INFO) << "Deleting context";
  delete context;
  context = nullptr; // just in case

  LOG(INFO) << "Thankyou for playing spacerace!";
  return 0;

}
