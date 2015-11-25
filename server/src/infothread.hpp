#include "network.hpp"

class InfoLogger
{
  public:
    InfoLogger(zmq::context_t& c)
      : socket_(c, ZMQ_PUSH)
    {
      int linger = 0;
      socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(int));
      socket_.connect("inproc://infologger");
    }

    void operator()(const std::string& category, const std::string& subject, const json& data)
    {
      json j {
              {"category", category},
              {"subject", subject},
              {"data", data}
             };
      send(socket_, {j.dump()});
    }

    void close()
    {
      socket_.close(); 
    }

  private:
    zmq::socket_t socket_;
};


void infoMessage(const std::string& category, const std::string& subject, const json& data)
{
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
    {
      LOG(INFO) << msg;
      send(pub, msg);
    }
  }

  pull.close();
  pub.close();

}
