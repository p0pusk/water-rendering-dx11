#include "marching-cubes.h"

#include <vector>

#include "SimpleMath.h"

using namespace std;

void MarchingCube::create_grid() {
  int size = m_num.x * m_num.y * m_num.z;
  m_voxel_grid.data.resize(size);
  m_voxel_grid.resolution = m_num;

  for (int x = 0; x < m_num.x; x++) {
    for (int y = 0; y < m_num.y; y++) {
      for (int z = 0; z < m_num.z; z++) {
        int index = x + (z * m_num.y + y) * m_num.x;
        Vector3 worldPos = m_width * Vector3(x, y, z) + m_worldPos;
        m_voxel_grid.data[index] = check_collision(worldPos);
      }
    }
  }
}

void MarchingCube::march(std::vector<Vector3>& vertex) {
  vertex.clear();

  for (int x = 0; x < m_num.x; x++) {
    for (int y = 0; y < m_num.y; y++) {
      for (int z = 0; z < m_num.z; z++) {
        int index = x + (z * m_num.y + y) * m_num.x;
        Vector3 worldPos = m_width * Vector3(x, y, z) + m_worldPos;
        m_voxel_grid.data[index] = check_collision(worldPos);
      }
    }
  }

  for (int x = 0; x < m_num.x - 1; x++) {
    for (int y = 0; y < m_num.y - 1; y++) {
      for (int z = 0; z < m_num.z - 1; z++) {
        march_cube(XMINT3(x, y, z), vertex);
      }
    }
  }
}

void MarchingCube::march_cube(XMINT3 pos, std::vector<Vector3>& vertex) {
  auto trianglulation = get_triangulations(pos.x, pos.y, pos.z);
  for (int i = 0; i < 15; i++) {
    int edge_index = trianglulation[i];
    if (edge_index < 0) {
      break;
    }

    auto point_indecies = EDGES_TABLE[edge_index];
    XMINT3 p1 = POINTS_TABLE[point_indecies[0]];
    XMINT3 p2 = POINTS_TABLE[point_indecies[1]];

    Vector3 worldP1 =
        (Vector3(p1.x, p1.y, p1.z) + Vector3(pos.x, pos.y, pos.z)) * m_width +
        m_worldPos;
    Vector3 worldP2 =
        (Vector3(p2.x, p2.y, p2.z) + Vector3(pos.x, pos.y, pos.z)) * m_width +
        m_worldPos;

    Vector3 final_point = (worldP1 + worldP2) / 2;
    vertex.push_back(final_point);
  }
}

const int* MarchingCube::get_triangulations(UINT x, UINT y, UINT z) {
  /*

  indecies:
           (5)-------(6)
         .  |      .  |
      (4)-------(7)   |
       |    |    |    |
       |   (1)---|---(2)
       | .       | .
      (0)-------(3)

  */

  uint8_t idx = 0b00000000;
  idx |= (uint8_t)(!m_voxel_grid.get(x, y, z)) << 0;
  idx |= (uint8_t)(!m_voxel_grid.get(x, y, z + 1)) << 1;
  idx |= (uint8_t)(!m_voxel_grid.get(x + 1, y, z + 1)) << 2;
  idx |= (uint8_t)(!m_voxel_grid.get(x + 1, y, z)) << 3;
  idx |= (uint8_t)(!m_voxel_grid.get(x, y + 1, z)) << 4;
  idx |= (uint8_t)(!m_voxel_grid.get(x, y + 1, z + 1)) << 5;
  idx |= (uint8_t)(!m_voxel_grid.get(x + 1, y + 1, z + 1)) << 6;
  idx |= (uint8_t)(!m_voxel_grid.get(x + 1, y + 1, z)) << 7;

  return TRIANGULATIONS[idx];
}

bool MarchingCube::check_collision(Vector3 point) {
  float min_dist = m_particle_radius;
  for (auto& p : m_particles) {
    float d = Vector3::Distance(point, p.position);
    if (d <= min_dist) {
      return true;
    }
  }
  return false;
}
