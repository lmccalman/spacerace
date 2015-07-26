#pragma once


void loadMaps(const json& settings, MapData& mapData)
{
  Map m {Eigen::MatrixXf::Zero(2,2), Eigen::MatrixXf::Zero(2,2), Eigen::MatrixXf::Zero(2,2), "jsonmapdata"};
  mapData.maps.push_back(m);
  mapData.maps.push_back(m);
  //TODO load the maps into the struct
  
}
