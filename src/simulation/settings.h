#pragma once

#include "pch.h"

struct Settings {
  Vector3 worldOffset = Vector3(-1, 0, -1);
  XMINT3 initCube = XMINT3(10, 40, 10);
  Vector3 initLocalPos = Vector3(0, 0, 0);
  float h = 0.1f;
  Vector3 boundaryLen = Vector3(50, 15, 15) * h;
  float mass = 0.5f;
  float dynamicViscosity = 1.1f;
  float particleRadius = h / 2;
  float dampingCoeff = 0.3f;
  float marchingCubeWidth = h / 2;
  UINT blockSize = 64;
  bool cpu = true;
  bool marching = false;
  const static UINT TABLE_SIZE = 271000;
};
