#include "zmq.hpp"
#include "network.hpp"
#include "json.hpp"

void runControlThread(zmq::context_t& context, 
                      PlayerSet& players,
                      ControlData& control,
                      const GameState& gameState,
                      const json& settings)
{
  InfoLogger logger(context);

  int linger = settings["controlSocket"]["linger"];
  int timeout = settings["controlSocket"]["timeout"];
  uint port = settings["controlSocket"]["port"];
  zmq::socket_t socket(context, ZMQ_PULL);
  initialiseSocket(socket, port, linger, timeout);

  std::vector<std::string> messageTokens;
  while (!interruptedBySignal)
  {
    std::vector<std::string> msg;
    bool newMsg = receive(socket, msg);
    if (newMsg && gameState.running)
    {
      std::lock_guard<std::mutex> playerSetLock(players.mutex);
      // should be of form <secret_code>, <0 or 1>,<-1, 0 or 1>
      boost::split(messageTokens, msg[0], boost::is_any_of(","));
      std::string secretKey = messageTokens[0];
      bool validSender = control.idx.count(secretKey);
      bool validMessage = messageTokens.size() == 3;
      if (validSender && validMessage)
      {
        std::lock_guard<std::mutex> controlLock(control.mutex);
        uint idx = control.idx[secretKey];
        control.inputs(idx,0) = float(messageTokens[1] == "1");
        control.inputs(idx,1) = float(messageTokens[2] == "1") - float(messageTokens[2] == "-1");
      }
      else if (newMsg)
      {
        if (!validSender)
          logger("control", "error", {"message","Unknown secret key: " + secretKey});
        if (!validMessage)
          logger("control", "error", {"message","Ship " + players.idFromSecret[secretKey] + " sent invalid message"}); 
      }
    }
  }
  
  socket.close();
  logger.close();

}
  

