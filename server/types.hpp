#pragma once

#include <map>
#include <string>
#include <set>
#include <mutex>
#include <sys/types.h>

#include "Eigen/Dense"
#include "json.hpp"

using json = nlohmann::json;

const uint STATE_LENGTH = 6;
const uint CONTROL_LENGTH = 2;

using StateMatrix = Eigen::Matrix<float,Eigen::Dynamic,STATE_LENGTH>;
using ControlMatrix = Eigen::Matrix<float, Eigen::Dynamic, CONTROL_LENGTH>;
using StateVector = Eigen::Matrix<float,1,STATE_LENGTH>;
using ControlVector = Eigen::Matrix<float,1,CONTROL_LENGTH>;

namespace Eigen
{
  using MatrixXb = Matrix<bool, Dynamic, Dynamic>;
}

struct ControlData
{
  ControlMatrix inputs;
  std::map<std::string, uint> idx;
  std::map<uint, std::string> ids;
  std::mutex mutex;
};


struct Player
{
  std::string id;
  std::string secretKey;
  std::string team;
  float density;
};

struct PlayerSet
{
  std::map<std::string, Player> fromId;
  std::map<std::string, std::string> idFromSecret;
  std::mutex mutex;
};

struct Map
{
  std::string name;

  Eigen::MatrixXb occupancy;

  std::set<std::pair<uint,uint>> start;
  std::set<std::pair<uint,uint>> finish;

  Eigen::MatrixXf flowx;
  Eigen::MatrixXf flowy;
  
  Eigen::MatrixXf endDistance;
  Eigen::MatrixXf wallDistance;

  Eigen::MatrixXf wallNormalx;
  Eigen::MatrixXf wallNormaly;

  float maxDistance;

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


struct GameStats
{
  std::map<std::string, float> playerDists;
  std::map<std::string, unsigned int> playerRanks;
  std::map<std::string, float> totalPlayerScore;
  std::map<std::string, float> totalTeamScore;
};


struct SimulationParameters
{
  float linearThrust;
  float linearDrag;
  float rotationalThrust;
  float rotationalDrag;
  float shipRadius;
  float shipFriction;
  float shipRestitution;
  float wallRestitution;
  float wallFriction;
  float elasticity;
  float mapScale;
  float timeStep;
  float targetFPS;
  Eigen::VectorXf shipDensities;
};

