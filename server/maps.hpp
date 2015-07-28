#pragma once
#include "numpy.hpp"
#include <cstring>


// start, end, occupancy -- bool
// the rest are floats

Eigen::MatrixXf loadFloatFromNumpy(const std::string& filename)
{
  std::vector<int> shape;
  std::vector<float> data;
  aoba::LoadArrayFromNumpy(filename, shape, data);
  Eigen::MatrixXf mat(shape[0], shape[1]);
  std::size_t size = sizeof(float) * shape[0] * shape[1];
  memcpy(mat.data(), data.data(), size);
  return mat;
}


Eigen::MatrixXb loadBoolFromNumpy(const std::string& filename)
{
  std::vector<int> shape;
  std::vector<char> data;
  aoba::LoadArrayFromNumpy<char>(filename, shape, data);
  Eigen::MatrixXb mat(shape[0], shape[1]);
  std::size_t size = sizeof(char) * shape[0] * shape[1];
  memcpy(mat.data(), data.data(), size);
  return mat;
}

void loadMaps(const json& settings, MapData& mapData)
{
  for (std::string mapName : settings["maps"])
  {
    std::string path = settings["mapPath"];
    std::string prefix = path + "/" + mapName;
    Map m;
    m.start = loadBoolFromNumpy(prefix + "_start.npy");
    m.finish = loadBoolFromNumpy(prefix + "_end.npy"); 
    m.occupancy = loadBoolFromNumpy(prefix + "_occupancy.npy");
    m.flowx = loadFloatFromNumpy(prefix + "_flowx.npy"); 
    m.flowy = loadFloatFromNumpy(prefix + "_flowy.npy");
    m.endDistance = loadFloatFromNumpy(prefix + "_enddist.npy");
    m.wallDistance = loadFloatFromNumpy(prefix + "_walldist.npy"); 
    m.wallNormalx = loadFloatFromNumpy(prefix + "_wnormx.npy");
    m.wallNormaly = loadFloatFromNumpy(prefix + "_wnormy.npy");
    //TODO build json structure
    mapData.maps.push_back(std::move(m));
  }
  
}
