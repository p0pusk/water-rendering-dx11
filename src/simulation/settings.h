#pragma once

#include "pch.h"

struct Settings {
  Vector3 worldOffset = Vector3(-1, 0, -1);
  XMINT3 initCube = XMINT3(64, 64, 128);
  Vector3 initLocalPos = Vector3(0, 0, 0);
  float h = 0.01f;
  Vector3 boundaryLen = Vector3(128, 128, 128) * h;
  float mass = 0.0005f;
  float dynamicViscosity = 2.1f;
  float particleRadius = h / 2;
  float dampingCoeff = 0.5f;
  float marchingCubeWidth = h / 2;
  UINT blockSize = 1024;
  bool cpu = false;
  bool marching = true;
  const static UINT TABLE_SIZE = 2710000;
};
