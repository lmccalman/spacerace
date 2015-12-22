#pragma once

#include <algorithm>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "types.hpp"

std::string formulateResponse(const std::string& shipName, const std::string& shipTeam,
      const std::string& gameName, const Map& map)
{
  json j {{"name", shipName},{"team", shipTeam}, {"game", gameName}, {"map",map.name}};
  return j.dump();
}

Player processMessage(const std::vector<std::string>& rawMsg, 
    const json& settings)
{
  // try and parse the message into json
  json msg;
  try 
  {
    msg = json::parse(rawMsg[2]);
  }
  catch (...)
  {
    throw Error("lobby", "error", {"message", "could not process json!"}); 
  }
  LOG(INFO)  << "New message received: " << msg;

  // Check we have the required fields to build a player
  if (!msg.count("name"))
    throw Error("lobby", "error", {"message", "lobby message must contain name field"});
  if (!msg.count("team"))
    throw Error("lobby", "error", {"message", "lobby message must contain team field"});
  if (!msg.count("password"))
    throw Error("lobby", "error", {"message", "lobby message must contain password field"});

  // Build the actual player
  Player p;
  p.id = msg["name"].get<std::string>();
  p.team = msg["team"].get<std::string>();
  p.secretKey = msg["password"].get<std::string>();
  if (msg.count("mass"))
  {
    float d = msg["mass"];
    d = std::max(d, float(settings["simulation"]["ship"]["minimumDensity"]));
    d = std::min(d, float(settings["simulation"]["ship"]["maximumDensity"]));
    p.density = d;
  }
  else
  {
    p.density = float(settings["simulation"]["ship"]["defaultDensity"]);
  }
  
  return p;
}

void addPlayerToNextGame(const Player& p, const std::string& zmqAddress,
    PlayerSet& players, GameState& gameState, Map& map,
    zmq::socket_t& socket, InfoLogger& logger, const json& settings)
{
    std::lock_guard<std::mutex> gameStateLock(gameState.mutex);
    LOG(INFO) << "Lobby adding new player to game " << gameState.name;

    players.fromId[p.id] = p;
    players.idFromSecret[p.secretKey] = p.id;
    // Notify everyone of the new players
    std::vector<std::string> lobbyPlayers;
    for(auto const& p: players.fromId)
      lobbyPlayers.push_back(p.first);
    json r = {{"map", map.name},
              {"newPlayer", p.id},
              {"players", lobbyPlayers},
              {"game", gameState.name}};
    logger("lobby", "status", r);

    std::string response = formulateResponse(p.id, p.team,
        gameState.name, map);
    LOG(INFO) << "lobby sending response: " << response;
    send(socket, {zmqAddress, "", response}); 
}


void addPlayer(const Player& p, const std::string& zmqAddress, 
    PlayerSet& currentPlayers, GameState& currentGameState, PlayerSet& nextPlayers, 
    GameState& nextGameState, MapData& mapData, zmq::socket_t& socket, 
    InfoLogger& logger, const json& settings)
{
  std::lock_guard<std::mutex> currentLock(currentPlayers.mutex);
  std::lock_guard<std::mutex> nextLock(nextPlayers.mutex);
  std::lock_guard<std::mutex> mapLock(mapData.mutex);
  if (currentPlayers.fromId.count(p.id))
  {
    // already in a game, just resend the response
    uint map = mapData.currentMap;
    std::string response = formulateResponse(p.id, p.team, 
        currentGameState.name, mapData.maps[mapData.currentMap]);
    LOG(INFO) << "lobby sending response: " << response;
    send(socket, {zmqAddress, "", response}); 
  }
  else if (nextPlayers.fromId.count(p.id))
  {
    throw Error("lobby", "error", {"message", "player " + p.id + "already added!"});
  }
  else if (nextPlayers.fromId.size() == settings["maxPlayers"])
  {
    throw Error("lobby", "error", {"message", "Game full! try the next game."});
  }
  else // no reason not to add you to next game
  {
    uint nextMap = (mapData.currentMap + 1) % mapData.maps.size();
    addPlayerToNextGame(p, zmqAddress, nextPlayers, nextGameState, 
        mapData.maps[nextMap], socket, logger, settings);
  }
}

void runLobbyThread(zmq::context_t& context, MapData& mapData, 
    PlayerSet& inGamePlayers, GameState& inGameState,
    PlayerSet& nextPlayers, GameState& nextGameState, const json& settings)
{
  LOG(INFO) << "starting lobby thread";
  InfoLogger logger(context);

  // Set up zeromq socket
  int linger = settings["lobbySocket"]["linger"];
  int timeout = settings["lobbySocket"]["timeout"];
  uint port = settings["lobbySocket"]["port"];
  zmq::socket_t socket(context, ZMQ_ROUTER);
  initialiseSocket(socket, port, linger, timeout);

  // Lobby main loop
  while (!interruptedBySignal)
  {
    std::vector<std::string> rawMsg;
    bool newMsg = receive(socket, rawMsg);
    if (newMsg)
    {
      const std::string& zmqAddress = rawMsg[0];
      try
      {
        Player p = processMessage(rawMsg, settings);
        addPlayer(p, zmqAddress, inGamePlayers, inGameState, nextPlayers, 
            nextGameState, mapData, socket, logger, settings);
      }
      catch(Error e)
      {
        logger(e.category, e.subject, e.data);
      }
    }
  }
  
  LOG(INFO) << "Lobby thread exiting...";
  logger.close();
  socket.close();
}
