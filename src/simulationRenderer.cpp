#include "simulationRenderer.h"
//
#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include "CommonStates.h"
#include "Effects.h"
#include "SimpleMath.h"
#include "device-resources.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "particle.h"
#include "pch.h"
#include "sph-gpu.h"
#include "utils.h"

using DX::ThrowIfFailed;

HRESULT SimRenderer::Init() {
  HRESULT result = S_OK;
  try {
    m_sphAlgo.Init(m_particles);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Sph init failed" << std::endl;
    exit(1);
  }
  try {
    m_sphGpuAlgo.Init(m_particles);
  } catch (const std::exception &e) {
    std::cerr << "Sph gpu init failed!" << std::endl;
    std::cerr << "  " << e.what() << std::endl;
    exit(1);
  }
  try {
    result = InitMarching();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "InitMarching() failed" << std::endl;
    exit(1);
  }
  try {
    result = InitSpheres();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "InitSpheres() failed" << std::endl;
    exit(1);
  }

  auto pDevice = DeviceResources::getInstance().m_pDevice;

  m_states = std::make_unique<CommonStates>(pDevice.Get());
  m_fxFactory = std::make_unique<EffectFactory>(pDevice.Get());

  D3D11_QUERY_DESC desc{};
  desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  desc.MiscFlags = 0;
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &(m_pQueryDisjoint[0])));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &(m_pQueryDisjoint[1])));
  desc.Query = D3D11_QUERY_TIMESTAMP;
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &(m_pQueryMarchingStart[0])));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &(m_pQueryMarchingStart[1])));
  DX::ThrowIfFailed(
      pDevice->CreateQuery(&desc, &(m_pQueryMarchingPreprocess[0])));
  DX::ThrowIfFailed(
      pDevice->CreateQuery(&desc, &(m_pQueryMarchingPreprocess[1])));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &(m_pQueryMarchingMain[0])));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &(m_pQueryMarchingMain[1])));

  assert(SUCCEEDED(result));

  return result;
}

HRESULT SimRenderer::InitSpheres() {
  auto pDevice = DeviceResources::getInstance().m_pDevice;
  // get sphere data
  static const size_t SphereSteps = 4;

  std::vector<Vector3> sphereVertices;
  std::vector<UINT16> indices;

  size_t indexCount;
  size_t vertexCount;

  DX::GetSphereDataSize(SphereSteps, SphereSteps, indexCount, vertexCount);

  sphereVertices.resize(vertexCount);
  indices.resize(indexCount);

  m_sphereIndexCount = (UINT)indexCount;

  DX::CreateSphere(SphereSteps, SphereSteps, indices.data(),
                   sphereVertices.data());

  for (auto &v : sphereVertices) {
    v = v * m_settings.particleRadius;
  }

  // create input layout
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT result = S_OK;

  // Create vertex buffer
  {
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = sphereVertices.data();
    data.SysMemPitch = (UINT)(sphereVertices.size() * sizeof(Vector3));
    data.SysMemSlicePitch = 0;

    result = DX::CreateVertexBuffer<Vector3>(
        sphereVertices.size(), D3D11_USAGE_DEFAULT, 0, &data,
        "ParticleVertexBuffer", &m_pVertexBuffer);
    DX::ThrowIfFailed(result);
  }

  // create index buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(sizeof(UINT16) * indices.size());
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = indices.data();
    data.SysMemPitch = (UINT)(sizeof(UINT16) * indices.size());
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    ThrowIfFailed(result);
    result = DX::SetResourceName(m_pIndexBuffer.Get(), "ParticleIndexBuffer");
    DX::ThrowIfFailed(result);
  }

  ID3DBlob *pVertexShaderCode = nullptr;
  result = DX::CompileAndCreateShader(
      L"shaders/ParticleInstance.vs",
      (ID3D11DeviceChild **)m_pVertexShader.GetAddressOf(), {},
      &pVertexShaderCode);
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/DiffuseInstance.vs",
      (ID3D11DeviceChild **)m_pDiffuseVertexShader.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/ParticleInstance.ps",
      (ID3D11DeviceChild **)m_pPixelShader.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = pDevice->CreateInputLayout(
      InputDesc, ARRAYSIZE(InputDesc), pVertexShaderCode->GetBufferPointer(),
      pVertexShaderCode->GetBufferSize(), &m_pInputLayout);
  DX::ThrowIfFailed(result);

  result = DX::SetResourceName(m_pInputLayout.Get(), "ParticleInputLayout");
  DX::ThrowIfFailed(result);

  SAFE_RELEASE(pVertexShaderCode);

  return result;
}

