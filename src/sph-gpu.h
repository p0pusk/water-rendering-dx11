#pragma once

#include <memory>

#include "device-resources.h"
#include "particle.h"
#include "pch.h"
#include "settings.h"

struct SphCB {
  Vector3 pos;
  UINT particleNum;
  XMINT3 cubeNum;
  float cubeLen;
  float h;
  float mass;
  float dynamicViscosity;
  float dampingCoeff;
  float marchingCubeWidth;
  Vector3 dt;
};

class SphGpu {
 public:
  SphGpu(std::shared_ptr<DX::DeviceResources> deviceResources,
         const Settings& settings)
      : m_pDeviceResources(deviceResources), m_settings(settings) {}

  void Init(const std::vector<Particle>& particles);
  void Update();
  void Render();

 private:
  std::shared_ptr<DX::DeviceResources> m_pDeviceResources;
  const Settings& m_settings;
  UINT m_num_particles;

  float m_sphTime;
  UINT m_frameNum = 1;

  SphCB m_sphCB;
  ComPtr<ID3D11Buffer> m_pSphCB;
  ComPtr<ID3D11Buffer> m_pSphDataBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pSphBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pSphBufferSRV;
  ComPtr<ID3D11Buffer> m_pHashBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pHashBufferUAV;

  ComPtr<ID3D11ComputeShader> m_pDensityCS;
  ComPtr<ID3D11ComputeShader> m_pPressureCS;
  ComPtr<ID3D11ComputeShader> m_pClearTableCS;
  ComPtr<ID3D11ComputeShader> m_pTableCS;
  ComPtr<ID3D11ComputeShader> m_pForcesCS;
  ComPtr<ID3D11ComputeShader> m_pPositionsCS;

  ID3D11Query* m_pQueryDisjoint[2];
  ID3D11Query* m_pQuerySphStart[2];
  ID3D11Query* m_pQuerySphEnd[2];

  UINT GetHash(XMINT3 cell);
  XMINT3 GetCell(Vector3 position);
};
