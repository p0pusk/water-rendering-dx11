#pragma once

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <stdint.h>

#include <vector>

#include "particle.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

const uint32_t TABLE_SIZE = 262144;
const uint32_t NO_PARTICLE = 0xFFFFFFFF;

class NeighbourHash {
 public:
  NeighbourHash() = default;

  uint32_t getHash(const XMINT3& cell);
  XMINT3 getCell(const Particle& p, float h, const Vector3& offset);
  void createTable(const std::vector<Particle>& sortedParticles);

  std::vector<uint32_t> m;
};
