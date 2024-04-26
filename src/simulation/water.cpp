#include "water.h"

#include "SimpleMath.h"
#include "device-resources.h"
#include "marching-cubes.h"
#include "utils.h"

HRESULT Water::Init(UINT boxWidth) {
  auto pDevice = DeviceResources::getInstance().m_pDevice;
  HeightField::Props p;
  p.gridSize = XMINT2(boxWidth, boxWidth);
  p.pos = Vector3(0 - boxWidth / 2 * p.w, 0, 0 - boxWidth / 2 * p.w);
  m_heightfield = new HeightField(p);

  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT result = S_OK;

  m_heightfield->GetRenderData(m_vertecies, m_indecies);

  // Create vertex buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(m_heightfield->m_grid.size() *
                            m_heightfield->m_grid[0].size() * sizeof(Vector3));
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = m_vertecies.data();
    data.SysMemPitch = (UINT)(m_vertecies.size() * sizeof(Vector3));
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result =
          DX::SetResourceName(m_pVertexBuffer.Get(), "ParticleVertexBuffer");
    }
  }

  // Create index buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(m_indecies.size() * sizeof(UINT16));
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = m_indecies.data();
    data.SysMemPitch = (UINT)(m_indecies.size() * sizeof(UINT16));
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = DX::SetResourceName(m_pIndexBuffer.Get(), "ParticleIndexBuffer");
    }
  }

  ID3DBlob *pSmallSphereVertexShaderCode = nullptr;
  if (SUCCEEDED(result)) {
    result = DX::CompileAndCreateShader(
        L"shaders/MarchingCubes.vs",
        (ID3D11DeviceChild **)m_pVertexShader.GetAddressOf(), {},
        &pSmallSphereVertexShaderCode);
  }
  if (SUCCEEDED(result)) {
    result =
        DX::CompileAndCreateShader(L"shaders/MarchingCubes.ps",
                                   (ID3D11DeviceChild **)m_pPixelShader.Get());
  }

  if (SUCCEEDED(result)) {
    result = pDevice->CreateInputLayout(
        InputDesc, ARRAYSIZE(InputDesc),
        pSmallSphereVertexShaderCode->GetBufferPointer(),
        pSmallSphereVertexShaderCode->GetBufferSize(), &m_pInputLayout);
    if (SUCCEEDED(result)) {
      result = DX::SetResourceName(m_pInputLayout.Get(), "ParticleInputLayout");
    }
  }

  SAFE_RELEASE(pSmallSphereVertexShaderCode);

  return result;
}

void Water::Update(float dt) {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;
  m_heightfield->Update(dt);

  m_heightfield->GetRenderData(m_vertecies, m_indecies);
  pContext->UpdateSubresource(m_pVertexBuffer.Get(), 0, nullptr,
                              m_vertecies.data(), 0, 0);
  pContext->UpdateSubresource(m_pIndexBuffer.Get(), 0, nullptr,
                              m_indecies.data(), 0, 0);
}

void Water::Render(ID3D11Buffer *pSceneBuffer) {
  auto dxResources = DeviceResources::getInstance();
  auto pContext = dxResources.m_pDeviceContext;
  pContext->OMSetDepthStencilState(dxResources.m_pDepthState.Get(), 0);
  pContext->OMSetBlendState(dxResources.m_pTransBlendState.Get(), nullptr,
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
  ID3D11Buffer *cbuffers[] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);
  pContext->DrawIndexed(m_indecies.size(), 0, 0);
}

void Water::StartImpulse(int x, int z, float value, float radius) {
  m_heightfield->StartImpulse(x, z, value, radius);
}

void Water::StopImpulse(int x, int z, float radius) {
  m_heightfield->StopImpulse(x, z, radius);
}
