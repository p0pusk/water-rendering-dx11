#pragma once

#include <stdint.h>

#include <algorithm>
#include <memory>

#include "CommonStates.h"
#include "Effects.h"
#include "SimpleMath.h"
#include "device-resources.h"
#include "marching-cubes.h"
#include "mc-gpu.h"
#include "neighbour-hash.h"
#include "settings.h"
#include "sph-gpu.h"
#include "sph.h"

#define _USE_MATH_DEFINES

#include "pch.h"

using namespace DirectX::SimpleMath;
using namespace DirectX;

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
  void Render(Vector4 cameraPos, ID3D11Buffer *pSceneBuffer = nullptr);

  const Settings &m_settings;

private:
  UINT m_sphereIndexCount;

  MarchingCube m_marchingCubesAlgo;
  Sph m_sphAlgo;
  SphGpu m_sphGpuAlgo;
  std::unique_ptr<MCGpu> m_mcGpu;

  std::vector<Vector3> m_vertex;
  std::vector<UINT> m_index;
  std::unique_ptr<CommonStates> m_states;
  std::unique_ptr<IEffectFactory> m_fxFactory;

  ComPtr<ID3D11Buffer> m_pMarchingVertexBuffer;
  ComPtr<ID3D11Buffer> m_pVertexBuffer;
  ComPtr<ID3D11Buffer> m_pIndexBuffer;
  ComPtr<ID3D11InputLayout> m_pInputLayout;
  ComPtr<ID3D11VertexShader> m_pMarchingVertexShader;
  ComPtr<ID3D11VertexShader> m_pVertexShader;
  ComPtr<ID3D11VertexShader> m_pDiffuseVertexShader;
  ComPtr<ID3D11PixelShader> m_pMarchingPixelShader;
  ComPtr<ID3D11PixelShader> m_pPixelShader;

  ComPtr<ID3D11InputLayout> m_pMarchingInputLayout;

  UINT m_frameNum;

  HRESULT InitSpheres();
  HRESULT InitMarching();

  void RenderMarching(Vector4 cameraPos, ID3D11Buffer *pSceneBuffer = nullptr);
  void RenderSpheres(ID3D11Buffer *pSceneBuffer = nullptr);
  void RenderDiffuse(ID3D11Buffer *pSceneBuffer = nullptr);
};
