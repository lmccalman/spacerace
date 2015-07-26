#pragma once

#include <map>
#include <string>
#include <set>
#include <mutex>
#include "Eigen/Dense"

const uint STATE_LENGTH = 6;
const uint CONTROL_LENGTH = 2;

using StateMatrix = Eigen::Matrix<float,Eigen::Dynamic,STATE_LENGTH>;
using ControlMatrix = Eigen::Matrix<float, Eigen::Dynamic, CONTROL_LENGTH>;
using StateVector = Eigen::Matrix<float,1,STATE_LENGTH>;
using ControlVector = Eigen::Matrix<float,1,CONTROL_LENGTH>;

struct PlayerData
{
  std::set<std::string> next;
  std::set<std::string> current;
  std::map<std::string, uint> indices;
  std::map<std::string, std::string> secretKeys;
  ControlMatrix controlInputs;
  std::mutex mutex;
};

struct Map
{
  Eigen::MatrixXf occupancy;
  Eigen::MatrixXf normalx;
  Eigen::MatrixXf normaly;
  std::string json;
};

struct MapData
{
  std::vector<Map> maps;
  uint currentMap = 0;
  std::mutex mutex;
};

