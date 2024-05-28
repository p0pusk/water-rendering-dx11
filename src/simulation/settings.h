#pragma once

#include "SimpleMath.h"
#include "pch.h"

struct Settings {
  Vector3 worldOffset = Vector3(-8.f, 0.3f, -8.f);
  XMINT3 initCube = XMINT3(64, 64, 128);
  float h = 0.1f;
  Vector3 initLocalPos = Vector3(1.f, 1.f, 1.f);
  XMINT3 marchingResolution = XMINT3(256, 256, 256);
  Vector3 boundaryLen = Vector3(15, 10, 15);
  float mass = 1.f;
  float dynamicViscosity = 1.2f;
  float particleRadius = h / 2;
  float dampingCoeff = 0.5f;
  Vector3 marchingCubeWidth = Vector3(boundaryLen.x / marchingResolution.x,
                                      boundaryLen.y / marchingResolution.y,
                                      boundaryLen.z / marchingResolution.z);
  Vector2 trappedAirThreshold = Vector2(1, 20);
  Vector2 wavecrestThreshold = Vector2(1, 8);
  Vector2 energyThreshold = Vector2(2, 50);
  UINT blockSize = 1024;
  UINT diffuseNum = 256 * 1024;
  bool cpu = false;
  bool diffuseEnabled = true;
  bool marching = true;
  float dt = 1.f / 160.f;
  UINT TABLE_SIZE = 512 * 2001 - 1;
};
