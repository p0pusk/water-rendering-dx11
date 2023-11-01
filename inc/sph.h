#pragma once

#include <stdint.h>

#include "neighbour-hash.h"

#define _USE_MATH_DEFINES

#include <SimpleMath.h>
#include <math.h>

#include <iostream>
#include <vector>

using namespace DirectX::SimpleMath;
using namespace DirectX;

class SPH {
 public:
  struct Props {
    Vector3 pos = Vector3::Zero;
    XMINT3 cubeNum = XMINT3(5, 5, 5);
    float cubeLen = 0.2f;
    float h = 0.15f;
    float mass = 0.02f;
    float dynamicViscosity = 1.1f;
    float particleRadius = 0.1f;
    float dampingCoeff = 0.5f;
  };

  std::vector<Particle> m_particles;
  int n = 0;

  SPH(Props& props) : m_props(props) {
    poly6 = 315.0f / (64.0f * M_PI * pow(props.h, 9));
    spikyGrad = -45.0f / (M_PI * pow(props.h, 6));
    spikyLap = 45.0f / (M_PI * pow(props.h, 6));
    h2 = props.h * props.h;
  };

  void Update(float dt);
  void Init();

  Props m_props;

 private:
  float poly6;
  float spikyGrad;
  float spikyLap;

  NeighbourHash m_hash;

  float h2;

  void UpdateDensity();
  void CheckBoundary(Particle& p);
};
