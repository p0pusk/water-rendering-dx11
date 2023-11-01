#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct Particle {
  Vector3 position;
  float density;
  float mass;
  float pressure;
  Vector3 pressureGrad;
  Vector3 viscosity;
  Vector3 force;
  Vector3 velocity;
  uint32_t hash;
};
