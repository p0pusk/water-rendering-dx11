#pragma once

#include "CommonStates.h"
#include "Effects.h"
#include "pch.h"
#include "settings.h"
#include "sph-gpu.h"

using namespace DirectX::SimpleMath;
using namespace DirectX;
using namespace Microsoft::WRL;

struct MarchingIndirectBuffer {
  UINT vertexPerInst = 3;
  UINT counter;
  UINT startIndexLocation = 0;
  int baseVertexLocation = 0;
  UINT startInstanceLocation = 0;
};

struct CountBuffer {
  UINT counter;
  UINT usedCells;
};

struct MarchingOutBuffer {
  Vector3 v1;
  Vector3 v2;
  Vector3 v3;
  Vector3 normal1;
  Vector3 normal2;
  Vector3 normal3;
};

struct MarchingVertex {
  Vector3 pos;
  Vector3 normal;
};

class MCGpu {
public:
  MCGpu(const Settings &settings) : m_settings(settings) {}

  void Init();
  void Update(const SphGpu &sph);
  void Render(ID3D11Buffer *sceneBuffer);
  void ImGuiRender();

private:
  void CreateQueries();
  void CollectTimestamps();
  MarchingIndirectBuffer m_MarchingIndirectBuffer;

  const Settings &m_settings;
  std::unique_ptr<CommonStates> m_states;
  std::unique_ptr<IEffectFactory> m_fxFactory;

  ComPtr<ID3D11Buffer> m_pMarchingOutBuffer;
  ComPtr<ID3D11Buffer> m_pIndirectBuffer;
  ComPtr<ID3D11Buffer> m_pCountBuffer;

  ComPtr<ID3D11UnorderedAccessView> m_pMarchingOutBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pMarchingOutBufferSRV;

  ComPtr<ID3D11Buffer> m_pMarchingVertexBuffer;
  ComPtr<ID3D11VertexShader> m_pVertexShader;
  ComPtr<ID3D11PixelShader> m_pPixelShader;
  ComPtr<ID3D11ComputeShader> m_pMarchingComputeShader;
  ComPtr<ID3D11ComputeShader> m_pSurfaceCountCS;
  ComPtr<ID3D11ComputeShader> m_pSmoothingPreprocessCS;
  ComPtr<ID3D11ComputeShader> m_pBoxFilterCS;
  ComPtr<ID3D11ComputeShader> m_pGaussFilterCS;
  ComPtr<ID3D11Buffer> m_pVoxelGridBuffer1;
  ComPtr<ID3D11UnorderedAccessView> m_pVoxelGridBufferUAV1;
  ComPtr<ID3D11Buffer> m_pVoxelGridBuffer2;
  ComPtr<ID3D11UnorderedAccessView> m_pVoxelGridBufferUAV2;
  ComPtr<ID3D11UnorderedAccessView> m_pCountBufferUAV;

  ComPtr<ID3D11Query> m_pQueryDisjoint;
  ComPtr<ID3D11Query> m_pQueryMarchingStart;
  ComPtr<ID3D11Query> m_pQueryMarchingPreprocess;
  ComPtr<ID3D11Query> m_pQueryMarchingMain;

  UINT m_frameNum = 0;
  float m_preprocessTime;
  float m_mainTime;
  float m_sumTime;
};
