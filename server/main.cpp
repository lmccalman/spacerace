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
  LOG(INFO) << "Logging initialised";
  LOG(INFO) << "Initialising signal handler";
  initialiseSignalHandler();

  LOG(INFO) << "Parsing command line";
  po::variables_map vm = commandLineArgs(ac, av);
  LOG(INFO) << "loading setting from json";
  std::string settingsPath = vm["settings"].as<std::string>();
  if (!fileExists(settingsPath))
  {
    LOG(FATAL) << "Could not find settings file";
    return 1;
  }
  json settings = loadSettingsFile(settingsPath);
  
  // ZMQ initialisation
  LOG(INFO) << "Initialising ZeroMQ";
  std::unique_ptr<zmq::context_t> context(new zmq::context_t(1));
  zmq::socket_t stateSocket(*context, ZMQ_PUB);
  initialiseSocket(stateSocket, settings["stateSocket"]["port"],
      settings["stateSocket"]["linger"], settings["stateSocket"]["timeout"]);

  // Main game objects
  MapData mapData;  // make it here because contains mutex (no move or copy)
  loadMaps(settings, mapData);
  if (!(mapData.maps.size()> 0))
  {
    LOG(FATAL)  << "Must have at least 1 map loaded...Exiting";
    return 1;
  }
  // initialise map 0 as the "next" map
  mapData.currentMap = mapData.maps.size() - 1; 
  PlayerSet nextPlayers;
  PlayerSet currentPlayers;
  ControlData control;
  bool gameRunning = false;
   
  // Initialise threads 
  LOG(INFO) << "Starting info thread";
  std::future<void> infoThread = std::async(std::launch::async, runInfoThread,
      std::ref(*context), 
      std::cref(settings));
  
  LOG(INFO) << "Starting lobby thread";
  std::future<void> lobbyThread = std::async(std::launch::async, runLobbyThread, 
      std::ref(*context), 
      std::ref(mapData),
      std::ref(nextPlayers),
      std::cref(settings));

  LOG(INFO) << "Starting control thread";
  std::future<void> controlThread = std::async(std::launch::async, runControlThread,
      std::ref(*context), 
      std::ref(currentPlayers), 
      std::ref(control),
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
      std::lock_guard<std::mutex> nextPlayerLock(nextPlayers.mutex);
      std::lock_guard<std::mutex> currentPlayerLock(currentPlayers.mutex);
      std::lock_guard<std::mutex> controlLock(control.mutex);
      
      // get list of players
      std::swap(nextPlayers.ids, currentPlayers.ids);
      std::swap(nextPlayers.secretKeys, currentPlayers.secretKeys);
      nextPlayers.ids.clear();
      nextPlayers.secretKeys.clear();

      // build the control structures and index maps for the current game
      uint nShips = currentPlayers.ids.size();
      control.inputs = Eigen::MatrixXf::Zero(nShips, CONTROL_LENGTH);
      uint idx=0;
      for (auto const& i : currentPlayers.ids)
      {
        control.idx[i] = idx;
        idx++;
      }
      LOG(INFO) << nShips << " players connected for this round";
      
      // Update the map
      mapData.currentMap = (mapData.currentMap + 1) % mapData.maps.size();
      LOG(INFO) << "Beginning game on map " << mapData.currentMap;

    }
    LOG(INFO) << "Game data acquired.";

    if (currentPlayers.ids.size() == 0)
    {
      LOG(INFO) << "Skipping match because no-one has connected";
      continue; 
    }
    gameRunning = true;
    runGame(currentPlayers, control, 
        mapData.maps[mapData.currentMap], stateSocket, settings);
    gameRunning = false;
    LOG(INFO) << "Game Over";

  }
  
  LOG(INFO) << "Shutting down...";

  LOG(INFO) << "Closing sockets";
  stateSocket.close();

  // destroy context
  context = nullptr;

  LOG(INFO) << "Waiting for control thread...";
  controlThread.wait();
  LOG(INFO) << "Waiting for lobby thread...";
  lobbyThread.wait();
  LOG(INFO) << "Waiting for info thread...";
  infoThread.wait();

  LOG(INFO) << "Server Shutdown Successfully. Thank you for playing spacerace!";
  return 0;

}
