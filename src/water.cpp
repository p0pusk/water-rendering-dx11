#include "water.h"

// du/dt = F - u - 1/ro grad(p)

using namespace DirectX;


void WaterGrid::Init() {
  m_pressure.resize(m_x + 2);
  for (int i = 0; i < m_x + 2; i++) {
    m_pressure[i].resize(m_y + 2);
    for (int j = 0; j < m_y + 2; j++) {
      m_pressure[i][j].resize(m_z + 2);
    }
  }

  m_u.resize(m_x + 2);
  m_v.resize(m_x + 2);
  m_w.resize(m_x + 2);
  for (int i = 0; i < m_x + 2; i++) {
    m_u[i].resize(m_y + 2);
    m_v[i].resize(m_y + 2);
    m_w[i].resize(m_y + 2);
    for (int j = 0; j < m_y + 2; j++) {
      m_u[i][j].resize(m_z + 2);
      m_v[i][j].resize(m_z + 2);
      m_w[i][j].resize(m_z + 2);
    }
  }

  // boundary values no-slip
  for (int y = 1; y < m_y + 1; y++) {
    for (int z = 1; z < m_z + 1; z++) {
      m_u[0][y][z] = 0;
      m_u[m_x+1][y][z] = 0;
      m_v[0][y][z] = -m_v[1][y][z];
      m_v[m_x+1][y][z] = -m_v[m_x][y][z];
      m_w[0][y][z] = -m_w[1][y][z];
      m_w[m_x+1][y][z] = -m_w[m_x][y][z];
    }
  }

  for (int x = 1; x < m_x + 1; x++) {
    for (int z = 1; z < m_z + 1; z++) {
      m_u[x][0][z] = -m_u[x][1][z];
      m_u[x][m_y+1][z] = -m_u[x][m_y][z];
      m_v[x][0][z] = 0;
      m_v[x][m_y+1][z] = 0;
      m_w[x][0][z] = -m_w[x][1][z];
      m_w[x][m_y+1][z] = -m_w[x][m_y][z];
    }
  }

  for (int x = 1; x < m_x + 1; x++) {
    for (int y = 1; y < m_y + 1; y++) {
      m_u[x][y][0] = -m_u[x][y][1];
      m_u[x][y][m_z + 1] = -m_u[x][y][m_z];
      m_v[x][y][0] = -m_v[x][y][1];
      m_v[x][y][m_z + 1] = -m_v[x][y][m_z];
      m_w[x][y][0] = 0;
      m_w[x][y][m_z + 1] = 0;
    }
  }
}

SimpleMath::Vector3 WaterGrid::VelocityDerivative(SimpleMath::Vector3 u) {
  return m_forces - u - SimpleMath::Vector3::One * 1 / m_ro;
}

void WaterGrid::TimeStep() {
  for (int x = 1; x <= m_x; x++) {
    for (int y = 1; y <= m_y; y++) {
      for (int z = 1; z <= m_z; z++) {

        float d2udx2 = (m_u[x + 1][y][z] - 2 * m_u[x][y][z] + m_u[x - 1][y][z]) / m_dx / m_dx;
        float d2udy2 = (m_u[x][y + 1][z] - 2 * m_u[x][y][z] + m_u[x][y - 1][z]) / m_dy / m_dy;
        float d2udz2 = (m_u[x][y][z + 1] - 2 * m_u[x][y][z] + m_u[x][y][z - 1]) / m_dz / m_dz;
        
        // TODO: implement gamma calcuation
        float gamma = 0;
        float du2dx = 1 / m_dx / 4 * (pow((m_u[x][y][z] + m_u[x + 1][y][z]), 2) - pow(m_u[x - 1][y][z] + m_u[x][y][z], 2)) +
          gamma * 1 / m_dx / 4 * (abs(m_u[x][y][z] + m_u[x + 1][y][z]) * (m_u[x][y][z] - m_u[x + 1][y][z])
            - abs(m_u[x - 1][y][z] + m_u[x][y][z]) * (m_u[x - 1][y][z] - m_u[x][y][z]));

        float dv2dy = 1 / m_dy / 4 * (pow((m_v[x][y][z] + m_v[x][y + 1][z]), 2) - pow(m_v[x][y - 1][z] + m_v[x][y][z], 2)) +
          gamma * 1 / m_dy / 4 * (abs(m_v[x][y][z] + m_v[x][y + 1][z]) * (m_v[x][y][z] - m_v[x][y + 1][z])
            - abs(m_v[x][y - 1][z] + m_v[x][y][z]) * (m_v[x][y - 1][z] - m_v[x][y][z]));

        float dw2dz = 1 / m_dz / 4 * (pow((m_w[x][y][z] + m_w[x][y][z + 1]), 2) - pow(m_w[x][y][z - 1] + m_w[x][y][z], 2)) +
          gamma * 1 / m_dz / 4 * (abs(m_w[x][y][z] + m_w[x][y][z + 1]) * (m_w[x][y][z] - m_w[x][y][z + 1])
            - abs(m_w[x][y][z - 1] + m_w[x][y][z]) * (m_w[x][y][z - 1] - m_w[x][y][z]));


        float duvdx = 1 / m_dx / 4 *
          ((m_u[x][y][z] + m_u[x][y+1][z]) * (m_v[x][y][z] + m_v[x+1][y][z]) -
          (m_u[x-1][y][z] + m_u[x-1][y+1][z]) * (m_v[x-1][y][z] + m_v[x][y][z]))
          + gamma * 1 / m_dx / 4 *
          (abs(m_u[x][y][z] + m_u[x][y+1][z]) * (m_v[x][y][z] - m_v[x + 1][y][z])
            - abs(m_u[x - 1][y][z] + m_u[x-1][y+1][z]) * (m_v[x - 1][y][z] - m_v[x][y][z]));

        float duvdy = 1 / m_dy / 4 *
          ((m_v[x][y][z] + m_v[x + 1][y][z]) * (m_u[x][y][z] + m_u[x][y + 1][z]) -
            (m_v[x][y - 1][z] + m_v[x + 1][y - 1][z]) * (m_u[x][y - 1][z] + m_u[x][y][z]))
          + gamma * 1 / m_dy / 4 *
          (abs(m_v[x][y][z] + m_v[x][y + 1][z]) * (m_u[x][y][z] - m_u[x][y + 1][z])
            - abs(m_v[x][y - 1][z] + m_v[x + 1][y - 1][z]) * (m_u[x][y - 1][z] - m_u[x][y][z]));



      }
    }
  }
}

void WaterGrid::PressureSolve() {
  std::vector<float> A(m_x + m_y + m_z);
  for (int x = 1; x <= m_x; x++) {
  }
}
