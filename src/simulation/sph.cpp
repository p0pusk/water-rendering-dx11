#include "sph.h"

#include <stdint.h>

#include <algorithm>
#include <random>

#include "SimpleMath.h"
#include "neighbour-hash.h"
#include "particle.h"

void SPH::Init() {
  std::random_device dev;
  std::mt19937 rng(dev());

  float& h = m_props.h;
  float separation = h + 0.01f;
  Vector3 offset = {h / 2, 1.5f + h / 2, h / 2};

  m_particles.resize(m_props.cubeNum.x * m_props.cubeNum.y * m_props.cubeNum.z);

  for (int i = 0; i < m_props.cubeNum.x; i++) {
    for (int j = 0; j < m_props.cubeNum.y; j++) {
      for (int k = 0; k < m_props.cubeNum.z; k++) {
        size_t particleIndex =
            i + (j + k * m_props.cubeNum.y) * m_props.cubeNum.x;

        Particle& p = m_particles[particleIndex];
        p.position.x = m_props.pos.x + i * separation + offset.x;
        p.position.y = m_props.pos.y + j * separation + offset.y;
        p.position.z = m_props.pos.z + k * separation + offset.z;
        p.mass = m_props.mass;
        p.velocity = Vector3::Zero;
      }
    }
  }

  // create hash table
  for (auto& p : m_particles) {
    p.hash = m_hash.getHash(m_hash.getCell(p, h, m_props.pos));
  }

  std::sort(
      m_particles.begin(), m_particles.end(),
      [&](const Particle& a, const Particle& b) { return a.hash < b.hash; });

  m_hash.createTable(m_particles);
}

void SPH::Update(float dt) {
  n++;
  float& h = m_props.h;

  // Compute density
  // for (auto& p : m_particles) {
  //   p.density = 0;
  //   for (auto& n : m_particles) {
  //     float d2 = Vector3::DistanceSquared(p.position, n.position);
  //     if (d2 < h2) {
  //       p.density += n.mass * poly6 * pow(h2 - d2, 3);
  //     }
  //   }
  // }
  //
  UpdateDensity();

  // Compute pressure
  for (auto& p : m_particles) {
    float k = 1;
    float p0 = 1000;
    p.pressure = k * (p.density - p0);
  }

  // Compute pressure force
  for (auto& p : m_particles) {
    p.pressureGrad = Vector3::Zero;
    p.force = Vector3(0, -9.8f * p.density, 0);
    for (auto& n : m_particles) {
      float d = Vector3::Distance(p.position, n.position);
      Vector3 dir = (p.position - n.position);
      dir.Normalize();
      if (d < h) {
        p.pressureGrad += dir * n.mass * (p.pressure + n.pressure) /
                          (2 * n.density) * spikyGrad * std::pow(h - d, 2);
      }
    }
  }

  // Compute viscosity
  for (auto& p : m_particles) {
    p.viscosity = Vector3::Zero;
    for (auto& n : m_particles) {
      float d = Vector3::Distance(p.position, n.position);
      if (d < h) {
        p.viscosity += m_props.dynamicViscosity * n.mass *
                       (n.velocity - p.velocity) / n.density * spikyLap *
                       (h - d);
      }
    }
  }

  // TimeStep
  for (auto& p : m_particles) {
    p.velocity += dt * (p.pressureGrad + p.force + p.viscosity) / p.density;
    p.position += dt * p.velocity;

    // boundary condition
    CheckBoundary(p);
  }
}

void SPH::UpdateDensity() {
  for (auto& p : m_particles) {
    p.density = m_props.mass * poly6 * std::pow(h2, 3);

    XMINT3 cell = m_hash.getCell(p, m_props.h, m_props.pos);

    for (int x = -1; x <= 1; x++) {
      for (int y = -1; y <= 1; y++) {
        for (int z = -1; z <= 1; z++) {
          uint32_t cellHash =
              m_hash.getHash({cell.x + x, cell.y + y, cell.z + z});
          uint32_t ni = m_hash.m[cellHash];
          if (ni == NO_PARTICLE) {
            continue;
          }

          Particle* n = &m_particles[ni];
          while (n->hash == cellHash) {
            float d2 = Vector3::DistanceSquared(p.position, n->position);
            if (d2 < h2) {
              p.density += n->mass * poly6 * std::pow(h2 - d2, 3);
            }

            if (ni == m_particles.size() - 1) break;
            n = &m_particles[++ni];
          }
        }
      }
    }
  }
}

void SPH::CheckBoundary(Particle& p) {
  float& h = m_props.h;
  float& dampingCoeff = m_props.dampingCoeff;
  Vector3& pos = m_props.pos;
  Vector3 len =
      Vector3(m_props.cubeNum.x, m_props.cubeNum.y, m_props.cubeNum.z) *
      m_props.cubeLen;

  if (p.position.y < h + pos.y) {
    p.position.y = -p.position.y + 2 * (h + pos.y) + 0.0001f;
    p.velocity.y = -p.velocity.y * dampingCoeff;
  }

  if (p.position.x < h + pos.x) {
    p.position.x = -p.position.x + 2 * (pos.x + h) + 0.0001f;
    p.velocity.x = -p.velocity.x * dampingCoeff;
  }

  if (p.position.x > -h + pos.x + len.x) {
    p.position.x = -p.position.x + 2 * (-h + pos.x + len.x) - 0.0001f;
    p.velocity.x = -p.velocity.x * dampingCoeff;
  }

  if (p.position.z < h + pos.z) {
    p.position.z = -p.position.z + 2 * (h + pos.z) + 0.0001f;
    p.velocity.z = -p.velocity.z * dampingCoeff;
  }

  if (p.position.z > -h + pos.z + len.z) {
    p.position.z = -p.position.z + 2 * (-h + pos.z + len.z) - 0.0001f;
    p.velocity.z = -p.velocity.z * dampingCoeff;
  }
}
