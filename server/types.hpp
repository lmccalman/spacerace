#pragma once

#include <map>
#include <string>
#include <set>
#include <mutex>
#include "Eigen/Dense"
#include "json.hpp"

using json = nlohmann::json;

const uint STATE_LENGTH = 6;
const uint CONTROL_LENGTH = 2;

using StateMatrix = Eigen::Matrix<float,Eigen::Dynamic,STATE_LENGTH>;
using ControlMatrix = Eigen::Matrix<float, Eigen::Dynamic, CONTROL_LENGTH>;
using StateVector = Eigen::Matrix<float,1,STATE_LENGTH>;
using ControlVector = Eigen::Matrix<float,1,CONTROL_LENGTH>;

struct ControlData
{
  ControlMatrix inputs;
  std::map<std::string, uint> idx;
  std::map<std::string, std::string> keyToId;
  std::mutex mutex;
};

struct PlayerSet
{
  std::set<std::string> ids;
  std::map<std::string, std::string> secretKeys;
  std::mutex mutex;
};

struct Map
{
  Eigen::MatrixXf occupancy;
  Eigen::MatrixXf normalx;
  Eigen::MatrixXf normaly;
  json jsonData;
};

struct MapData
{
  std::vector<Map> maps;
  uint currentMap = 0;
  std::mutex mutex;
};


struct GameState
{
  std::string name;
  bool running = false;
  std::mutex mutex;
};
