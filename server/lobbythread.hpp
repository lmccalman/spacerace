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
      LOG(INFO) << "New message received!!";
      if (msg.size() == 3)
      {
        std::lock_guard<std::mutex> lock(players.mutex);
        uint nextMap = (mapData.currentMap + 1) % mapData.maps.size();
        if (!players.ids.count(msg[2]))
        {
          players.ids.insert(msg[2]);
          LOG(INFO) << "New Player! ID: " << msg[2];
          std::string secretCode = boost::lexical_cast<std::string>(newRandomID());
          secretCode.erase(13, secretCode.size()); // doesnt need to be that long
          players.secretKeys[msg[2]] = secretCode;
          send(socket, {msg[0], "", secretCode, mapData.maps[nextMap].json});
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
