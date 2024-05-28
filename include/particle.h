#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct Particle {
  Vector3 position;
  float density;
  Vector3 force;
  float pressure;
  Vector3 velocity;
  UINT hash;
  Vector3 normal;
};

struct Potential {
  float waveCrest;
  float trappedAir;
  float energy;
  float curvature;
};

struct DiffuseParticle {
  Vector3 position;
  Vector3 velocity;
  // 0 - spray, 1 - foam, 2 - bubbles
  UINT type;
  UINT origin;
  float lifetime;
};
