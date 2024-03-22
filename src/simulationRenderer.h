#pragma once

#include <stdint.h>

#include <algorithm>
#include <memory>

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

struct MarchingVertex {
  Vector3 pos;
  Vector3 normal;
};

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

class SimRenderer : Primitive {
 public:
  std::vector<Particle> m_particles;
  std::vector<UINT> m_hash_table;
  UINT m_num_particles;
  int n = 0;
  static const UINT TABLE_SIZE = 262144;

  SimRenderer() = delete;
  SimRenderer(SimRenderer const& s) = delete;
  SimRenderer(SimRenderer&& s) = delete;
  SimRenderer(std::shared_ptr<DX::DeviceResources> dxController,
              const Settings& props)
      : Primitive(dxController),
        m_hash_table(TABLE_SIZE, NO_PARTICLE),
        m_num_particles(props.cubeNum.x * props.cubeNum.y * props.cubeNum.z),
        m_settings(props),
        m_marchingCubesAlgo(props, m_particles),
        m_sphAlgo(props) {}

  HRESULT Init() override;
  void Update(float dt);
  void Render(ID3D11Buffer* pSceneBuffer = nullptr) override;

  const Settings& m_settings;

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
  ComPtr<ID3D11ComputeShader> m_pClearBufferCS;
  ComPtr<ID3D11VertexShader> m_pMarchingVertexShader;
  ComPtr<ID3D11Buffer> m_pMarchingOutBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pMarchingOutBufferUAV;
  ComPtr<ID3D11PixelShader> m_pMarchingPixelShader;
  ComPtr<ID3D11ComputeShader> m_pMarchingComputeShader;
  ComPtr<ID3D11Buffer> m_pVoxelGridBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pVoxelGridBufferUAV;

  ComPtr<ID3D11InputLayout> m_pMarchingInputLayout;

  ComPtr<ID3D11Buffer> m_pSphDataBuffer;
  SphCB m_sphCB;
  ComPtr<ID3D11Buffer> m_pSphCB;
  ComPtr<ID3D11UnorderedAccessView> m_pSphBufferUAV;
  ComPtr<ID3D11Buffer> m_pHashBuffer;
  ComPtr<ID3D11UnorderedAccessView> m_pHashBufferUAV;
  ComPtr<ID3D11ShaderResourceView> m_pSphBufferSRV;

  HRESULT InitSph();
  HRESULT InitSpheres();
  HRESULT InitMarching();

  HRESULT UpdatePhysGPU();

  void RenderMarching(ID3D11Buffer* pSceneBuffer = nullptr);
  void RenderSpheres(ID3D11Buffer* pSceneBuffer = nullptr);
};