HRESULT SimRenderer::InitMarching() {
  auto pDevice = DeviceResources::getInstance().m_pDevice;

  // create input layout
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT result = S_OK;

  UINT cubeNums = (m_settings.marchingResolution.x + 1) *
                  (m_settings.marchingResolution.y + 1) *
                  (m_settings.marchingResolution.z + 1);

  UINT max_n = 2147483648;
  // UINT max_n = 125.f * 1024.f * 1024.f / (4 * 3);

  // Create vertex buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(max_n / sizeof(Vector3) * sizeof(Vector3));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = sizeof(Vector3);

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pMarchingVertexBuffer);
    DX::ThrowIfFailed(result);

    result = DX::SetResourceName(m_pMarchingVertexBuffer.Get(),
                                 "MarchingVertexBuffer");
    DX::ThrowIfFailed(result, "Failed to create vertex buffer");
  }

  // create out buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth =
        max_n / sizeof(MarchingOutBuffer) * sizeof(MarchingOutBuffer);
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(MarchingOutBuffer);

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pMarchingOutBuffer);
    DX::ThrowIfFailed(result, "Failed in out buffer");
    result =
        DX::SetResourceName(m_pMarchingOutBuffer.Get(), "MarchingOutBuffer");
    DX::ThrowIfFailed(result);
  }

  // create out buffer uav
  result = DX::CreateBufferUAV(
      m_pMarchingOutBuffer.Get(), max_n / sizeof(MarchingOutBuffer),
      "MarchingOutUAV", &m_pMarchingOutBufferUAV, D3D11_BUFFER_UAV_FLAG_APPEND);

  // create out buffer srv
  result = DX::CreateBufferSRV(m_pMarchingOutBuffer.Get(),
                               max_n / sizeof(MarchingOutBuffer),
                               "MarchingOutSRV", &m_pMarchingOutBufferSRV);

  // create counter buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(MarchingIndirectBuffer);
    desc.BindFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    desc.StructureByteStride = sizeof(MarchingIndirectBuffer);

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &m_MarchingIndirectBuffer;
    data.SysMemPitch = sizeof(MarchingIndirectBuffer);

    result = pDevice->CreateBuffer(&desc, &data, &m_pCountBuffer);
    DX::ThrowIfFailed(result, "Failed in indirect buffer");
    result = DX::SetResourceName(m_pCountBuffer.Get(), "CountBuffer");
    DX::ThrowIfFailed(result);
  }

  // create Voxel Grid buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = cubeNums * sizeof(float);
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(float);

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pVoxelGridBuffer1);
    DX::ThrowIfFailed(result, "Failed in voxel buffer");
    result = DX::SetResourceName(m_pVoxelGridBuffer1.Get(), "VoxelGridBuffer");
    DX::ThrowIfFailed(result);
  }

  // create VoxelGrid UAV
  {
    result = DX::CreateBufferUAV(m_pVoxelGridBuffer1.Get(), cubeNums,
                                 "VoxelGridUAV", &m_pVoxelGridBufferUAV1);
    DX::ThrowIfFailed(result, "Failed in voxel UAV");
  }

  // create VoxelGrid for filters
  {
    DX::ThrowIfFailed(DX::CreateStructuredBuffer<float>(
        cubeNums, D3D11_USAGE_DEFAULT, 0, nullptr, "FilteredGrid",
        &m_pVoxelGridBuffer2));

    DX::ThrowIfFailed(DX::CreateBufferUAV(m_pVoxelGridBuffer2.Get(), cubeNums,
                                          "FilteredGridUAV",
                                          &m_pVoxelGridBufferUAV2));
  }

  // create surface buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(SurfaceBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(SurfaceBuffer);

    DX::ThrowIfFailed(pDevice->CreateBuffer(&desc, nullptr, &m_pSurfaceBuffer),
                      "Failed to create surfaceBuffer");

    DX::ThrowIfFailed(
        DX::SetResourceName(m_pSurfaceBuffer.Get(), "SurfaceBuffer"),
        "Failed to set SurfaceBuffer resource name");
  }

  // create surface buffer UAV
  {
    result = DX::CreateBufferUAV(m_pSurfaceBuffer.Get(), 1, "SurfaceBufferUAV",
                                 &m_pSurfaceBufferUAV);
    DX::ThrowIfFailed(result, "Failed in surface UAV");
  }

  // create filtered voxel buffer
  {
    DX::ThrowIfFailed(DX::CreateStructuredBuffer<int>(
        cubeNums, D3D11_USAGE_DEFAULT, 0, nullptr, "FilteredVoxelBuffer",
        &m_pFilteredVoxelBuffer));

    DX::ThrowIfFailed(DX::CreateBufferUAV(m_pFilteredVoxelBuffer.Get(),
                                          cubeNums, "FilteredVoxelUAV",
                                          &m_pFilteredVoxelBufferUAV));
  }

  ID3DBlob *pVertexShaderCode = nullptr;
  result = DX::CompileAndCreateShader(
      L"shaders/MarchingCubes.vs",
      (ID3D11DeviceChild **)m_pMarchingVertexShader.GetAddressOf(), {},
      &pVertexShaderCode);
  DX::ThrowIfFailed(result);

  ID3DBlob *pVertexShaderCodeIndirect = nullptr;
  result = DX::CompileAndCreateShader(
      L"shaders/MarchingCubesIndirect.vs",
      (ID3D11DeviceChild **)m_pMarchingIndirectVertexShader.GetAddressOf(), {},
      &pVertexShaderCodeIndirect);
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/MarchingCubes.ps",
      (ID3D11DeviceChild **)m_pMarchingPixelShader.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/MarchingCubes.cs",
      (ID3D11DeviceChild **)m_pMarchingComputeShader.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/SurfaceCounter.cs",
      (ID3D11DeviceChild **)m_pSurfaceCountCS.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/SmoothingPreprocess.cs",
      (ID3D11DeviceChild **)m_pSmoothingPreprocessCS.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/BoxFilter.cs",
      (ID3D11DeviceChild **)m_pBoxFilterCS.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/GaussFilter.cs",
      (ID3D11DeviceChild **)m_pGaussFilterCS.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = pDevice->CreateInputLayout(
      InputDesc, ARRAYSIZE(InputDesc), pVertexShaderCode->GetBufferPointer(),
      pVertexShaderCode->GetBufferSize(), &m_pMarchingInputLayout);
  DX::ThrowIfFailed(result);

  result =
      DX::SetResourceName(m_pMarchingInputLayout.Get(), "MarchingInputLayout");
  DX::ThrowIfFailed(result);

  SAFE_RELEASE(pVertexShaderCode);
  SAFE_RELEASE(pVertexShaderCodeIndirect);

  return result;
}

