#pragma once

#include "pch.h"

struct Settings {
  Vector3 worldOffset = Vector3(0, 0, 0);
  XMINT3 initCube = XMINT3(64, 64, 128);
  float h = 0.01f;
  Vector3 initLocalPos = Vector3(0.5, 0.5, 0.15);
  XMINT3 marchingResolution = XMINT3(256, 256, 256);
  Vector3 boundaryLen = Vector3(1.7, 1.5, 1.5);
  float mass = 0.0005f;
  float dynamicViscosity = 1.1f;
  float particleRadius = h / 2;
  float dampingCoeff = 0.8f;
  float marchingCubeWidth = boundaryLen.x / marchingResolution.x;
  UINT blockSize = 1024;
  bool cpu = false;
  bool marching = true;
  float dt = 1.f / 160.f;
  UINT TABLE_SIZE = initCube.x * initCube.y * initCube.z;
};
