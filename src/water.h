#pragma once

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <iostream>
#include <vector>

using namespace DirectX;

typedef std::vector < std::vector < std::vector <float>>> Cube;
typedef std::vector < std::vector < std::vector <DirectX::SimpleMath::Vector3>>> VectorCube;

class WaterGrid {
  int m_iteration = 0;
  int m_x;
  int m_y;
  int m_z;

  float m_dx;
  float m_dy;
  float m_dz;

  // fluid density
  float m_ro = 1000;

  DirectX::SimpleMath::Vector3 m_forces;

  Cube m_pressure;

  // velocity x component
  Cube m_u;
  // velocity y component
  Cube m_v;
  // velocity z component
  Cube m_w;
  
  Cube m_F;
  Cube m_G;
  Cube m_H;

  double m_dt;
  double m_vMax = 1;
  double m_kCFL = 1;


  WaterGrid(int x, int y, int z, double dh)
    :m_x(x), m_y(y), m_z(z), m_dx(dh) {

    Init();

    m_dt = m_kCFL * m_dx / m_vMax;
  }


  void Init();
  void TimeStep();
  void PressureSolve();
  SimpleMath::Vector3 VelocityDerivative(SimpleMath::Vector3 u);
};