void SimRenderer::Update(float dt) {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;
  if (m_settings.cpu) {
    m_sphAlgo.Update(dt, m_particles);
    if (m_settings.marching) {
      m_marchingCubesAlgo.march(m_vertex);
      HRESULT result = S_OK;
      D3D11_MAPPED_SUBRESOURCE resource;
      result = pContext->Map(m_pMarchingVertexBuffer.Get(), 0,
                             D3D11_MAP_WRITE_DISCARD, 0, &resource);
      DX::ThrowIfFailed(result);
      memcpy(resource.pData, m_vertex.data(),
             sizeof(Vector3) * m_vertex.size());
      pContext->Unmap(m_pMarchingVertexBuffer.Get(), 0);
    } else {
      pContext->UpdateSubresource(m_sphGpuAlgo.m_pSphDataBuffer.Get(), 0,
                                  nullptr, m_particles.data(), 0, 0);
    }
  } else {
    try {
      m_sphGpuAlgo.Update();
    } catch (std::exception &e) {
      std::cerr << "Sph GPU Update failed!" << std::endl;
      std::cerr << "  " << e.what() << std::endl;
    }
    if (m_settings.marching) {
      pContext->Begin(m_pQueryDisjoint[m_frameNum % 2]);
      pContext->End(m_pQueryMarchingStart[m_frameNum % 2]);
      UINT cubeNums = (m_settings.marchingResolution.x + 1) *
                      (m_settings.marchingResolution.y + 1) *
                      (m_settings.marchingResolution.z + 1);

      UINT groupNumber = DivUp(cubeNums, m_settings.blockSize);
      ID3D11Buffer *cb[1] = {m_sphGpuAlgo.m_pSphCB.Get()};
      pContext->CSSetConstantBuffers(0, 1, cb);

      if (m_frameNum == 0) {
        ID3D11UnorderedAccessView *uavs[1] = {m_pSurfaceBufferUAV.Get()};
        ID3D11ShaderResourceView *srvs[2] = {
            m_sphGpuAlgo.m_pEntriesBufferSRV.Get(),
            m_sphGpuAlgo.m_pHashBufferSRV.Get()};

        pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
        pContext->CSSetShaderResources(0, 2, srvs);
        pContext->CSSetShader(m_pSurfaceCountCS.Get(), nullptr, 0);
        pContext->Dispatch(groupNumber, 1, 1);

        ID3D11UnorderedAccessView *uavsNULL[1] = {nullptr};
        ID3D11ShaderResourceView *srvsNULL[2] = {nullptr, nullptr};
        pContext->CSSetUnorderedAccessViews(0, 1, uavsNULL, nullptr);
        pContext->CSSetShaderResources(0, 2, srvsNULL);
      }

      {
        ID3D11UnorderedAccessView *uavs[2] = {m_pVoxelGridBufferUAV1.Get(),
                                              m_pSurfaceBufferUAV.Get()};
        ID3D11ShaderResourceView *srvs[2] = {
            m_sphGpuAlgo.m_pEntriesBufferSRV.Get(),
            m_sphGpuAlgo.m_pHashBufferSRV.Get()};

        pContext->CSSetShaderResources(0, 2, srvs);
        pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

        pContext->CSSetShader(m_pSmoothingPreprocessCS.Get(), nullptr, 0);
        // pContext->CSSetShader(m_pMarchingPreprocessCS.Get(), nullptr, 0);

        pContext->Dispatch(groupNumber, 1, 1);

        ID3D11UnorderedAccessView *uavsNULL[2] = {nullptr, nullptr};
        ID3D11ShaderResourceView *srvsNULL[2] = {nullptr, nullptr};
        pContext->CSSetUnorderedAccessViews(0, 2, uavsNULL, nullptr);
        pContext->CSSetShaderResources(0, 2, srvsNULL);
      }

      {
        ID3D11UnorderedAccessView *uavs[2] = {m_pVoxelGridBufferUAV1.Get(),
                                              m_pVoxelGridBufferUAV2.Get()};
        pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
        pContext->CSSetShader(m_pBoxFilterCS.Get(), nullptr, 0);
        pContext->Dispatch(groupNumber, 1, 1);
      }

      {
        ID3D11UnorderedAccessView *uavs[2] = {m_pVoxelGridBufferUAV2.Get(),
                                              m_pVoxelGridBufferUAV1.Get()};
        pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
        pContext->CSSetShader(m_pGaussFilterCS.Get(), nullptr, 0);
        pContext->Dispatch(groupNumber, 1, 1);
      }

      pContext->End(m_pQueryMarchingPreprocess[m_frameNum % 2]);
      {
        const UINT pCounters[3] = {0, 0, 0};
        ID3D11UnorderedAccessView *uavs[3] = {m_pVoxelGridBufferUAV1.Get(),
                                              m_pMarchingOutBufferUAV.Get(),
                                              m_pSurfaceBufferUAV.Get()};

        pContext->CSSetUnorderedAccessViews(0, 3, uavs, pCounters);

        pContext->CSSetShader(m_pMarchingComputeShader.Get(), nullptr, 0);

        pContext->Dispatch(groupNumber, 1, 1);

        ID3D11UnorderedAccessView *uavsNULL[3] = {nullptr, nullptr, nullptr};
        pContext->CSSetUnorderedAccessViews(0, 3, uavsNULL, nullptr);

        pContext->CopyStructureCount(m_pCountBuffer.Get(), sizeof(UINT),
                                     m_pMarchingOutBufferUAV.Get());
      }
      pContext->End(m_pQueryMarchingMain[m_frameNum % 2]);
      pContext->End(m_pQueryDisjoint[m_frameNum % 2]);
    }
  }
}

