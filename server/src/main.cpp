#define ELPP_THREAD_SAFE
#define EIGEN_DONT_PARALLELIZE

#include <mutex>
#include <future>
#include <utility>
#include <set>
#include <iostream>
#include <thread>
#include <chrono>

#include "stream.hpp"

#include "zmq.hpp"
#include "json.hpp"
#include "easylogging++.h"

#include "types.hpp"
#include "init.hpp"
#include "network.hpp"
#include "infothread.hpp"
#include "lobbythread.hpp"
#include "controlthread.hpp"
#include "physics.hpp"
#include "maps.hpp"
#include "game.hpp"

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
    exit(EXIT_FAILURE);
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
    exit(EXIT_FAILURE);
  }
  // initialise map 0 as the "next" map
  uint gameNumber = 0;
  mapData.currentMap = mapData.maps.size() - 1; 
  PlayerSet nextPlayers;
  PlayerSet currentPlayers;
  ControlData control;
  GameState currentGameState;
  GameState nextGameState;
  GameStats stats;
  nextGameState.name = gameName(gameNumber);
  SimulationParameters params = readParams(settings);
   
  // Initialise threads 
  LOG(INFO) << "Starting info thread";
  std::future<void> infoThread = std::async(std::launch::async, runInfoThread,
      std::ref(*context), 
      std::cref(settings));
  
  LOG(INFO) << "Starting lobby thread";
  std::future<void> lobbyThread = std::async(std::launch::async, runLobbyThread, 
      std::ref(*context), 
      std::ref(mapData),
      std::ref(currentPlayers),
      std::ref(currentGameState),
      std::ref(nextPlayers),
      std::ref(nextGameState),
      std::cref(settings));


  LOG(INFO) << "Starting control thread";
  std::future<void> controlThread = std::async(std::launch::async, runControlThread,
      std::ref(*context), 
      std::ref(currentPlayers), 
      std::ref(control),
      std::cref(currentGameState),
      std::cref(settings));

  // External logging system
  InfoLogger logger(*context);
  logger("game", "status", json({"message", "External logging initialised"}));

  
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
      std::lock_guard<std::mutex> nextGameStateLock(currentGameState.mutex);
      std::lock_guard<std::mutex> currentGameStateLock(nextGameState.mutex);

      // Update the game states
      currentGameState.name = gameName(gameNumber);
      nextGameState.name = gameName(gameNumber+1);
      
      // get list of players
      std::swap(nextPlayers.fromId, currentPlayers.fromId);
      std::swap(nextPlayers.idFromSecret, currentPlayers.idFromSecret);
      nextPlayers.fromId.clear();
      nextPlayers.idFromSecret.clear();

      // build the control structures, index maps and densities for the current game
      control.idx.clear();
      control.ids.clear();
      uint nShips = currentPlayers.fromId.size();
      control.inputs = Eigen::MatrixXf::Zero(nShips, CONTROL_LENGTH);
      params.shipDensities = Eigen::VectorXf(nShips);
      uint idx=0;
      for (auto const& p : currentPlayers.fromId)
      {
        assert(p.first == p.second.id);
        control.idx[p.second.secretKey] = idx;
        control.ids[idx] = p.first;
        params.shipDensities[idx] = p.second.density;
        idx++;
      }
      LOG(INFO) << nShips << " players connected for this round";
      
      // Update the map
      mapData.currentMap = (mapData.currentMap + 1) % mapData.maps.size();
      LOG(INFO) << "Beginning game on map " << mapData.currentMap;
        
    }
    LOG(INFO) << "Game data acquired.";

    if (currentPlayers.fromId.size() == 0)
    {
      LOG(INFO) << "Skipping match because no-one has connected";
      gameNumber++;
      continue; 
    }
    
    currentGameState.running = true;
    runGame(currentPlayers, control, currentGameState,
        mapData.maps[mapData.currentMap], params, stats, stateSocket, settings, 
        logger);
    currentGameState.running = false;
    LOG(INFO) << "Game Over";
    gameNumber++;
  }
  
  LOG(INFO) << "Shutting down...";

  LOG(INFO) << "Closing sockets";
  logger.close();
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
