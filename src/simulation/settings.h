#pragma once

#include "pch.h"

struct Settings {
  Vector3 pos = Vector3(-1, 0, -1);
  XMINT3 cubeNum = XMINT3(20, 30, 10);
  float cubeLen = 0.10001f;
  float h = 0.1f;
  float mass = 0.2f;
  float dynamicViscosity = 0.f;
  float particleRadius = h / 2;
  float dampingCoeff = 0.5f;
  float marchingCubeWidth = h / 2;
  UINT blockSize = 1024;
  bool cpu = false;
  bool marching = false;
  const static UINT TABLE_SIZE = 271000;
};