void SimRenderer::Render(Vector4 cameraPos, ID3D11Buffer *pSceneBuffer) {
  ImGui::Begin("Simulation");
  if (m_settings.diffuseEnabled) {
    RenderDiffuse(pSceneBuffer);
  }
  if (m_settings.marching) {
    RenderMarching(cameraPos, pSceneBuffer);
  } else {
    RenderSpheres(pSceneBuffer);
  }
  m_sphGpuAlgo.ImGuiRender();
  ImGuiRender();
  m_frameNum++;
  ImGui::End();
}

void SimRenderer::RenderMarching(Vector4 cameraPos,
                                 ID3D11Buffer *pSceneBuffer) {
  auto dxResources = DeviceResources::getInstance();

  auto pContext = dxResources.m_pDeviceContext;
  auto pTransDepthState = dxResources.m_pTransDepthState;
  auto pTransBlendState = dxResources.m_pTransBlendState;
  pContext->OMSetDepthStencilState(m_states->DepthReverseZ(), 0);
  pContext->OMSetBlendState(m_states->AlphaBlend(), nullptr, 0xffffffff);

  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->PSSetShader(m_pMarchingPixelShader.Get(), nullptr, 0);

  ID3D11Buffer *cbuffers[1] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);

  if (m_settings.cpu) {
    ID3D11Buffer *vertexBuffers[] = {m_pMarchingVertexBuffer.Get()};
    UINT strides[] = {sizeof(Vector3)};
    UINT offsets[] = {0};
    pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pContext->IASetInputLayout(m_pMarchingInputLayout.Get());
    pContext->VSSetShader(m_pMarchingVertexShader.Get(), nullptr, 0);
    pContext->Draw(m_vertex.size(), 0);
  } else {
    pContext->IASetInputLayout(nullptr);
    pContext->VSSetShader(m_pMarchingIndirectVertexShader.Get(), nullptr, 0);

    ID3D11ShaderResourceView *srvs = {m_pMarchingOutBufferSRV.Get()};
    pContext->VSSetShaderResources(0, 1, &srvs);
    pContext->DrawInstancedIndirect(m_pCountBuffer.Get(), 0);

    ID3D11ShaderResourceView *nullSRV[1] = {nullptr};
    pContext->VSSetShaderResources(0, 1, nullSRV);
  }
}

