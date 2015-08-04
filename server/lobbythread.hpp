#pragma once

#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "types.hpp"

// Note that the mutex is intended to lock both the mapData and the ids

std::string formulateResponse(const std::string& shipName, const std::string& secretCode, 
    const std::string& gameName, const Map& map)
{
  json j {{"name", shipName}, {"secret", secretCode}, {"game", gameName}, {"map",map.name}};
  return j.dump();
}

std::string secretCode(boost::uuids::random_generator& gen)
{
  std::string s = boost::lexical_cast<std::string>(gen());
  s.erase(13, s.size()); // doesnt need to be that long
  return s;
}

void sendInitialInfo(bool& sentFirstStatus, PlayerSet& players,
    GameState& gameState, MapData& mapData, InfoLogger& logger)
{
  if (!sentFirstStatus && players.fromId.size() == 0)
  {
    sentFirstStatus = true;
    std::lock_guard<std::mutex> lock(players.mutex);
    std::lock_guard<std::mutex> gameStateLock(gameState.mutex);
    std::lock_guard<std::mutex> mapLock(mapData.mutex);
    uint nextMap = (mapData.currentMap + 1) % mapData.maps.size();
    logger("lobby", "status",{{"map", mapData.maps[nextMap].name},
                {"players", json::array()},
                {"game", gameState.name}});
  }
}

Player processMessage(const std::vector<std::string>& rawMsg, 
    const json& settings)
{
  static boost::uuids::random_generator gen;
  static float defaultDensity = float(settings["simulation"]["ship"]["defaultDensity"]);
  json msg = json::parse(rawMsg[2]);
  LOG(INFO)  << "New message received: " << msg;
  Player p;
  p.id = msg["name"].get<std::string>();
  p.team = msg["team"].get<std::string>();
  p.secretKey = secretCode(gen);
  p.density = defaultDensity;
  if (msg.count("mass"))
  {
    float d = msg["mass"];
    d = std::max(d, float(settings["simulation"]["ship"]["minimumDensity"]));
    d = std::min(d, float(settings["simulation"]["ship"]["maximumDensity"]));
    p.density = d;
  }
  return p;
}

// They can still play they just have to use the default density
// logger("lobby", "error", {"message", p.id + " tried to specify mass as " + msg["mass"].get<std::string>() + " but could not convert to float"});
// logger("lobby", "error", {"message", "Sorry, this game is full. Try the next game!"});
//
bool canAddPlayer(const Player& p, const PlayerSet& players, const json& settings)
{
  uint atMaxPlayers = players.fromId.size() == settings["maxPlayers"];
  return !players.fromId.count(p.id) && !atMaxPlayers;
}

void addPlayer(const Player& p, const std::string& zmqAddress, PlayerSet& players, GameState& gameState,
    MapData& mapData, zmq::socket_t& socket, InfoLogger& logger, const json& settings)
{
  std::lock_guard<std::mutex> lock(players.mutex);
  std::lock_guard<std::mutex> mapLock(mapData.mutex);
  uint nextMap = (mapData.currentMap + 1) % mapData.maps.size();
  if (canAddPlayer(p, players, settings))
  {
    // we want to resend next time it's empty again
    LOG(INFO) << "Lobby adding new player";
    std::lock_guard<std::mutex> gameStateLock(gameState.mutex);
    players.fromId[p.id] = p;
    players.idFromSecret[p.secretKey] = p.id;
    // Notify everyone of the new players
    std::vector<std::string> lobbyPlayers;
    for(auto const& p: players.fromId)
      lobbyPlayers.push_back(p.first);
    json r = {{"map", mapData.maps[nextMap].name},
              {"newPlayer", p.id},
              {"players", lobbyPlayers},
              {"game", gameState.name}};
    logger("lobby", "status", r);
    std::string response = formulateResponse(p.id, p.secretKey, gameState.name, 
                                             mapData.maps[nextMap]);
    LOG(INFO) << "lobby sending response: " << response;
    send(socket, {zmqAddress, "", response}); 
  }
}

void runLobbyThread(zmq::context_t& context, MapData& mapData, PlayerSet& players,
    GameState& gameState, const json& settings)
{
  LOG(INFO) << "starting lobby thread";
  InfoLogger logger(context);


  int linger = settings["lobbySocket"]["linger"];
  int timeout = settings["lobbySocket"]["timeout"];
  uint port = settings["lobbySocket"]["port"];
  zmq::socket_t socket(context, ZMQ_ROUTER);
  initialiseSocket(socket, port, linger, timeout);
  bool sentFirstStatus = false;

  while (!interruptedBySignal)
  {
    sendInitialInfo(sentFirstStatus, players, gameState, mapData, logger);
    // Receive messages
    std::vector<std::string> rawMsg;
    bool newMsg = receive(socket, rawMsg);
    if (newMsg)
    {
      const std::string& zmqAddress = rawMsg[0];
      Player p = processMessage(rawMsg, settings);
      sentFirstStatus = false;
      addPlayer(p, zmqAddress, players, gameState, mapData, socket, logger, settings);
    }
  }
  
  LOG(INFO) << "Lobby thread exiting...";
  logger.close();
  socket.close();
}
