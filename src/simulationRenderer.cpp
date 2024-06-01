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
#include "mc-gpu.h"
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
    m_mcGpu = std::make_unique<MCGpu>(m_settings);
    m_mcGpu->Init();
  } catch (std::exception &e) {
    std::cerr << "MC gpu init failed!" << std::endl;
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

  ID3DBlob *pVertexShaderCode = nullptr;
  result = DX::CompileAndCreateShader(
      L"shaders/MarchingCubes.vs",
      (ID3D11DeviceChild **)m_pMarchingVertexShader.GetAddressOf(), {},
      &pVertexShaderCode);
  DX::ThrowIfFailed(result);

  result = pDevice->CreateInputLayout(
      InputDesc, ARRAYSIZE(InputDesc), pVertexShaderCode->GetBufferPointer(),
      pVertexShaderCode->GetBufferSize(), &m_pMarchingInputLayout);
  DX::ThrowIfFailed(result);

  result =
      DX::SetResourceName(m_pMarchingInputLayout.Get(), "MarchingInputLayout");
  DX::ThrowIfFailed(result);

  SAFE_RELEASE(pVertexShaderCode);

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
      m_mcGpu->Update(m_sphGpuAlgo);
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
  m_mcGpu->ImGuiRender();
  m_frameNum++;
  ImGui::End();
}

void SimRenderer::RenderMarching(Vector4 cameraPos,
                                 ID3D11Buffer *pSceneBuffer) {
  if (m_settings.cpu) {
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
    ID3D11Buffer *vertexBuffers[] = {m_pMarchingVertexBuffer.Get()};
    UINT strides[] = {sizeof(Vector3)};
    UINT offsets[] = {0};
    pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pContext->IASetInputLayout(m_pMarchingInputLayout.Get());
    pContext->VSSetShader(m_pMarchingVertexShader.Get(), nullptr, 0);
    pContext->Draw(m_vertex.size(), 0);
  } else {
    m_mcGpu->Render(pSceneBuffer);
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
