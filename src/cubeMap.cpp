#include "cubemap.h"

#include "DDSTextureLoader.h"
#include "utils.h"
#include <iostream>

HRESULT CubeMap::InitCubemap() {
  HRESULT result = S_OK;

  auto dxResources = DeviceResources::getInstance();
  auto pDevice = dxResources.m_pDevice;
  auto pContext = dxResources.m_pDeviceContext;

  if (SUCCEEDED(result)) {
    result = DirectX::CreateDDSTextureFromFileEx(
        pDevice.Get(), pContext.Get(), L"Common/station.dds", 0,
        D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0,
        D3D11_RESOURCE_MISC_TEXTURECUBE, DirectX::DDS_LOADER_DEFAULT, nullptr,
        &m_pCubemapView);
  }

  assert(SUCCEEDED(result));

  if (SUCCEEDED(result)) {
    result = DX::SetResourceName(m_pCubemapView.Get(), "CubemapView");
  }

  return result;
}

HRESULT CubeMap::Init() {
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0}};

  HRESULT result = S_OK;
  auto pDevice = DeviceResources::getInstance().m_pDevice;

  static const size_t SphereSteps = 32;

  std::vector<Vector3> sphereVertices;
  std::vector<UINT16> indices;

  size_t indexCount;
  size_t vertexCount;

  DX::GetSphereDataSize(SphereSteps, SphereSteps, indexCount, vertexCount);

  sphereVertices.resize(vertexCount);
  indices.resize(indexCount);

  m_sphereIndexCount = indexCount;

  DX::CreateSphere(SphereSteps, SphereSteps, indices.data(),
                   sphereVertices.data());

  // Create vertex buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(sphereVertices.size() * sizeof(Vector3));
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = sphereVertices.data();
    data.SysMemPitch = (UINT)(sphereVertices.size() * sizeof(Vector3));
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = DX::SetResourceName(m_pVertexBuffer.Get(), "SphereVertexBuffer");
    }
  }

  // Create index buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(indices.size() * sizeof(UINT16));
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = indices.data();
    data.SysMemPitch = (UINT)(indices.size() * sizeof(UINT16));
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = DX::SetResourceName(m_pIndexBuffer.Get(), "SphereIndexBuffer");
    }
  }

  ID3DBlob *pSphereVertexShaderCode = nullptr;
  try {
    DX::CompileAndCreateShader(
        L"shaders/SphereTexture.vs",
        (ID3D11DeviceChild **)m_pVertexShader.GetAddressOf(), {},
        &pSphereVertexShaderCode);
    DX::CompileAndCreateShader(
        L"shaders/SphereTexture.ps",
        (ID3D11DeviceChild **)m_pPixelShader.GetAddressOf());
  } catch (...) {
    throw;
  }

  if (SUCCEEDED(result)) {
    result = pDevice->CreateInputLayout(
        InputDesc, 1, pSphereVertexShaderCode->GetBufferPointer(),
        pSphereVertexShaderCode->GetBufferSize(), &m_pInputLayout);
  }

  SAFE_RELEASE(pSphereVertexShaderCode);

  // Create geometry buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(SphereGeomBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    SphereGeomBuffer geomBuffer;
    geomBuffer.m = DirectX::XMMatrixIdentity();
    geomBuffer.size = 2.0f;
    // geomBuffer.m = DirectX::XMMatrixTranslation(2.0f, 0.0f, 0.0f);

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &geomBuffer;
    data.SysMemPitch = sizeof(geomBuffer);
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pGeomBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = DX::SetResourceName(m_pGeomBuffer.Get(), "SphereGeomBuffer");
    }
  }

  InitCubemap();

  return result;
}

void CubeMap::Render(ID3D11Buffer *pSceneBuffer) {
  auto dxResources = DeviceResources::getInstance();
  auto pContext = dxResources.m_pDeviceContext;
  auto pSampler = dxResources.m_pSampler;
  auto pDepthState = dxResources.m_pDepthState;

  ID3D11SamplerState *samplers[] = {pSampler.Get()};
  pContext->PSSetSamplers(0, 1, samplers);

  ID3D11ShaderResourceView *resources[] = {m_pCubemapView.Get()};
  pContext->PSSetShaderResources(0, 1, resources);

  pContext->OMSetDepthStencilState(pDepthState.Get(), 0);

  pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer.Get()};
  UINT strides[] = {12};
  UINT offsets[] = {0};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
  pContext->IASetInputLayout(m_pInputLayout.Get());

  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
  pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

  ID3D11Buffer *cbuffers[] = {pSceneBuffer, m_pGeomBuffer.Get()};
  pContext->VSSetConstantBuffers(0, 2, cbuffers);
  pContext->DrawIndexed(m_sphereIndexCount, 0, 0);
}

void CubeMap::Resize(UINT width, UINT height) {
  // Setup skybox sphere
  float n = 0.1f;
  float fov = M_PI / 3;
  float halfW = tanf(fov / 2) * n;
  float halfH = (float)height / width * halfW;

  float r = sqrtf(n * n + halfH * halfH + halfW * halfW) * 1.1f * 2.0f;

  SphereGeomBuffer geomBuffer;
  geomBuffer.m = DirectX::XMMatrixIdentity();
  geomBuffer.size = r;
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;
  pContext->UpdateSubresource(m_pGeomBuffer.Get(), 0, nullptr, &geomBuffer, 0,
                              0);
}
