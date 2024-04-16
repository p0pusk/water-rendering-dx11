#include "sph.h"

#include <vector>

Sph::Sph(const Settings& settings) : m_settings(settings) {
  poly6 = 315.0f / (64.0f * M_PI * pow(settings.h, 9));
  spikyGrad = -45.0f / (M_PI * pow(settings.h, 6));
  spikyLap = 45.0f / (M_PI * pow(settings.h, 6));
  h2 = settings.h * settings.h;
}

UINT Sph::GetHash(XMINT3 cell) {
  return ((cell.x * 73856093) ^ (cell.y * 19349663) ^ (cell.z * 83492791)) %
         m_settings.TABLE_SIZE;
}

XMINT3 Sph::GetCell(Vector3 position) {
  auto res = (position - m_settings.pos) / m_settings.h;
  return XMINT3(res.x, res.y, res.z);
}

void Sph::Init(std::vector<Particle>& particles) {
  const float& h = m_settings.h;
  const XMINT3& cubeNum = m_settings.cubeNum;
  float separation = h - 0.01f;
  Vector3 offset = {h, h, h};

  particles.resize(cubeNum.x * cubeNum.y * cubeNum.z);

  for (int x = 0; x < cubeNum.x; x++) {
    for (int y = 0; y < cubeNum.y; y++) {
      for (int z = 0; z < cubeNum.z; z++) {
        size_t particleIndex = x + (y + z * cubeNum.y) * cubeNum.x;

        Particle& p = particles[particleIndex];
        p.position.x = m_settings.pos.x + x * separation + offset.x;
        p.position.y = m_settings.pos.y + y * separation + offset.y;
        p.position.z = m_settings.pos.z + z * separation + offset.z;
        p.velocity = Vector3::Zero;
        p.hash = GetHash(GetCell(p.position));
      }
    }
  }

  std::sort(particles.begin(), particles.end(),
            [](Particle& a, Particle& b) { return a.hash < b.hash; });
}

void Sph::Update(float dt, std::vector<Particle>& particles) {
  const float& h = m_settings.h;

  m_hashM.clear();

  // init hash map
  for (auto& p : particles) {
    m_hashM.insert(std::make_pair(GetHash(GetCell(p.position)), &p));
  }

  // Compute density
  for (auto& p : particles) {
    p.density = 0;
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        for (int k = -1; k <= 1; k++) {
          Vector3 localPos = p.position + Vector3(i, j, k) * h;
          UINT key = GetHash(GetCell(localPos));
          int count = m_hashM.count(key);
          auto it = m_hashM.find(key);
          for (int c = 0; c < count; c++) {
            float d2 =
                Vector3::DistanceSquared(p.position, it->second->position);
            if (d2 < h2) {
              p.density += m_settings.mass * poly6 * pow(h2 - d2, 3);
            }
            it++;
          }
        }
      }
    }
  }

  // Compute pressure
  for (auto& p : particles) {
    float k = 1;
    float p0 = 1000;
    p.pressure = k * (p.density - p0);
  }

  // Compute pressure force
  for (auto& p : particles) {
    p.pressureGrad = Vector3::Zero;
    p.force = Vector4(0, -9.8f * p.density, 0, 0);
    p.viscosity = Vector4::Zero;

    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        for (int k = -1; k <= 1; k++) {
          Vector3 localPos = p.position + Vector3(i, j, k) * h;
          UINT key = GetHash(GetCell(localPos));
          int count = m_hashM.count(key);
          auto it = m_hashM.find(key);
          for (int c = 0; c < count; c++) {
            float d = Vector3::Distance(p.position, it->second->position);
            Vector3 dir = (p.position - it->second->position);
            dir.Normalize();
            if (d < h) {
              p.pressureGrad +=
                  dir * m_settings.mass * (p.pressure + it->second->pressure) /
                  (2 * it->second->density) * spikyGrad * std::pow(h - d, 2);
              p.viscosity += m_settings.dynamicViscosity * m_settings.mass *
                             Vector4(it->second->velocity - p.velocity) /
                             it->second->density * spikyLap *
                             (m_settings.h - d);
            }
            it++;
          }
        }
      }
    }
  }

  // TimeStep
  for (auto& p : particles) {
    p.velocity +=
        dt * (p.pressureGrad + Vector3(p.force + p.viscosity)) / p.density;
    p.position += dt * Vector3(p.velocity);

    // boundary condition
    CheckBoundary(p);
  }
}

void Sph::CheckBoundary(Particle& p) {
  float h = m_settings.h;
  float dampingCoeff = m_settings.dampingCoeff;
  const Vector3& pos = m_settings.pos;
  Vector3 len = Vector3(m_settings.cubeNum.x, m_settings.cubeNum.y,
                        m_settings.cubeNum.z) *
                m_settings.cubeLen;

  if (p.position.y < h + pos.y) {
    p.position.y = -p.position.y + 2 * (h + pos.y);
    p.velocity.y = -p.velocity.y * dampingCoeff;
  }

  if (p.position.x < h + pos.x) {
    p.position.x = -p.position.x + 2 * (pos.x + h);
    p.velocity.x = -p.velocity.x * dampingCoeff;
  }

  if (p.position.x > -h + pos.x + len.x) {
    p.position.x = -p.position.x + 2 * (-h + pos.x + len.x);
    p.velocity.x = -p.velocity.x * dampingCoeff;
  }

  if (p.position.z < h + pos.z) {
    p.position.z = -p.position.z + 2 * (h + pos.z);
    p.velocity.z = -p.velocity.z * dampingCoeff;
  }

  if (p.position.z > -h + pos.z + len.z) {
    p.position.z = -p.position.z + 2 * (-h + pos.z + len.z);
    p.velocity.z = -p.velocity.z * dampingCoeff;
  }
}