void SimRenderer::RenderDiffuse(ID3D11Buffer *pSceneBuffer) {
  auto dxResources = DeviceResources::getInstance();
  auto pContext = dxResources.m_pDeviceContext;
  pContext->OMSetDepthStencilState(m_states->DepthReverseZ(), 0);
  pContext->OMSetBlendState(m_states->AlphaBlend(), nullptr, 0xffffffff);

  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
  pContext->VSSetShader(m_pDiffuseVertexShader.Get(), nullptr, 0);
  pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

  pContext->VSSetConstantBuffers(0, 1, &pSceneBuffer);
  pContext->VSSetShaderResources(
      0, 1, m_sphGpuAlgo.m_pDiffuseBufferSRV1.GetAddressOf());

  D3D11_MAPPED_SUBRESOURCE resource;
  HRESULT result = pContext->Map(m_sphGpuAlgo.m_pStateBuffer.Get(), 0,
                                 D3D11_MAP_READ, 0, &resource);
  DX::ThrowIfFailed(result);
  SphStateBuffer buf;
  memcpy(&buf, resource.pData, sizeof(SphStateBuffer));
  pContext->Unmap(m_sphGpuAlgo.m_pStateBuffer.Get(), 0);

  pContext->DrawInstanced(1, buf.diffuseNum, 0, 0);
}

