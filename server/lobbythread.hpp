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

void runLobbyThread(zmq::context_t& context, MapData& mapData, PlayerSet& players,
    GameState& gameState, const json& settings)
{
  LOG(INFO) << "starting lobby thread";
  InfoLogger logger(context);

  boost::uuids::random_generator gen;

  int linger = settings["lobbySocket"]["linger"];
  int timeout = settings["lobbySocket"]["timeout"];
  uint port = settings["lobbySocket"]["port"];
  zmq::socket_t socket(context, ZMQ_ROUTER);
  initialiseSocket(socket, port, linger, timeout);

  while (!interruptedBySignal)
  {
    std::vector<std::string> msg;
    bool newMsg = receive(socket, msg);
    if (newMsg)
    {
      
      const std::string& shipName = msg[2];
      std::lock_guard<std::mutex> lock(players.mutex);
      uint nextMap = (mapData.currentMap + 1) % mapData.maps.size();
      if (!players.ids.count(shipName))
      {
        std::lock_guard<std::mutex> gameStateLock(gameState.mutex);
        players.ids.insert(shipName);
        logger({"New Player connected with ID: " + shipName});
        std::string secret = secretCode(gen);
        players.secretKeys[shipName] = secret;
        players.densities[shipName] = float(settings["simulation"]["ship"]["defaultDensity"]);
        if (msg.size() >= 4)
        {
          try
          {
            float d = stof(msg[3]);
            d = std::max(d, float(settings["simulation"]["ship"]["minimumDensity"]));
            d = std::min(d, float(settings["simulation"]["ship"]["maximumDensity"]));
            players.densities[shipName] = d;
          }
          catch(...)
          {
            // They can still play they just have to use the default density
            logger({"ERROR", shipName + " tried to specify density as " + msg[3] + " but could not convert to float"});
          }
        }
        std::string response = formulateResponse(shipName, secret, gameState.name, 
                                                 mapData.maps[nextMap]);
        send(socket, {msg[0], "", response}); 
      }
      else
      {
        logger({"ERROR", "ID already in use: " + shipName});
        send(socket,{msg[0], "", "ERROR: ID Taken. Please try something else."});
      }

    }
  }
  LOG(INFO) << "Lobby thread exiting...";
  logger.close();
  socket.close();
}
