#include "network.hpp"

std::function<void(const std::string&)> getInfoLogger(zmq::context_t& c)
{
  int linger = 0;
  zmq::socket_t socket(c, ZMQ_PUSH);
  socket.setsockopt(ZMQ_LINGER, &linger, sizeof(int));
  socket.connect("inproc://infologger");
  
  auto f = [&socket] (const std::string& s) 
  {
    uint n = s.size();
    zmq::message_t m(n);
    memcpy(m.data(), s.data(), n);
    socket.send(m); 
  };

  return f;
}

void runInfoThread(zmq::context_t& c, const json& settings)
{
  int linger = 0;
  int timeout = 500;
  zmq::socket_t pull(c, ZMQ_PULL);
  zmq::socket_t pub(c, ZMQ_PUB);
  pull.setsockopt(ZMQ_LINGER, &linger, sizeof(int));
  pub.setsockopt(ZMQ_LINGER, &linger, sizeof(int));
  pull.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(int));
  pull.bind("inproc://infologger");
  pub.bind("tcp://*:" + std::to_string(int(settings["infoSocket"]["port"])));

  while (!interruptedBySignal)
  {
    std::vector<std::string> msg;
    bool newMsg = receive(pull, msg);
    if (newMsg)
      send(pub, msg);
  }
}
