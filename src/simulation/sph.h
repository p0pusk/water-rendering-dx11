#pragma once

#include "particle.h"
#include "pch.h"
#include "settings.h"

class Sph {
 public:
  Sph(const Settings& settings);
  void Init(std::vector<Particle>& particles);
  void Update(float dt, std::vector<Particle>& particles);
  void CheckBoundary(Particle& p);

  UINT GetHash(XMINT3 cell);
  XMINT3 GetCell(Vector3 pos);

 private:
  const Settings& m_settings;
  std::unordered_multimap<UINT, Particle const*> m_hashM;

  float poly6;
  float h2;
  float spikyGrad;
  float spikyLap;

  static const UINT TABLE_SIZE = 262144;
};
