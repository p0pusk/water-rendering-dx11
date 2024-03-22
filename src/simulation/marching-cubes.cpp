#include "marching-cubes.h"

#include <vector>

#include "SimpleMath.h"

using namespace std;

void MarchingCube::update_grid() {
  for (int i = 0; i < m_voxel_grid.data.size(); i++) {
    m_voxel_grid.data[i] = false;
  }

  for (int i = 0; i < m_num.x - 1; ++i) {
    for (int j = 0; j < m_num.y - 1; ++j) {
      for (int k = 0; k < m_num.z - 1; ++k) {
        int index = i + (k * m_num.y + j) * m_num.x;
        m_voxel_grid.data[index] =
            check_collision(Vector3(i, j, k) * m_width + m_worldOffset);
      }
    }
  }
}

void MarchingCube::march(std::vector<Vector3>& vertex) {
  vertex.clear();
  update_grid();

  for (int y = 0; y < m_num.y - 1; y++) {
    for (int x = 0; x < m_num.x - 1; x++) {
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
    assert(edge_index < 12);
    XMINT3 p1 = POINTS_TABLE[point_indecies[0]];
    XMINT3 p2 = POINTS_TABLE[point_indecies[1]];

    Vector3 worldP1 =
        (Vector3(p1.x, p1.y, p1.z) + Vector3(pos.x, pos.y, pos.z)) * m_width +
        m_worldOffset;
    Vector3 worldP2 =
        (Vector3(p2.x, p2.y, p2.z) + Vector3(pos.x, pos.y, pos.z)) * m_width +
        m_worldOffset;

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

  UINT idx = 0;
  idx |= (UINT)(!m_voxel_grid.get(x, y, z)) << 0;
  idx |= (UINT)(!m_voxel_grid.get(x, y, z + 1)) << 1;
  idx |= (UINT)(!m_voxel_grid.get(x + 1, y, z + 1)) << 2;
  idx |= (UINT)(!m_voxel_grid.get(x + 1, y, z)) << 3;
  idx |= (UINT)(!m_voxel_grid.get(x, y + 1, z)) << 4;
  idx |= (UINT)(!m_voxel_grid.get(x, y + 1, z + 1)) << 5;
  idx |= (UINT)(!m_voxel_grid.get(x + 1, y + 1, z + 1)) << 6;
  idx |= (UINT)(!m_voxel_grid.get(x + 1, y + 1, z)) << 7;

  assert(idx < 256);

  return TRIANGULATIONS[idx];
}

bool MarchingCube::check_collision(Vector3 point) {
  float min_dist = m_particle_radius;
  for (auto& p : m_particles) {
    float d = Vector3::Distance(point, p.position);
    if (d < min_dist) {
      return true;
    }
  }
  return false;
}
