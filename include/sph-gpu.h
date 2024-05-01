#pragma once

#include <memory>

#include "device-resources.h"
#include "imgui.h"
#include "particle.h"
#include "pch.h"
#include "settings.h"

#define NO_PARTICLE 0xFFFFFFFF

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
  SphGpu(const Settings &settings)
      : m_settings(settings), m_hash_table(m_settings.TABLE_SIZE, NO_PARTICLE) {
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

private:
  UINT GetHash(XMINT3 cell);
  XMINT3 GetCell(Vector3 pos);
  void CreateQueries();
  void CollectTimestamps();

  const Settings &m_settings;
  UINT m_num_particles;
  std::vector<UINT> m_hash_table;

  SphCB m_sphCB;
  ComPtr<ID3D11Buffer> m_pHashBuffer;

  ComPtr<ID3D11ComputeShader> m_pDensityCS;
  ComPtr<ID3D11ComputeShader> m_pPressureCS;
  ComPtr<ID3D11ComputeShader> m_pClearTableCS;
  ComPtr<ID3D11ComputeShader> m_pTableCS;
  ComPtr<ID3D11ComputeShader> m_pForcesCS;
  ComPtr<ID3D11ComputeShader> m_pPositionsCS;

  ComPtr<ID3D11Query> m_pQueryDisjoint[2];
  ComPtr<ID3D11Query> m_pQuerySphStart[2];
  ComPtr<ID3D11Query> m_pQuerySphCopy[2];
  ComPtr<ID3D11Query> m_pQuerySphClear[2];
  ComPtr<ID3D11Query> m_pQuerySphHash[2];
  ComPtr<ID3D11Query> m_pQuerySphDensity[2];
  ComPtr<ID3D11Query> m_pQuerySphPressure[2];
  ComPtr<ID3D11Query> m_pQuerySphForces[2];
  ComPtr<ID3D11Query> m_pQuerySphPosition[2];

  float m_sphTime;
  float m_sphCopyTime;
  float m_sphClearTime;
  float m_sphCreateHashTime;
  float m_sphDensityTime;
  float m_sphPressureTime;
  float m_sphForcesTime;
  float m_sphPositionsTime;
  float m_sphOverallTime;
  float m_sortTime;
  UINT m_frameNum = 0;
};
