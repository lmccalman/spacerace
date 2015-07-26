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

#include "types.hpp"
#include "init.hpp"
#include "network.hpp"
#include "lobbythread.hpp"
#include "controlthread.hpp"
#include "infothread.hpp"
#include "physics.hpp"
#include "game.hpp"
#include "maps.hpp"

INITIALIZE_EASYLOGGINGPP

int main(int ac, char* av[])
{
  // Initialise the various subsystems
  Eigen::initParallel();
  START_EASYLOGGINGPP(ac, av);
  initialiseSignalHandler();
  po::variables_map vm = commandLineArgs(ac, av);
  json settings = loadSettingsFile(vm["settings"].as<std::string>());
  
  // ZMQ initialisation
  std::unique_ptr<zmq::context_t> context(new zmq::context_t(1));
  zmq::socket_t stateSocket(*context, ZMQ_PUB);
  initialiseSocket(stateSocket, settings["stateSocket"]["port"],
      settings["stateSocket"]["linger"], settings["stateSocket"]["timeout"]);

  // Main game objects
  
  MapData mapData;  // make it here because contains mutex (no move or copy)
  loadMaps(settings, mapData);
  PlayerData playerData;
  bool gameRunning = false;
   
  // Initialise threads 
  std::future<void> infoThread = std::async(std::launch::async, runInfoThread,
      std::ref(*context), 
      std::cref(settings));
  
  std::future<void> lobbyThread = std::async(std::launch::async, runLobbyThread, 
      std::ref(*context), 
      std::ref(mapData),
      std::ref(playerData),
      std::cref(settings));

  std::future<void> controlThread = std::async(std::launch::async, runControlThread,
      std::ref(*context), 
      std::ref(playerData), 
      std::ref(gameRunning),
      std::cref(settings));

  
  uint lobbyWait = settings["lobbyWait"];
  while(!interruptedBySignal)
  {

    // waiting period between games
    LOG(INFO) << "Waiting for connections... (" << lobbyWait << " seconds)";
    waitForSignal(lobbyWait);
    LOG(INFO) << "Wait complete. Initialising Game!!!";
    
    //get the game started
    LOG(INFO) << "Acquiring game data from other threads";
    {
      std::lock_guard<std::mutex> mapLock(mapData.mutex);
      std::lock_guard<std::mutex> playerLock(playerData.mutex);

      playerData.controlInputs.setZero();
      playerData.current.clear();
      std::swap(playerData.current, playerData.next);
      // increment to the next map
      mapData.currentMap = (mapData.currentMap + 1) % mapData.maps.size();
    }
    LOG(INFO) << "Game data acquired.";

    // can't play if no-one playing
    if (playerData.current.size() == 0)
    {
      LOG(INFO) << "Skipping match because no-one has connected";
      continue; 
    }
    gameRunning = true;
    runGame(playerData, mapData);
    gameRunning = false;
    LOG(INFO) << "Game Over";

  }
  
  LOG(INFO) << "Shutting down...";

  LOG(INFO) << "Closing sockets";
  stateSocket.close();

  // destroy context
  context = nullptr;

  LOG(INFO) << "Waiting for lobby thread...";
  lobbyThread.wait();

  LOG(INFO) << "Thankyou for playing spacerace!";
  return 0;

}
