#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "types.hpp"

// Note that the mutex is intended to lock both the mapData and the ids
void runLobbyThread(zmq::context_t& context, MapData& mapData, PlayerSet& players,
    const json& settings)
{
  LOG(INFO) << "starting lobby thread";
  InfoLogger logger(context);

  boost::uuids::random_generator newRandomID;
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
      if (msg.size() == 3)
      {
        std::lock_guard<std::mutex> lock(players.mutex);
        uint nextMap = (mapData.currentMap + 1) % mapData.maps.size();
        if (!players.ids.count(msg[2]))
        {
          players.ids.insert(msg[2]);
          logger({"New Player connected with ID: " + msg[2]});
          std::string secretCode = boost::lexical_cast<std::string>(newRandomID());
          secretCode.erase(13, secretCode.size()); // doesnt need to be that long
          players.secretKeys[msg[2]] = secretCode;
          send(socket, {msg[0], "", secretCode, mapData.maps[nextMap].json});
        }
        else
        {
          logger({"ERROR", "ID already in use: " + msg[2]});
          send(socket,{msg[0], "", "ERROR: ID Taken. Please try something else."});
        }
      }
      else
      {
        logger({"ERROR", "Client " + msg[0] + " sent mal-formed connection message"});
        send(socket,{msg[0], "", "ERROR: mal-formed connection message"});
      }
    }
  }
  LOG(INFO) << "Lobby thread exiting...";
  logger.close();
  socket.close();
}
