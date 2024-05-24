#pragma once

#include "pch.h"

struct Settings {
  Vector3 worldOffset = Vector3(-1, 0.02f, -1);
  XMINT3 initCube = XMINT3(64, 128, 64);
  float h = 0.01f;
  Vector3 initLocalPos = Vector3(0.5f, 0.f, 0.5f);
  XMINT3 marchingResolution = XMINT3(256, 256, 256);
  Vector3 boundaryLen = Vector3(2.0, 1.5, 1.5);
  float mass = 0.0005f;
  float dynamicViscosity = 1.2f;
  float particleRadius = h / 2;
  float dampingCoeff = 0.5f;
  float marchingCubeWidth = boundaryLen.x / marchingResolution.x;
  UINT blockSize = 1024;
  bool cpu = false;
  bool marching = true;
  float dt = 1.f / 160.f;
  UINT TABLE_SIZE = 512 * 2001 - 1;
};
