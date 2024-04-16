#pragma once

#include <stdint.h>

#include <algorithm>
#include <memory>

#include "SimpleMath.h"
#include "device-resources.h"
#include "marching-cubes.h"
#include "neighbour-hash.h"
#include "primitive.h"
#include "settings.h"
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

struct MarchingOutBuffer {
  Vector3 v1;
  Vector3 v2;
  Vector3 v3;
};

struct MarchingVertex {
  Vector3 pos;
  Vector3 normal;
};

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

class SimRenderer : Primitive {
public:
  std::vector<Particle> m_particles;
  std::vector<UINT> m_hash_table;
  UINT m_num_particles;
  int n = 0;

  SimRenderer() = delete;
  SimRenderer(SimRenderer const &s) = delete;
  SimRenderer(SimRenderer &&s) = delete;
  SimRenderer(std::shared_ptr<DX::DeviceResources> dxController,
              const Settings &props)
      : Primitive(dxController),
        m_hash_table(m_settings.TABLE_SIZE, NO_PARTICLE),
        m_num_particles(props.initCube.x * props.initCube.y * props.initCube.z),
        m_settings(props), m_marchingCubesAlgo(props, m_particles),
        m_sphAlgo(props) {}

  HRESULT Init() override;
  void Update(float dt);
  void Render(ID3D11Buffer *pSceneBuffer = nullptr) override;
  void ImGuiRender();

  const Settings &m_settings;

private:
  UINT m_sphereIndexCount;

  MarchingCube m_marchingCubesAlgo;
  Sph m_sphAlgo;

  std::vector<Vector3> m_vertex;
  std::vector<UINT> m_index;

  ComPtr<ID3D11Buffer> m_pMarchingVertexBuffer;
  ComPtr<ID3D11ComputeShader> m_pDensityCS;
  ComPtr<ID3D11ComputeShader> m_pPressureCS;
  ComPtr<ID3D11ComputeShader> m_pClearTableCS;
  ComPtr<ID3D11ComputeShader> m_pTableCS;
  ComPtr<ID3D11ComputeShader> m_pForcesCS;
  ComPtr<ID3D11ComputeShader> m_pPositionsCS;
  ComPtr<ID3D11ComputeShader> m_pMarchingPreprocessCS;
  ComPtr<ID3D11ComputeShader> m_pMarchingClearCS;
  ComPtr<ID3D11VertexShader> m_pMarchingVertexShader;
  ComPtr<ID3D11VertexShader> m_pMarchingIndirectVertexShader;
  ComPtr<ID3D11Buffer> m_pMarchingOutBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pMarchingOutBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pMarchingOutBufferSRV;
  ComPtr<ID3D11PixelShader> m_pMarchingPixelShader;
  ComPtr<ID3D11ComputeShader> m_pMarchingComputeShader;
  ComPtr<ID3D11Buffer> m_pVoxelGridBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pVoxelGridBufferUAV;
  ComPtr<ID3D11Buffer> m_pCountBuffer;

  ComPtr<ID3D11InputLayout> m_pMarchingInputLayout;
  ComPtr<ID3D11InputLayout> m_pMarchingIndirectInputLayout;

  ComPtr<ID3D11Buffer> m_pSphDataBuffer;
  SphCB m_sphCB;
  MarchingIndirectBuffer m_MarchingIndirectBuffer;
  ComPtr<ID3D11Buffer> m_pSphCB;
  ComPtr<ID3D11UnorderedAccessView> m_pSphBufferUAV;
  ComPtr<ID3D11Buffer> m_pHashBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pHashBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pSphBufferSRV;

  ID3D11Query *m_pQueryDisjoint[2];
  ID3D11Query *m_pQuerySphStart[2];
  ID3D11Query *m_pQuerySphCopy[2];
  ID3D11Query *m_pQuerySphClear[2];
  ID3D11Query *m_pQuerySphHash[2];
  ID3D11Query *m_pQuerySphDensity[2];
  ID3D11Query *m_pQuerySphPressure[2];
  ID3D11Query *m_pQuerySphForces[2];
  ID3D11Query *m_pQuerySphPosition[2];
  ID3D11Query *m_pQueryMarchingStart[2];
  ID3D11Query *m_pQueryMarchingClear[2];
  ID3D11Query *m_pQueryMarchingPreprocess[2];
  ID3D11Query *m_pQueryMarchingMain[2];

  float m_sphTime;
  float m_sphCopyTime;
  float m_sphClearTime;
  float m_sphCreateHashTime;
  float m_sphDensityTime;
  float m_sphPressureTime;
  float m_sphForcesTime;
  float m_sphPositionsTime;
  float m_sphOverallTime;
  float m_marchingClear;
  float m_marchingPrep;
  float m_marchingMain;
  float m_sortTime;
  UINT m_frameNum = 1;

  HRESULT InitSph();
  HRESULT InitSpheres();
  HRESULT InitMarching();

  HRESULT UpdatePhysGPU();
  void CollectTimestamps();

  void RenderMarching(ID3D11Buffer *pSceneBuffer = nullptr);
  void RenderSpheres(ID3D11Buffer *pSceneBuffer = nullptr);
};
