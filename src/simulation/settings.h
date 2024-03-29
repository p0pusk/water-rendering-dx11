#pragma once

struct Settings {
  Vector3 pos = Vector3::Zero;
  XMINT3 cubeNum = XMINT3(10, 10, 10);
  float cubeLen = 0.20f;
  float h = 0.1f;
  float mass = 0.02f;
  float dynamicViscosity = 1.1f;
  float particleRadius = h;
  float dampingCoeff = 0.5f;
  float marchingCubeWidth = h / 2;
  UINT blockSize = 1024;
  bool cpu = true;
  bool marching = false;
};
