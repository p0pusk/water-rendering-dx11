#pragma once

#define _USE_MATH_DEFINES

#include <SimpleMath.h>
#include <math.h>

#include <iostream>
#include <vector>

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
};

class SPH {
 public:
  std::vector<Particle> m_particles;

  SPH(int n) : boxH(0.25f) {
    m_particles.resize(n);

    h = 0.15f;
    poly6 = 315.0f / (64.0f * M_PI * pow(h, 9));
    spikyGrad = -45.0f / (M_PI * pow(h, 6));
    spikyLap = 45.0f / (M_PI * pow(h, 6));

    h2 = h * h;
  };

  void Update(float dt);
  void Init();

 public:
  float poly6;
  float spikyGrad;
  float spikyLap;

  // pressure constant
  float dynamicViscosity = 3.5f;

  float h;
  float h2;
  int n = 0;
  float boxH;

  void CheckBoundary(Particle& p);
  float GradKernel(float r, float h);
  float Kernel(float r, float h);
  float LagrangianKernel(float r, float h);
};