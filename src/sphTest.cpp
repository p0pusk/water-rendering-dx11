#include <iostream>
#include "sph.h"


int main() {
  SPH sph(100);
  sph.Init();

  auto p = sph.m_particles[0];
  auto v = sph.m_particles[1];
  std::cout << "p position: " << p.position.x << " " << p.position.y << " " << p.position.z << std::endl;
  std::cout << "v position: " << v.position.x << " " << v.position.y << " " << v.position.z << std::endl;
  std::cout << "dist: " << SimpleMath::Vector3::Distance(p.position, v.position) << std::endl;
  std::cout << "Kernel 0: " << sph.Kernel( 0, sph.m_h) << std::endl;
  std::cout << "Kernel: " << sph.Kernel(SimpleMath::Vector3::Distance(p.position, v.position), sph.m_h);

  for (int i = 0; i < 100; i++) {
    std::cout << std::endl << "n: " << sph.m_n << std::endl;
    std::cout << "density: " << sph.m_particles[0].density << std::endl;
    std::cout << "pressure: " << sph.m_particles[0].pressure << std::endl;
    std::cout << "position: " << p.position.x << " " << p.position.y << " " << p.position.z << std::endl;
    std::cout << "acceleration: " << p.acceleration.x << " " << p.acceleration.y << " " << p.acceleration.z << std::endl;
    std::cout << "velocity: " << p.velocity.x << " " << p.velocity.y << " " << p.velocity.z << std::endl;

    sph.Update(0.004);
  }
}