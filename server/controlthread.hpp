#include "zmq.hpp"
#include "network.hpp"
#include "json.hpp"

void runControlThread(zmq::context_t& context, 
                      PlayerSet& players,
                      ControlData& control,
                      const bool& gameRunning,
                      const json& settings)
{
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
    if (newMsg && gameRunning)
    {
      LOG(INFO) << "Control message received for processing";
      std::lock_guard<std::mutex> playerSetLock(players.mutex);
      // should be of form <secret_code>, <0 or 1>,<-1, 0 or 1>
      boost::split(messageTokens, msg[0], boost::is_any_of(","));
      bool validSender = control.idx.count(msg[0]);
      if (validSender)
      {
        std::lock_guard<std::mutex> controlLock(control.mutex);
        uint idx = control.idx[messageTokens[0]];
        control.inputs(idx,0) = float(messageTokens[2] == "1");
        control.inputs(idx,1) = float(messageTokens[3] == "1") - float(messageTokens[3] == "-1");
        LOG(INFO) << "Ship " << control.keyToId[messageTokens[0]] << " enacted control " 
          << control.inputs(idx,0) << "," << control.inputs(idx,1);
      }
    }
  }
}
  

