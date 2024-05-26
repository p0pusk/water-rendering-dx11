#pragma once

#include "pch.h"

struct Settings {
  Vector3 worldOffset = Vector3(-12.f, -0.5f, -8.f);
  XMINT3 initCube = XMINT3(64, 64, 128);
  float h = 0.1f;
  Vector3 initLocalPos = Vector3(2.f, 1.f, 2.f);
  XMINT3 marchingResolution = XMINT3(256, 256, 256);
  Vector3 boundaryLen = Vector3(20, 15, 16);
  float mass = 1.f;
  float dynamicViscosity = 1.2f;
  float particleRadius = h / 4;
  float dampingCoeff = 0.5f;
  float marchingCubeWidth = boundaryLen.x / marchingResolution.x;
  Vector2 trappedAirThreshold = Vector2(1000, 2000);
  Vector2 wavecrestThreshold = Vector2(0, 10);
  Vector2 energyThreshold = Vector2(10, 100);
  UINT blockSize = 1024;
  UINT diffuseNum = 2 * 1024 * 1024;
  bool cpu = false;
  bool diffuseEnabled = true;
  bool marching = true;
  float dt = 1.f / 160.f;
  UINT TABLE_SIZE = 512 * 2001 - 1;
};
