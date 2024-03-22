#pragma once

#include <DirectXMath.h>

#include <list>
#include <vector>

#include "SimpleMath.h"
#include "lookup-list.h"
#include "particle.h"
#include "settings.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

class MarchingCube {
 public:
  MarchingCube() = delete;
  MarchingCube(const Settings& settings, const std::vector<Particle>& particles)
      : m_particles(particles),
        m_width(settings.marchingCubeWidth),
        m_worldLen(Vector3(settings.cubeNum.x, settings.cubeNum.y,
                           settings.cubeNum.z) *
                   settings.cubeLen),
        m_worldOffset(settings.pos),
        m_num(XMINT3(std::ceil(m_worldLen.x / m_width),
                     std::ceil(m_worldLen.y / m_width),
                     std::ceil(m_worldLen.z / m_width))),
        m_particle_radius(settings.h) {
    int size = m_num.x * m_num.y * m_num.z;
    m_voxel_grid.data.resize(size);
    m_voxel_grid.resolution = m_num;
    update_grid();
  }

  void march(std::vector<Vector3>& vertex);

 private:
  void update_grid();
  void march_cube(XMINT3 pos, std::vector<Vector3>& vertex);
  bool check_collision(Vector3 point);
  const int* get_triangulations(UINT x, UINT y, UINT z);

  struct VoxelGrid {
    std::vector<bool> data;
    XMINT3 resolution;

    bool get(size_t x, size_t y, size_t z) {
      return data[x + y * resolution.x + z * resolution.x * resolution.y];
    }
  };
  VoxelGrid m_voxel_grid;
  const std::vector<Particle>& m_particles;
  float m_width;
  Vector3 m_worldLen;
  Vector3 m_worldOffset;
  XMINT3 m_num;
  float m_particle_radius;
};
