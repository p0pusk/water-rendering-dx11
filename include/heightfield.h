#pragma once

#include <DirectXMath.h>
#include <SimpleMath.h>

#include <vector>

using namespace DirectX;
using namespace DirectX::SimpleMath;

class HeightField {
 public:
  struct Props {
    Vector3 pos = Vector3::Zero;
    XMINT2 gridSize = XMINT2(5, 5);
    float w = 0.01f;
    float waveSpeed = 1;
    float height = 5;
    float g = 9.8f;
    float density = 1000;
  };

  struct Cell {
    float velocity;
    float height;
  };

  struct ObjectCell {
    float mass;
    float height;
    float prev_height;
    float velocity;
  };

  HeightField() = delete;
  HeightField(const Props& props);
  void Update(float dt);
  void GetRenderData(std::vector<Vector3>& vertecies,
                     std::vector<UINT16>& indecies);
  void StartImpulse(int x, int z, float value, float radius);
  void StopImpulse(int x, int z, float radius);

  std::vector<std::vector<Cell>> m_grid;
  std::vector<std::vector<ObjectCell>> m_objects;
  int m_numX;
  int m_numZ;
  Props m_props;
};
