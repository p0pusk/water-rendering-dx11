#include "sph.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <random>


void SPH::Init() {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<> dist(0, 5); 

  std::cout << dist(rng) << std::endl;
  for (auto& p : m_particles) {
    p.mass = 0.01;
    p.position = SimpleMath::Vector3(dist(rng), dist(rng), dist(rng));
    m_lambda = 2 * m_k * (1 + m_polytropicIndex) *
      pow(M_PI, -3 / (2 * m_polytropicIndex)) *
      pow((m_star_mass * tgamma(5 / 2 + m_polytropicIndex)) / (pow(m_star_radius, 3) / tgamma(1 + m_polytropicIndex)), 1 / m_polytropicIndex) / pow(m_star_mass, 2);

    p.velocity = SimpleMath::Vector3::Zero;
    p.acceleration = SimpleMath::Vector3::Zero;
  }
}

void SPH::Update(float dt) {
   m_n++;

   UpdateDensity();
   // update pressure
   for (int i = 0; i < m_particles.size(); i++) {
     auto& particle = m_particles[i];
     particle.pressure = m_k * pow(particle.density, 1 + 1 / m_polytropicIndex);
   }
   UpdateAcceleration();

   for (int i = 0; i < m_particles.size(); i++) {
     m_particles[i].velocity +=  m_particles[i].acceleration * dt;
     m_particles[i].position += m_particles[i].velocity * dt;
   }
 }

void SPH::UpdateDensity() {
  for (int i = 0; i < m_particles.size(); i++) {
    auto &particle = m_particles[i];
    particle.density = 0;

    for (int j = 0; j < m_particles.size(); j++) {
      auto &neighbour = m_particles[j];
      particle.density += neighbour.mass * Kernel(SimpleMath::Vector3::Distance(particle.position, neighbour.position), m_h);
    }
  }
}

void SPH::UpdateAcceleration() {
  for (int i = 0; i < m_particles.size(); i++) {
    auto& particle = m_particles[i];
    particle.acceleration = SimpleMath::Vector3(0, -9.8, 0) / particle.density;
  }

  for (int i = 0; i < m_particles.size(); i++) {
    auto& particle = m_particles[i];
     for (int j = 0; j < m_particles.size(); j++) {
       auto &neighbour = m_particles[j];
       auto a = -particle.mass *
         (particle.pressure + neighbour.pressure) / neighbour.density / particle.density / 2
           * GradKernel(particle.position - neighbour.position, m_h, SimpleMath::Vector3::Distance(particle.position, neighbour.position));
       particle.acceleration += a;
     }
   }
}

SimpleMath::Vector3 SPH::GradKernel(SimpleMath::Vector3 v, float smoothing, float distance) {
  return -45 / M_PI / pow(smoothing, 6) * distance
    * SimpleMath::Vector3(
      pow(smoothing - v.x, 2) / v.x,
      pow(smoothing - v.y, 2) / v.y,
      pow(smoothing - v.z, 2) / v.z);
}

float SPH::Kernel(float distance, float smoothing) {
  return 1 / pow(M_PI, 3 / 2) / pow(smoothing, 3) * exp(-pow(distance, 2) / pow(smoothing, 2));
}


