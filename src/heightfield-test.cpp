#include "heightfield.h"

#include <iostream>

int main() {
  HeightField::Props props;
  props.gridSize = XMINT2(10, 10);
  HeightField hf(props);

  float dt = 0.03f;
  // hf.m_grid[4][4].height = 7;
  for (int i = 0; i < 100; i++) {
    std::cout << "iteration: " << i << '\n';

    if (i == 1) {
      hf.StartImpulse(4, 4, 2, 0);
    }
    hf.Update(dt);

    std::cout << "Velocity:\n";
    for (auto& row : hf.m_grid) {
      for (auto& c : row) {
        std::cout << c.velocity << " | ";
      }
      std::cout << "\n";
    }

    std::cout << "Height:\n";
    for (auto& row : hf.m_grid) {
      for (auto& c : row) {
        std::cout << c.height << " | ";
      }
      std::cout << "\n";
    }

    std::cout << "Objects:\n";
    for (auto& row : hf.m_objects) {
      for (auto& c : row) {
        std::cout << c.height << " | ";
      }
      std::cout << "\n";
    }

    std::cout << "=====================================\n";
  }
}
