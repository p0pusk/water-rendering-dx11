#pragma once

#include <memory>

#include "device-resources.h"
#include "imgui.h"
#include "particle.h"
#include "pch.h"
#include "settings.h"

struct SphCB {
  Vector3 worldOffset;
  UINT particleNum;
  Vector3 boundaryLen;
  float h;
  float mass;
  float dynamicViscosity;
  float dampingCoeff;
  float marchingCubeWidth;
  UINT hashTableSize;
  Vector3 dt;
};

class SphGpu {
public:
  SphGpu(const Settings &settings) : m_settings(settings), m_frameNum(0) {
    m_pQueryDisjoint.resize(2);
    m_pQuerySphStart.resize(2);
    m_pQuerySphPrefix.resize(2);
    m_pQuerySphClear.resize(2);
    m_pQuerySphHash.resize(2);
    m_pQuerySphDensity.resize(2);
    m_pQuerySphPressure.resize(2);
    m_pQuerySphForces.resize(2);
    m_pQuerySphPosition.resize(2);
  }

  void Init(const std::vector<Particle> &particles);
  void Update();
  void Render();
  void ImGuiRender();

  ComPtr<ID3D11Buffer> m_pSphCB;
  ComPtr<ID3D11Buffer> m_pSphDataBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pSphBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pSphBufferSRV;
  ComPtr<ID3D11UnorderedAccessView> m_pHashBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pHashBufferSRV;
  ComPtr<ID3D11UnorderedAccessView> m_pEntriesBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pEntriesBufferSRV;

private:
  void CreateQueries();
  void CollectTimestamps();

  const Settings &m_settings;
  UINT m_num_particles;

  SphCB m_sphCB;
  ComPtr<ID3D11Buffer> m_pHashBuffer;
  ComPtr<ID3D11Buffer> m_pEntriesBuffer;
  ComPtr<ID3D11Buffer> m_pScanHelperBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pScanHelperBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pScanHelperBufferSRV;

  ComPtr<ID3D11ComputeShader> m_pClearTableCS;
  ComPtr<ID3D11ComputeShader> m_pCreateHashCS;
  ComPtr<ID3D11ComputeShader> m_pPrefixSumCS;
  ComPtr<ID3D11ComputeShader> m_pCreateEntriesCS;
  ComPtr<ID3D11ComputeShader> m_pDensityCS;
  ComPtr<ID3D11ComputeShader> m_pPressureCS;
  ComPtr<ID3D11ComputeShader> m_pForcesCS;
  ComPtr<ID3D11ComputeShader> m_pPositionsCS;

  std::vector<ComPtr<ID3D11Query>> m_pQueryDisjoint;
  std::vector<ComPtr<ID3D11Query>> m_pQuerySphStart;
  std::vector<ComPtr<ID3D11Query>> m_pQuerySphClear;
  std::vector<ComPtr<ID3D11Query>> m_pQuerySphHash;
  std::vector<ComPtr<ID3D11Query>> m_pQuerySphPrefix;
  std::vector<ComPtr<ID3D11Query>> m_pQuerySphDensity;
  std::vector<ComPtr<ID3D11Query>> m_pQuerySphPressure;
  std::vector<ComPtr<ID3D11Query>> m_pQuerySphForces;
  std::vector<ComPtr<ID3D11Query>> m_pQuerySphPosition;

  float m_sphTime;
  float m_sphPrefixTime;
  float m_sphClearTime;
  float m_sphCreateHashTime;
  float m_sphDensityTime;
  float m_sphPressureTime;
  float m_sphForcesTime;
  float m_sphPositionsTime;
  float m_sphOverallTime;
  float m_sortTime;
  UINT m_frameNum;
};
