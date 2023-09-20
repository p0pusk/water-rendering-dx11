#include <DirectXMath.h>
#include <iostream>
#include <vector>

typedef std::vector<std::vector<std::vector<float>>> Cube;

class WaterGrid {
  int m_iteration = 0;
  int m_x;
  int m_y;
  int m_z;
  double m_h;

  Cube m_pressure;
  Cube m_velocityX;
  Cube m_velocityY;
  Cube m_velocityZ;

  double m_dt;
  double m_vMax = 1;
  double m_kCFL = 1;


  WaterGrid(int x, int y, int z, double h)
    :m_x(x), m_y(y), m_z(z), m_h(h) {

    InitGrid();

    m_dt = m_kCFL * m_h / m_vMax;
  }


  void InitGrid() {
    m_pressure.resize(m_x);
    for (int i = 0; i < m_x; i++) {
      m_pressure[i].resize(m_y);
      for (int j = 0; j < m_y; j++) {
        m_pressure[i][j].resize(m_z);
      }
    }

    m_velocityX.resize(m_x);
    for (int i = 0; i < m_x + 1; i++) {
      m_velocityX[i].resize(m_y);
      for (int j = 0; j < m_y; j++) {
        m_velocityX[i][j].resize(m_z);
      }
    }

    m_velocityY.resize(m_x);
    for (int i = 0; i < m_x; i++) {
      m_velocityY[i].resize(m_y);
      for (int j = 0; j < m_y + 1; j++) {
        m_velocityY[i][j].resize(m_z);
      }
    }

    m_velocityZ.resize(m_x);
    for (int i = 0; i < m_x; i++) {
      m_velocityZ[i].resize(m_y);
      for (int j = 0; j < m_y; j++) {
        m_velocityZ[i][j].resize(m_z + 1);
      }
    }
  }
};