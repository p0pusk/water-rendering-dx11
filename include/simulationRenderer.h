#pragma once

#include <stdint.h>

#include <algorithm>
#include <memory>

#include "SimpleMath.h"
#include "device-resources.h"
#include "marching-cubes.h"
#include "neighbour-hash.h"
#include "settings.h"
#include "sph-gpu.h"
#include "sph.h"

#define _USE_MATH_DEFINES

#include "pch.h"

using namespace DirectX::SimpleMath;
using namespace DirectX;

struct MarchingIndirectBuffer {
  UINT vertexPerInst = 3;
  UINT counter;
  UINT startIndexLocation = 0;
  int baseVertexLocation = 0;
  UINT startInstanceLocation = 0;
};

struct SurfaceBuffer {
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

class SimRenderer {
public:
  std::vector<Particle> m_particles;
  UINT m_num_particles;
  int n = 0;

  SimRenderer() = delete;
  SimRenderer(SimRenderer const &s) = delete;
  SimRenderer(SimRenderer &&s) = delete;
  SimRenderer(const Settings &settings)
      : m_num_particles(settings.initCube.x * settings.initCube.y *
                        settings.initCube.z),
        m_settings(settings), m_marchingCubesAlgo(settings, m_particles),
        m_sphAlgo(settings), m_sphGpuAlgo(settings) {}

  HRESULT Init();
  void Update(float dt);
  void Render(ID3D11Buffer *pSceneBuffer = nullptr);
  void ImGuiRender();

  const Settings &m_settings;

private:
  UINT m_sphereIndexCount;

  MarchingCube m_marchingCubesAlgo;
  Sph m_sphAlgo;
  SphGpu m_sphGpuAlgo;

  std::vector<Vector3> m_vertex;
  std::vector<UINT> m_index;

  ComPtr<ID3D11Buffer> m_pMarchingVertexBuffer;
  ComPtr<ID3D11Buffer> m_pVertexBuffer;
  ComPtr<ID3D11Buffer> m_pIndexBuffer;
  ComPtr<ID3D11InputLayout> m_pInputLayout;
  ComPtr<ID3D11ComputeShader> m_pDensityCS;
  ComPtr<ID3D11ComputeShader> m_pPressureCS;
  ComPtr<ID3D11ComputeShader> m_pClearTableCS;
  ComPtr<ID3D11ComputeShader> m_pTableCS;
  ComPtr<ID3D11ComputeShader> m_pForcesCS;
  ComPtr<ID3D11ComputeShader> m_pPositionsCS;
  ComPtr<ID3D11ComputeShader> m_pMarchingPreprocessCS;
  ComPtr<ID3D11VertexShader> m_pMarchingVertexShader;
  ComPtr<ID3D11VertexShader> m_pVertexShader;
  ComPtr<ID3D11VertexShader> m_pMarchingIndirectVertexShader;
  ComPtr<ID3D11Buffer> m_pMarchingOutBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pMarchingOutBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pMarchingOutBufferSRV;
  ComPtr<ID3D11PixelShader> m_pMarchingPixelShader;
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
  ComPtr<ID3D11Buffer> m_pCountBuffer;

  ComPtr<ID3D11InputLayout> m_pMarchingInputLayout;
  ComPtr<ID3D11InputLayout> m_pMarchingIndirectInputLayout;

  MarchingIndirectBuffer m_MarchingIndirectBuffer;
  ComPtr<ID3D11Buffer> m_pSurfaceBuffer;
  ComPtr<ID3D11Buffer> m_pFilteredVoxelBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pFilteredVoxelBufferUAV;
  ComPtr<ID3D11UnorderedAccessView> m_pSurfaceBufferUAV;

  ID3D11Query *m_pQueryDisjoint[2];
  ID3D11Query *m_pQueryMarchingStart[2];
  ID3D11Query *m_pQueryMarchingPreprocess[2];
  ID3D11Query *m_pQueryMarchingMain[2];

  float m_marchingClear;
  float m_marchingPrep;
  float m_marchingMain;
  UINT m_frameNum;

  HRESULT InitSpheres();
  HRESULT InitMarching();

  void CollectTimestamps();

  void RenderMarching(ID3D11Buffer *pSceneBuffer = nullptr);
  void RenderSpheres(ID3D11Buffer *pSceneBuffer = nullptr);
};
