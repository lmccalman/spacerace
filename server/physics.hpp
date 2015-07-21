#pragma once

#include "Eigen/Dense"

const uint STATE_LENGTH = 6;
const uint CONTROL_LENGTH = 2;

using StateMatrix = Eigen::Matrix<float,Eigen::Dynamic,STATE_LENGTH>;
using ControlMatrix = Eigen::Matrix<float, Eigen::Dynamic, CONTROL_LENGTH>;
using StateVector = Eigen::Matrix<float,1,STATE_LENGTH>;
using ControlVector = Eigen::Matrix<float,1,CONTROL_LENGTH>;

// State Vector
// ux uy vx vy  theta omega 
// 0  1  2  3   4     5  

// Control Vector
// T_l T_r
// 0   1

void collisionFreeDerivative(const StateVector& s, const ControlVector& c, StateVector& d, float d_l, float d_r)
{
  //ux_dot = vx
  d(0) = s(2);
  //uy_dot = vy
  d(1) = s(3);
  //vx_dot = T_l cos\theta - D_lv_x\sqrt{v_x^2 + v_y^2}
  d(2) = c(0) * cos(s(4)) - d_l * s(2) * sqrt(s(2)*s(2) + s(3)*s(3));
  //vy_dot = T_l sin\theta - D_lv_y\sqrt{v_x^2 + v_y^2}
  d(3) = c(0) * sin(s(4)) - d_l * s(3) * sqrt(s(2)*s(2) + s(3)*s(3));
  //theta_dot = \omega
  d(4) = s(5);
  //omega_dot = T_r - D_r \omega^2
  d(5) = c(1) - d_r * s(5) * s(5);
}

void eulerTimeStep(StateMatrix& s, ControlMatrix& c, float d_l, float d_r, float h)
{
  // first order euler method
  uint n = s.rows();
  StateVector d;
  #pragma omp parallel for private(d)
  for (uint i=0;i<n;i++)
  {
    collisionFreeDerivative(s.row(i), c.row(i), d, d_l, d_r);
    s.row(i) = s.row(i) + d*h;
  }
}

