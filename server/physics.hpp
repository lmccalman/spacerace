#pragma once
#include <cmath>
#include "Eigen/Dense"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <boost/functional/hash.hpp>  // need boost to hash std::pair
// State Vector
// ux uy vx vy  theta omega
// 0  1  2  3   4     5  

// Control Vector
// T_l T_r
// 0   1

// Hidden State // used for collisions, gravity, elastic walls etc
// F_x F_y Tau
//  0   1   2


float sigmoid(float x)
{
  return float(-1. + 2./(1. + exp(-x)));
}


void interpolate_map(float x, float y, float& wall_dist, float&
    norm_x, float& norm_y, const Map& map, const SimulationParameters&
    params) 
{
  uint w = map.occupancy.cols(); // I'm confused by maps.h
  uint h = map.occupancy.rows();

  // Get floats of image coordinates from map coordinates
  float fx = x * params.mapScale;
  float fy = y * params.mapScale;

  uint ix = std::max(std::min(uint(fx), w-2), uint(0));
  uint iy = std::max(std::min(uint(fy), h-2), uint(0));

  /*
  wall_dist = map.wallDistance(iy, ix);
  norm_x = map.wallNormalx(iy, ix);
  std::cout << ", " << map.occupancy(iy, ix) << x << ", " << y << ", " << ix << ", " << iy << ", " << wall_dist << ", " << norm_x << ", " << norm_y << "\n";
  return;
  */
  // Compute linear interp alphas
  float alphax = std::max(float(0.), std::min(float(1.), fx - float(ix)));
  float alphay = std::max(float(0.), std::min(float(1.), fy - float(iy)));
  
  wall_dist = 0.;
  norm_x = 0.;
  norm_y = 0.;

  for (uint dx=0; dx<2; dx++)
  {
    alphax = 1. - alphax;
    for (uint dy=0; dy<2; dy++)
    {
      alphay = 1. - alphay;
      
      wall_dist += alphax * alphay * map.wallDistance(iy+dy, ix+dx);
      norm_x += alphax * alphay * map.wallNormalx(iy+dy, ix+dx);
      norm_y += alphax * alphay * map.wallNormaly(iy+dy, ix+dx);
    }
  }

  return;
}


