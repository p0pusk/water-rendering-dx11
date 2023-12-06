#pragma once

#include <DirectXMath.h>

#include <list>
#include <vector>

#include "SimpleMath.h"
#include "lookup-list.h"
#include "particle.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

class MarchingCube {
 public:
  MarchingCube() = delete;
  MarchingCube(Vector3 worldLen, Vector3 worldPos, float width,
               std::vector<Particle>& particles, float particle_radius)
      : m_worldLen(worldLen),
        m_worldPos(worldPos),
        m_width(width),
        m_particle_radius(particle_radius),
        m_num(
            XMINT3(worldLen.x / width, worldLen.y / width, worldLen.z / width)),
        m_particles(particles) {
    create_grid();
  }

  ~MarchingCube();

  void march(std::vector<Vector3>& vertex);

 private:
  void create_grid();
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
  std::vector<Particle>& m_particles;
  Vector3 m_worldLen;
  Vector3 m_worldPos;
  XMINT3 m_num;
  float m_particle_radius;
  float m_width;
};
