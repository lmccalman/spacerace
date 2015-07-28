#pragma once

#include <vector>
#include <ostream>

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