#pragma once

#include <memory>

#include "ScanCS.h"
#include "SimpleMath.h"
#include "device-resources.h"
#include "imgui.h"
#include "particle.h"
#include "pch.h"
#include "settings.h"

const UINT BITONIC_BLOCK_SIZE = 1024;
const UINT TRANSPOSE_BLOCK_SIZE = 16;

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
  UINT diffuseNum;
  float dt;
  float diffuseEnabled;
  Vector2 trappedAirThreshold;
  Vector2 wavecrestThreshold;
  Vector2 energyThreshold;
  Vector2 padding;
};

struct SphStateBuffer {
  float time;
  UINT diffuseNum;
};

class SphGpu {
public:
  SphGpu(const Settings &settings) : m_settings(settings), m_frameNum(0) {}

  ~SphGpu() { m_cScanCS.OnD3D11DestroyDevice(); }

  void Init(const std::vector<Particle> &particles);
  void Update();
  void Render();
  void ImGuiRender();

  ComPtr<ID3D11Buffer> m_pSphCB;
  ComPtr<ID3D11Buffer> m_pSphDataBuffer;
  ComPtr<ID3D11Buffer> m_pDiffuseBuffer1;
  ComPtr<ID3D11Buffer> m_pDiffuseBuffer2;
  ComPtr<ID3D11Buffer> m_pStateBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pSphBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pSphBufferSRV;
  ComPtr<ID3D11UnorderedAccessView> m_pDiffuseBufferUAV1;
  ComPtr<ID3D11ShaderResourceView> m_pDiffuseBufferSRV1;
  ComPtr<ID3D11UnorderedAccessView> m_pDiffuseBufferUAV2;
  ComPtr<ID3D11ShaderResourceView> m_pDiffuseBufferSRV2;
  ComPtr<ID3D11UnorderedAccessView> m_pHashBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pHashBufferSRV;
  ComPtr<ID3D11UnorderedAccessView> m_pEntriesBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pEntriesBufferSRV;

private:
  void CreateQueries();
  void CollectTimestamps();
  void DiffuseSort();

  const Settings &m_settings;
  UINT m_num_particles;
  UINT m_diffuseParticelsNum;

  SphCB m_sphCB;
  CScanCS m_cScanCS;
  ComPtr<ID3D11Buffer> m_pHashBuffer;
  ComPtr<ID3D11Buffer> m_pEntriesBuffer;
  ComPtr<ID3D11Buffer> m_pScanHelperBuffer;
  ComPtr<ID3D11Buffer> m_pPotentialsBuffer;
  ComPtr<ID3D11Buffer> m_pSortCB;
  ComPtr<ID3D11UnorderedAccessView> m_pScanHelperBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pScanHelperBufferSRV;
  ComPtr<ID3D11UnorderedAccessView> m_pStateUAV;
  ComPtr<ID3D11ShaderResourceView> m_pStateSRV;
  ComPtr<ID3D11UnorderedAccessView> m_pPotentialsUAV;
  ComPtr<ID3D11ShaderResourceView> m_pPotentialsSRV;

  ComPtr<ID3D11ComputeShader> m_pClearTableCS;
  ComPtr<ID3D11ComputeShader> m_pCreateHashCS;
  ComPtr<ID3D11ComputeShader> m_pPrefixSumCS;
  ComPtr<ID3D11ComputeShader> m_pCreateEntriesCS;
  ComPtr<ID3D11ComputeShader> m_pDensityCS;
  ComPtr<ID3D11ComputeShader> m_pPressureCS;
  ComPtr<ID3D11ComputeShader> m_pForcesCS;
  ComPtr<ID3D11ComputeShader> m_pPositionsCS;
  ComPtr<ID3D11ComputeShader> m_pSpawnDiffuseCS;
  ComPtr<ID3D11ComputeShader> m_pPotentialsCS;
  ComPtr<ID3D11ComputeShader> m_pBitonicSortCS;
  ComPtr<ID3D11ComputeShader> m_pTransposeCS;

  ComPtr<ID3D11Query> m_pQueryDisjoint;
  ComPtr<ID3D11Query> m_pQuerySphStart;
  ComPtr<ID3D11Query> m_pQuerySphClear;
  ComPtr<ID3D11Query> m_pQuerySphHash;
  ComPtr<ID3D11Query> m_pQuerySphPrefix;
  ComPtr<ID3D11Query> m_pQuerySphDensity;
  ComPtr<ID3D11Query> m_pQuerySphPressure;
  ComPtr<ID3D11Query> m_pQuerySphForces;
  ComPtr<ID3D11Query> m_pQuerySphPosition;

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

  struct SortCB {
    UINT iLevel;
    UINT iLevelMask;
    UINT iWidth;
    UINT iHeight;
  };
};