void derivatives(const StateMatrix& states, StateMatrix& derivs, 
    const ControlMatrix& ctrls, const SimulationParameters& params, 
    const Map & map)
{
  float cd_a_rho = params.linearDrag;  // 0.1  coeff of drag * area * density of fluid
  float k_elastic = params.elasticity;  // 4000.  // spring constant of ships
  float rad = 1.;  // leave radius 1 - we decided to change map scale instead
  const Eigen::VectorXf &masses = params.shipDensities;  // order of 1.0 Mass of ships 
  float spin_drag_ratio = params.rotationalDrag; // 1.8;  // spin friction to translation friction
  float eps = 1e-5;  // Avoid divide by zero special cases
  float mu = params.shipFriction;  // 0.05;  // friction coefficient between ships
  float mu_wall = params.wallFriction;  //0.25*?wallFriction;  // 0.01;  // wall friction parameter
  float wall_restitution = params.wallRestitution; // 0.5
  float ship_restitution = params.shipRestitution; // circa 0.5
  float diameter = 2.*rad;  // rad(i) + rad(j) for any i, j
  float inertia_mass_ratio = 0.25;
  float map_grid = rad * 2. + eps; // must be 2*radius + eps
  std::unordered_map<std::pair<int, int>, std::vector<uint>, 
                     boost::hash<std::pair<int, int>>> bins;

  uint n = states.rows();
  Eigen::MatrixXd f = Eigen::MatrixXd::Zero(n, 2);
  Eigen::VectorXd trq = Eigen::VectorXd::Zero(n);
  // rotationalThrust Order +- 10 
  // linearThrust Order +100
  // mapscale order 10 - thats params.pixelsize
  // Accumulate forces and torques into these:
  uint collide_checks = 0;  // debug count...

  for (uint i=0; i<n; i++) {
    Eigen::Vector2f pos_i;
    pos_i(0) = states(i,0);
    pos_i(1) = states(i,1);
    Eigen::Vector2f vel_i;
    vel_i(0) = states(i,2);
    vel_i(1) = states(i,3);
    float theta_i = states(i,4);
    float w_i = states(i,5);

    // 1. Control 
    float thrusting = ctrls(i, 0);
    float turning = ctrls(i, 1);
    f(i, 0) = thrusting * params.linearThrust * cos(theta_i);
    f(i, 1) = thrusting * params.linearThrust * sin(theta_i);
    trq(i) = turning * params.rotationalThrust;

    // 2. Drag
    f(i, 0) -= cd_a_rho * vel_i(0);
    f(i, 1) -= cd_a_rho * vel_i(1);
    trq(i) -= spin_drag_ratio*cd_a_rho*w_i*rad*rad; // * abs(w_i)

    // 3. Inter-ship collisions against ships of lower index...  
    // Figure out this ship's hashes: It has 4 in 2 dimensions
    std::unordered_set<uint> collision_shortlist;
    std::pair<int, int> my_hash;
    for (int dx=-1; dx < 2; dx+=2)
      for (int dy=-1; dy < 2; dy+=2)
      {
        float x_mod = pos_i(0) + float(dx)*rad;
        float y_mod = pos_i(1) + float(dy)*rad;
        my_hash = std::make_pair(int(x_mod / map_grid), 
                                 int(y_mod / map_grid));
        if (bins.count(my_hash) > 0)
        {
            // Already exists - shortlist others and add self
            std::vector<uint> current_bin = bins.find(my_hash)->second; 
            // -->first is the key as it returns a key/value pair
            for (uint bin_idx: current_bin)
              if (bin_idx != i)
                collision_shortlist.insert(bin_idx);
            current_bin.push_back(i);
        }
        else
        {
            // didnt exist - add self, and push into map
            std::vector<uint> current_bin;
            current_bin.push_back(i);
            bins.insert(std::make_pair(my_hash, current_bin));
        }
      }

    for (uint j: collision_shortlist) {  // =i+1; j<n; j++) {
      collide_checks ++;
      // std::cout << "Checking " << i << ", " << j << "\n";
      Eigen::Vector2f pos_j;
      pos_j(0) = states(j,0);
      pos_j(1) = states(j,1);
      Eigen::Vector2f vel_j;
      vel_j(0) = states(j,2);
      vel_j(1) = states(j,3);
      float theta_j = states(j,4);
      float w_j = states(j,5);

      Eigen::Vector2f dP = pos_j - pos_i;
      float dist = dP.norm() + eps - diameter;
      Eigen::Vector2f dPhat = dP / (dP.norm() + eps);
      if (dist < 0) {
        // we have a collision interaction
        
        // A. Direct collision: apply linear spring normal force
        float f_magnitude = - dist * k_elastic; // dist < =
        if ((vel_j - vel_i).dot(pos_j - pos_i) > 0)
            f_magnitude *= ship_restitution;
        Eigen::Vector2f f_norm = f_magnitude * dPhat;
        f(i, 0) -= f_norm(0);
        f(i, 1) -= f_norm(1);
        f(j, 0) += f_norm(0);
        f(j, 1) += f_norm(1);

        // B. Surface frictions: approximate spin effects
        Eigen::Vector2f perp;  // surface tangent pointing +theta direction
        perp(0) = -dPhat(1);
        perp(1) = dPhat(0);
        
        // relative velocities of surfaces
        float v_rel = rad*w_i + rad*w_j + perp.dot(vel_i - vel_j);
        float fric = f_magnitude * mu * sigmoid(v_rel);
        Eigen::Vector2f f_fric = fric * perp;
        f(i, 0) += f_fric(0);
        f(i, 1) += f_fric(1);
        f(j, 0) -= f_fric(0);
        f(j, 1) -= f_fric(1);
        trq(i) -= fric * rad;
        trq(j) -= fric * rad;
      }  // end collision
    } // end loop 3. opposing ship


    // 4. Wall single body collisions
    // compute distance to wall and local normals
    float wall_dist, norm_x, norm_y;
    interpolate_map(pos_i(0), pos_i(1), wall_dist, norm_x, norm_y, map, params);
    float dist = wall_dist - rad;
    if (dist < 0)
    {
      /* if (dist < -1.) */
      /*     assert(false); */

      // Spring force
      float f_norm_mag = -dist*k_elastic;  // dist is negative, f_norm is +ve
      if (norm_x*vel_i(0) + norm_y*vel_i(1) > 0)
          f_norm_mag *= wall_restitution;
      
      if (dist > -rad*0.25)
      {
        // not significantly through wall yet
        f(i, 0) += f_norm_mag * norm_x;
        f(i, 1) += f_norm_mag * norm_y;
      }
      else
      {
        // uh-oh - lets just SET normal forces and seriously damp vel
        f(i, 0) = f_norm_mag * norm_x;
        f(i, 1) = f_norm_mag * norm_y;
        f(i, 0) -= 100. * vel_i(0);
        f(i, 1) -= 100. * vel_i(1);
        
      }
      // Surface friction
      Eigen::Vector2f perp;  // surface tangent pointing +theta direction
      perp(0) = -norm_y;
      perp(1) = norm_x;
      float v_rel = w_i * rad + vel_i(0)*norm_y - vel_i(1)*norm_x;
      float fric = f_norm_mag * mu_wall * sigmoid(v_rel);
      f(i, 0) -= fric*norm_y;
      f(i, 1) += fric*norm_x;
      trq(i) -= fric * rad;
    }
  } // end loop current ship

  // std::cout << "Collision checks:" << collide_checks << "\n";
  // Compose the vector of derivatives:
  float vmax = 40.0;
  for (int i=0; i<n; i++)
  {
    float vx = states(i,2);
    float vy = states(i,3);
    float speed = std::sqrt(vx*vx + vy*vy);
    if (speed > vmax)
    {
      vx *= vmax/speed;
      vy *= vmax/speed;
    }
    
    // x_dot = vx
    derivs(i, 0) = vx;
    // y_dot = vy
    derivs(i, 1) = vy; 
    // vx_dot = fx / m
    float ax = f(i,0)/masses(i);
    float ay = f(i,1)/masses(i);
    
    derivs(i, 2) = ax;
    // vy_dot = fy / m
    derivs(i, 3) = ay;
    // theta_dot = omega
    derivs(i, 4) = states(i, 5);
    // omega_dot = T_r / (inertia_mass_ratio*m)
    derivs(i,5) = trq(i) / (inertia_mass_ratio * masses(i));
  }
  
  // ux uy vx vy  theta omega
  // 0  1  2  3   4     5
}

