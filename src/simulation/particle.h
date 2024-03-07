#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct Particle {
  Vector3 position;
  float density;
  Vector3 pressureGrad;
  float pressure;
  Vector4 viscosity;
  Vector4 force;
  Vector3 velocity;
  UINT hash;
};
