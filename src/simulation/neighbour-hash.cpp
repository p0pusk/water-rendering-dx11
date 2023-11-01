#include "neighbour-hash.h"

#include <stdint.h>

#include <vector>

#include "SimpleMath.h"
#include "particle.h"
#include "sph.h"

uint32_t NeighbourHash::getHash(const XMINT3 &cell) {
  return ((uint32_t)(cell.x * 73856093) ^ (uint32_t)(cell.y * 19349663) ^
          (uint32_t)(cell.z * 83492791)) %
         TABLE_SIZE;
}

XMINT3 NeighbourHash::getCell(const Particle &p, float h,
                              const Vector3 &offset) {
  return XMINT3((p.position.x - offset.x) / h, (p.position.y - offset.y) / h,
                (p.position.z - offset.z) / h);
}

void NeighbourHash::createTable(const std::vector<Particle> &sortedParticles) {
  m.resize(TABLE_SIZE);
  for (size_t i = 0; i < TABLE_SIZE; ++i) {
    m[i] = NO_PARTICLE;
  }

  uint32_t prevHash = NO_PARTICLE;
  for (size_t i = 0; i < sortedParticles.size(); ++i) {
    uint32_t currentHash = sortedParticles[i].hash;
    if (currentHash != prevHash) {
      m[currentHash] = i;
      prevHash = currentHash;
    }
  }
}
