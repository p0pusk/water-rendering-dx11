#include "sph.h"

#include <random>

void SPH::Init() {
  std::random_device dev;
  std::mt19937 rng(dev());
  boxH = 0.5f;

  std::uniform_real_distribution<> dist((double)-boxH, (double)boxH);
  std::uniform_real_distribution<> distY(0, 5);

  std::cout << dist(rng) << std::endl;
  for (auto& p : m_particles) {
    p.mass = 0.02f;
    p.position = Vector3(dist(rng), distY(rng), dist(rng));
    p.velocity = Vector3::Zero;
  }
}

void SPH::Update(float dt) {
  n++;

  // Compute density
  for (auto& p : m_particles) {
    p.density = 0;
    for (auto& n : m_particles) {
      float d2 = Vector3::DistanceSquared(p.position, n.position);
      if (d2 < h2) {
        p.density += n.mass * poly6 * pow(h2 - d2, 3);
      }
    }
  }

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
        p.pressureGrad += -dir * n.mass * (p.pressure + n.pressure) /
                          (2 * n.density) * spikyGrad;
      }
    }
  }

  // Compute viscosity
  for (auto& p : m_particles) {
    p.viscosity = Vector3::Zero;
    for (auto& n : m_particles) {
      float d = Vector3::Distance(p.position, n.position);
      if (d < h) {
        p.viscosity += dynamicViscosity * n.mass * (n.velocity - p.velocity) /
                       n.density * spikyLap * (h - d);
      }
    }
  }

  // TimeStep
  for (auto& p : m_particles) {
    p.velocity += dt * (p.pressureGrad + p.force + p.viscosity) / p.density;
    // p.velocity += dt * (p.force + p.pressureGrad) / p.density;
    p.position += dt * p.velocity;

    // boundary condition
    CheckBoundary(p);
  }
}

void SPH::CheckBoundary(Particle& p) {
  float dampingCoeff = 0.5f;
  if (p.position.y < 0) {
    p.velocity.y *= -dampingCoeff;
    p.position.y = 0 + 0.01f;
  }
  if (p.position.x < -boxH) {
    p.velocity.x *= -dampingCoeff;
    p.position.x = -boxH;
  }
  if (p.position.x > boxH) {
    p.velocity.x *= -dampingCoeff;
    p.position.x = boxH;
  }
  if (p.position.z < -boxH) {
    p.velocity.z *= -dampingCoeff;
    p.position.z = -boxH;
  }
  if (p.position.z > boxH) {
    p.velocity.z *= -dampingCoeff;
    p.position.z = boxH;
  }
}
float SPH::GradKernel(float r, float h) {
  if (r >= 0 && r <= h) {
    return 45 / M_PI / pow(h, 6) * pow(h - r, 2);
  }
  return 0;
}

float SPH::Kernel(float r, float h) {
  if (r >= 0 && r <= h) {
    return 315 / 64 / M_PI / pow(h, 9) * pow(pow(h, 2) - pow(r, 2), 3);
  }
  return 0;
}

float SPH::LagrangianKernel(float r, float h) {
  if (r >= 0 && r <= h) {
    return 45 / M_PI / pow(h, 6) * (h - r);
  }
  return 0;
}
