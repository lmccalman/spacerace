#define ELPP_THREAD_SAFE
#define EIGEN_DONT_PARALLELIZE

#include <mutex>
#include <future>
#include <utility>
#include <set>
#include <iostream>
#include <thread>
#include <chrono>
#include "zmq.hpp"
#include "json.hpp"
#include "easylogging++.h"
#include "network.hpp"
#include "Eigen/Dense"
#include "physics.hpp"
#include <boost/algorithm/string.hpp>
#include "game.hpp"
#include "types.hpp"
#include "init.hpp"

INITIALIZE_EASYLOGGINGPP

using json = nlohmann::json;

// Note that the mutex is intended to lock both the mapData and the ids
void collectClients(zmq::context_t& context, std::set<std::string>& ids,
    std::mutex& mutex, const std::string& mapData)
{
  LOG(INFO) << "starting lobby thread";
  // -1 implies wait until message
  int msWait = 500;
  int linger = 0;
  
  zmq::socket_t socket(context, ZMQ_ROUTER);
  socket.setsockopt(ZMQ_LINGER, &linger, sizeof(int));
  std::string address = "tcp://*:5557";
  socket.bind(address.c_str());

  // note the void* cast is a weird part of the zmq api
  zmq::pollitem_t pollList [] = {{(void*)socket, 0, ZMQ_POLLIN, 0}};
  while (!interruptedBySignal)
  {
    zmq::poll(pollList, 1, msWait);
  
    bool newMsg = pollList[0].revents & ZMQ_POLLIN;
    if (newMsg)
    {
      LOG(INFO) << "New message received!!";
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
  LOG(INFO) << "Lobby thread exiting...";
}

void waitAndListen(uint seconds)
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


int main(int ac, char* av[])
{
  Eigen::initParallel();
  std::unique_ptr<zmq::context_t> context(new zmq::context_t(1));

  initialiseSignalHandler();

  LOG(INFO) << "Initialising sockets";

  Sockets sockets {
  zmq::socket_t(*context, ZMQ_PUB),
  zmq::socket_t(*context, ZMQ_PUB),
  // zmq::socket_t(context, ZMQ_PULL)
  };

  sockets.log.bind("tcp://*:5555");
  sockets.state.bind("tcp://*:5556");
  // sockets.control.bind("tcp://*:5558")

  LOG(INFO) << "Starting main loop";

  std::mutex mapDataMutex;
  std::string nextMapData = "nextMapData";
  std::string currentMapData = "currentMapData";
  std::set<std::string> nextPlayers;
  std::set<std::string> currentPlayers;
  
  std::future<void> lobbyThread = std::async(std::launch::async, collectClients, 
      std::ref(*context), 
      std::ref(nextPlayers),
      std::ref(mapDataMutex), 
      std::cref(nextMapData));

  uint lobbyWait = 5;
  float d_l = 1.0;
  float d_r = 0.5;
  float h = 1e-5;

  while(!interruptedBySignal)
  {
    // waiting period between games
    // need to do lots of short sleeps in a loop else ctrlc doesn't work
    LOG(INFO) << "Waiting for connections... (" << lobbyWait << " seconds)";
    waitAndListen(lobbyWait);
    LOG(INFO) << "Wait complete. Initialising Game!!!";
    

    //get the game started
    LOG(INFO) << "Acquiring game data from control thread";
    {
      std::lock_guard<std::mutex> lock(mapDataMutex);
      currentPlayers.clear();
      std::swap(currentPlayers, nextPlayers);
      currentMapData = nextMapData;
      nextMapData = "somenewmapdata";
    }
    LOG(INFO) << "Game data acquired.";

    // can't play if no-one playing
    if (currentPlayers.size() == 0)
    {
      LOG(INFO) << "Skipping match because no-one has connected";
      continue; 
    }

    runGame(currentPlayers, sockets, *context);
    LOG(INFO) << "Game Over";

    //now play the game
    // finish off the game
  }
  
  LOG(INFO) << "Shutting down...";

  LOG(INFO) << "Closing sockets";
  sockets.log.close();
  sockets.state.close();

  // destroy context
  context = nullptr;

  LOG(INFO) << "Waiting for lobby thread...";
  lobbyThread.wait();

  LOG(INFO) << "Thankyou for playing spacerace!";
  return 0;

}