void rk4TimeStep(StateMatrix& s, const ControlMatrix& c, const
        SimulationParameters& params, const Map & m)
{
  // CHANGED(AL): Now implementing RK4
  StateMatrix k1, k2, k3, k4;
  uint n_states = s.cols();
  uint n = s.rows();
  k1 = StateMatrix(n, n_states);
  k2 = StateMatrix(n, n_states);
  k3 = StateMatrix(n, n_states);
  k4 = StateMatrix(n, n_states);

  float dt = params.timeStep;

  derivatives(s, k1, c, params, m);
  derivatives(s + 0.5*k1*dt, k2, c, params, m);
  derivatives(s + 0.5*k2*dt, k3, c, params, m);
  derivatives(s + k3*dt, k4, c, params, m);
  
  s += (k1 + 2.*k2 + 2.*k3 + k4) * dt/6.;

}

SimulationParameters readParams(const json& j)
{
  SimulationParameters s;
  
  s.linearThrust =  j["simulation"]["ship"]["linearThrust"];
  s.linearDrag = j["simulation"]["world"]["linearDrag"];
  s.rotationalThrust = j["simulation"]["ship"]["rotationalThrust"];
  s.rotationalDrag = j["simulation"]["world"]["rotationalDrag"]; 
  s.shipRadius = j["simulation"]["ship"]["radius"];
  s.shipFriction = j["simulation"]["ship"]["friction"];
  s.shipRestitution = j["simulation"]["ship"]["restitution"];
  s.elasticity = j["simulation"]["world"]["elasticity"];
  s.mapScale = j["simulation"]["world"]["mapScale"];
  s.timeStep = j["simulation"]["timeStep"];
  s.targetFPS = j["simulation"]["targetFPS"];
  s.wallFriction = j["simulation"]["world"]["friction"];
  s.wallRestitution = j["simulation"]["world"]["restitution"];

  /*
  s.wallFriction  = j["simulation"]["world"]["wallFriction"];
  s.wallElacticity = j["simulation"]["world"]["wallElacticity"];
  s.wallDamping = j["simulation"]["world"]["wallDamping"];
  s.shipDamping = j["simulation"]["ship"]["damping"];
  */

  return s;
}
