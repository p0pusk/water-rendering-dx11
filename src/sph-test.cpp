#include "sph.h"

#include <iostream>

int main() {
  SPH::Props props;
  props.cubeNum = {10, 10, 10};
  SPH sph(props);
  sph.Init();

  auto& p = sph.m_particles[100];

  for (int i = 0; i < 100; i++) {
    std::cout << std::endl << "n: " << sph.n << std::endl;
    std::cout << "density: " << p.density << std::endl;
    std::cout << "pressure: " << p.pressure << std::endl;
    std::cout << "pressure grad: " << p.pressureGrad.x << " "
              << p.pressureGrad.y << " " << p.pressureGrad.z << std::endl;
    std::cout << "viscosity: " << p.viscosity.x << " " << p.viscosity.y << " "
              << p.viscosity.z << std::endl;
    std::cout << "velocity: " << p.velocity.x << " " << p.velocity.y << " "
              << p.velocity.z << std::endl;
    std::cout << "position: " << p.position.x << " " << p.position.y << " "
              << p.position.z << std::endl;

    sph.Update(0.04);
  }
}
