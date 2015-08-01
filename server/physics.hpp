#pragma once
#include <cmath>
#include "Eigen/Dense"
#include <vector>
#include <iostream>

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

  uint ix = std::min(uint(fx), w-2);
  uint iy = std::min(uint(fy), h-2);

  // Compute linear interp alphas
  float alphax = fx - float(ix);
  float alphay = fy - float(iy);
  
  wall_dist = 0.;
  norm_x = 0.;
  norm_y = 0.;

  for (int a=0; a<2; a++)
  {
    alphax = 1. - alphax;
    for (int b=0; b<2; b++)
    {
      alphay = 1. - alphay;
      
      wall_dist += alphax * alphay * map.wallDistance(iy, ix);
      norm_x += alphax * alphay * map.wallNormalx(iy, ix);
      norm_y += alphax * alphay * map.wallNormaly(iy, ix);

      iy += 1;
    }
    ix += 1;
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
  float mu = params.friction;  // 0.05;  // friction coefficient between ships
  float mu_wall = 0.25*params.friction;  //wallFriction;  // 0.01;  // wall friction parameter
  float diameter = 2.*rad;  // rad(i) + rad(j) for any i, j
  float inertia_mass_ratio = 0.25;
  // rotationalThrust Order +- 10 
  // linearThrust Order +100
  // mapscale order 10 - thats params.pixelsize
  // Accumulate forces and torques into these:
  uint n = states.rows();
  Eigen::MatrixXd f = Eigen::MatrixXd::Zero(n, 2);
  Eigen::VectorXd trq = Eigen::VectorXd::Zero(n);
  
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
    trq(i) -= spin_drag_ratio*cd_a_rho*w_i*abs(w_i)*rad*rad;

    // 3. Inter-ship collisions
    for (uint j=i+1; j<n; j++) {
      Eigen::Vector2f pos_j;
      pos_j(0) = states(j,0);
      pos_j(1) = states(j,1);
      Eigen::Vector2f vel_j;
      vel_j(0) = states(j,2);
      vel_j(1) = states(j,3);
      float theta_j = states(j,4);
      float w_j = states(j,5);

      Eigen::Vector2f dP = pos_j - pos_i;
      float dist = dP.norm() + eps;
      
      if (dist < diameter) {
        // we have a collision interaction
        
        // A. Direct collision: apply linear spring normal force
        float f_magnitude = (diameter - dist) * k_elastic;
        Eigen::Vector2f f_norm = f_magnitude * dP;
        f(i, 0) -= f_norm(0);
        f(i, 1) -= f_norm(1);
        f(j, 0) += f_norm(0);
        f(j, 1) += f_norm(1);

        // B. Surface friction: approximate spin effects
        Eigen::Vector2f perp;  // surface tangent pointing +theta direction
        perp(0) = -dP(1) / dist;
        perp(1) = dP(0) / dist;
        
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
      // Spring force
      float f_norm_mag = -dist*k_elastic;
      f(i, 0) += f_norm_mag * norm_x;
      f(i, 1) += f_norm_mag * norm_y;

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

  // Compose the vector of derivatives:
  for (int i=0; i<n; i++)
  {
    // x_dot = vx
    derivs(i, 0) = states(i, 2);
    // y_dot = vy
    derivs(i, 1) = states(i, 3); 
    // vx_dot = fx / m
    derivs(i, 2) = f(i, 0) / masses(i);
    // vy_dot = fy / m
    derivs(i, 3) = f(i, 1) / masses(i);
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
  s.friction = j["simulation"]["world"]["friction"];
  s.elasticity = j["simulation"]["world"]["elasticity"];
  s.mapScale = j["simulation"]["world"]["mapScale"];
  s.timeStep = j["simulation"]["timeStep"];
  s.targetFPS = j["simulation"]["targetFPS"];
  s.integrationSteps = j["simulation"]["integrationSteps"];
  
  /*
  s.wallFriction  = j["simulation"]["world"]["wallFriction"];
  s.wallElacticity = j["simulation"]["world"]["wallElacticity"];
  s.wallDamping = j["simulation"]["world"]["wallDamping"];
  s.shipDamping = j["simulation"]["ship"]["damping"];
  */

  return s;
}
