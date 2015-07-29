#pragma once

// State Vector
// ux uy vx vy  theta omega
// 0  1  2  3   4     5  

// Control Vector
// T_l T_r
// 0   1

// Hidden State // used for collisions, gravity, elastic walls etc
// F_x F_y Tau
//  0   1   2

void derivative(const StateVector& s, StateVector& d, const ControlVector& c, const SimulationParameters& params, const Map & m)
{
   //ux_dot = vx + f_x
   d(0) = s(2);
   //uy_dot = vy + f_y
   d(1) = s(3); 
   //vx_dot = T_l cos\theta - D_lv_x\sqrt{v_x^2 + v_y^2}
   d(2) = c(0) * cos(s(4)) - params.linearDrag * s(2) * sqrt(s(2)*s(2) + s(3)*s(3));
   //vy_dot = T_l sin\theta - D_lv_y\sqrt{v_x^2 + v_y^2}
   d(3) = c(0) * sin(s(4)) - params.linearDrag * s(3) * sqrt(s(2)*s(2) + s(3)*s(3));
   //theta_dot = \omega
   d(4) = s(5);
   //omega_dot = T_r - D_r \omega^2
   d(5) = c(1) - params.rotationalDrag * s(5) * s(5);
}

void rk4TimeStep(StateMatrix& s, const ControlMatrix& c, const SimulationParameters& params, const Map & m)
{
  uint n = s.rows();
  StateVector d;
  for (uint i=0;i<n;i++)
  {
    derivative(s.row(i), d, c.row(i), params, m);
    //TODO something rk4ish with d
    s.row(i) = s.row(i) + d*params.timeStep;
  }
}

SimulationParameters readParams(const json& j)
{
  SimulationParameters s;
  
  s.linearThrust =  j["simulation"]["ship"]["linearThrust"];
  s.rotationalThrust = j["simulation"]["ship"]["rotationalThrust"];
  s.shipRadius = j["simulation"]["ship"]["radius"];
  s.shipFriction = j["simulation"]["ship"]["friction"];
  s.shipElacticity = j["simulation"]["ship"]["elacticity"];
  s.shipDamping = j["simulation"]["ship"]["damping"];

  s.linearDrag = j["simulation"]["world"]["linearDrag"];
  s.rotationalDrag = j["simulation"]["world"]["rotationalDrag"]; 
  s.wallFriction  = j["simulation"]["world"]["wallFriction"];
  s.wallElacticity = j["simulation"]["world"]["wallElacticity"];
  s.wallDamping = j["simulation"]["world"]["wallDamping"];
  s.pixelSize = j["simulation"]["world"]["pixelSize"];

  s.timeStep = j["simulation"]["timeStep"];
  s.integrationSteps = j["simulation"]["integrationSteps"];
  s.targetFPS = j["simulation"]["targetFPS"];

  return s;
}
