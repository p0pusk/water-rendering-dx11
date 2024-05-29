#pragma once

#include <SimpleMath.h>

using namespace DirectX::SimpleMath;
using namespace DirectX;

struct Particle {
  Vector3 position;
  float density;
  Vector3 force;
  float pressure;
  Vector3 velocity;
  UINT hash;
  Vector3 normal;
};

struct PBParticle {
  XMFLOAT3 position;
  XMFLOAT3 velocity;
  XMFLOAT3 force;
  XMFLOAT3 predictedPosition;
  XMFLOAT3 deltaP;
  float lambda;
  float density;
  float mass;
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
  UINT neighbours;
};