void SimRenderer::RenderSpheres(ID3D11Buffer *pSceneBuffer) {
  auto dxResources = DeviceResources::getInstance();
  auto pContext = dxResources.m_pDeviceContext;
  pContext->OMSetDepthStencilState(dxResources.m_pDepthState.Get(), 0);
  pContext->OMSetBlendState(dxResources.m_pOpaqueBlendState.Get(), nullptr,
                            0xffffffff);

  pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer.Get()};
  UINT strides[] = {sizeof(Vector3)};
  UINT offsets[] = {0, 0};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

  pContext->IASetInputLayout(m_pInputLayout.Get());
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
  pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

  ID3D11Buffer *cbuffers[1] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->VSSetShaderResources(0, 1,
                                 m_sphGpuAlgo.m_pSphBufferSRV.GetAddressOf());

  cbuffers[0] = m_sphGpuAlgo.m_pSphCB.Get();
  pContext->PSSetShaderResources(0, 1,
                                 m_sphGpuAlgo.m_pSphBufferSRV.GetAddressOf());
  pContext->PSSetConstantBuffers(0, 1, cbuffers);
  pContext->DrawIndexedInstanced(m_sphereIndexCount, m_num_particles, 0, 0, 0);
}

void SimRenderer::CollectTimestamps() {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;

  while (pContext->GetData(m_pQueryDisjoint[(m_frameNum + 1) % 2], nullptr, 0,
                           0) == S_FALSE)
    ;

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
  pContext->GetData(m_pQueryDisjoint[(m_frameNum + 1) % 2], &tsDisjoint,
                    sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);

  if (tsDisjoint.Disjoint) {
    return;
  }

  // Get all the timestamps
  UINT64 tsMarchingBegin, tsMarchingPreproc, tsMarchingMain;

  pContext->GetData(m_pQueryMarchingStart[(m_frameNum + 1) % 2],
                    &tsMarchingBegin, sizeof(UINT64), 0);
  pContext->GetData(m_pQueryMarchingPreprocess[(m_frameNum + 1) % 2],
                    &tsMarchingPreproc, sizeof(UINT64), 0);
  pContext->GetData(m_pQueryMarchingMain[(m_frameNum + 1) % 2], &tsMarchingMain,
                    sizeof(UINT64), 0);

  // Convert to real time
  m_marchingPrep = float(tsMarchingPreproc - tsMarchingBegin) /
                   float(tsDisjoint.Frequency) * 1000.0f;
  m_marchingMain = float(tsMarchingMain - tsMarchingPreproc) /
                   float(tsDisjoint.Frequency) * 1000.0f;
  m_marchingSum += m_marchingPrep + m_marchingMain;
}

void SimRenderer::ImGuiRender() {
  CollectTimestamps();
  if (ImGui::CollapsingHeader("Time"), ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("MarchingCubes pass: %.3f ms", m_marchingMain + m_marchingPrep);
  }
  if (ImGui::CollapsingHeader("MarchingCubes"),
      ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("Preprocess time: %.3f ms", m_marchingPrep);
    ImGui::Text("Main time: %.3f ms", m_marchingMain);
    ImGui::Text("Avg time: %.3f ms", m_marchingSum / m_frameNum);
  }
}
