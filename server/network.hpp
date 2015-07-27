#pragma once

#include <vector>
#include <string>
#include "zmq.hpp"

void send(zmq::socket_t& socket, const std::vector<std::string>& msg)
{
  uint i=0;
  while (i < msg.size() - 1)
  {
    uint n = msg[i].size();
    zmq::message_t m(n);
    memcpy(m.data(), msg[i].data(), n);
    socket.send(m, ZMQ_SNDMORE);
    i++;
  }
  uint n = msg[i].size();
  zmq::message_t m(n);
  memcpy(m.data(), msg[i].data(), n);
  socket.send(m);
}

bool receive(zmq::socket_t& socket, std::vector<std::string>& msg)
{
  // std::vector<std::string> msg;
  int isMore = 1;
  size_t isMoreSize = sizeof(int);
  bool hasMsg;
  msg.clear();
  while (isMore)
  {
    zmq::message_t m;
    hasMsg = socket.recv(&m);
    if (hasMsg)
    {
      msg.push_back(std::string(static_cast<char*>(m.data()), m.size()));
      socket.getsockopt(ZMQ_RCVMORE, (void*)(&isMore), &isMoreSize);
    }
    else
    {
      break; 
    }
  }
  return hasMsg;
}

std::ostream &operator<<(std::ostream &os, std::vector<std::string> const &v) 
{ 
  if (!v.empty()) 
  {
    os << "|" << v.front();
    std::for_each(v.begin() + 1, v.end(),
        [&](const std::string& p) { os << "|" << p; });
    os << "|";
  }
  return os;
}

std::string asHex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

void initialiseSocket(zmq::socket_t& s, uint port, int linger, int timeoutMS)
{
  s.setsockopt(ZMQ_LINGER, &linger, sizeof(int));
  s.setsockopt(ZMQ_RCVTIMEO, &timeoutMS, sizeof(int));
  std::string address = "tcp://*:" + std::to_string(port);
  s.bind(address.c_str());

}
