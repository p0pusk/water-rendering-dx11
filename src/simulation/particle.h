#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct Particle {
  Vector3 position;
  float density;
  float pressure;
  Vector3 force;
  Vector3 velocity;
  UINT hash;
};
