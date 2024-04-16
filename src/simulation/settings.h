#pragma once

#include "pch.h"

struct Settings {
  Vector3 worldOffset = Vector3(-1, 0, -1);
  XMINT3 initCube = XMINT3(20, 20, 20);
  Vector3 initLocalPos = Vector3(0, 0, 0);
  float h = 0.1f;
  Vector3 boundaryLen = Vector3(40, 20, 20) * h;
  float mass = 0.2f;
  float dynamicViscosity = 0.1f;
  float particleRadius = h / 2;
  float dampingCoeff = 0.5f;
  float marchingCubeWidth = h / 2;
  UINT blockSize = 1024;
  bool cpu = false;
  bool marching = false;
  const static UINT TABLE_SIZE = 271000;
};
