#include "water.h"

#include "SimpleMath.h"
#include "marching-cubes.h"

HRESULT Water::Init() { return Init(5); }

HRESULT Water::Init(UINT boxWidth) {
  auto pDevice = m_pDeviceResources->GetDevice();
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
      result = SetResourceName(m_pVertexBuffer, "ParticleVertexBuffer");
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
      result = SetResourceName(m_pIndexBuffer, "ParticleIndexBuffer");
    }
  }

  ID3DBlob *pSmallSphereVertexShaderCode = nullptr;
  if (SUCCEEDED(result)) {
    result = m_pDeviceResources->CompileAndCreateShader(L"../shaders/MarchingCubes.vs",
                                    (ID3D11DeviceChild **)&m_pVertexShader, {},
                                    &pSmallSphereVertexShaderCode);
  }
  if (SUCCEEDED(result)) {
    result = m_pDeviceResources->CompileAndCreateShader(L"../shaders/MarchingCubes.ps",
                                    (ID3D11DeviceChild **)&m_pPixelShader);
  }

  if (SUCCEEDED(result)) {
    result = pDevice->CreateInputLayout(
        InputDesc, ARRAYSIZE(InputDesc),
        pSmallSphereVertexShaderCode->GetBufferPointer(),
        pSmallSphereVertexShaderCode->GetBufferSize(), &m_pInputLayout);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pInputLayout, "ParticleInputLayout");
    }
  }

  SAFE_RELEASE(pSmallSphereVertexShaderCode);

  return result;
}

void Water::Update(float dt) {
  auto pContext = m_pDeviceResources->GetDeviceContext();
  m_heightfield->Update(dt);

  m_heightfield->GetRenderData(m_vertecies, m_indecies);
  pContext->UpdateSubresource(m_pVertexBuffer, 0, nullptr,
                                              m_vertecies.data(), 0, 0);
  pContext->UpdateSubresource(m_pIndexBuffer, 0, nullptr,
                                              m_indecies.data(), 0, 0);
}

void Water::Render(ID3D11Buffer *pSceneBuffer) {
  auto pContext = m_pDeviceResources->GetDeviceContext();
  pContext->OMSetDepthStencilState(m_pDeviceResources->GetDepthState(), 0);
  pContext->OMSetBlendState(m_pDeviceResources->GetTransBlendState(), nullptr, 0xffffffff);

  pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer};
  UINT strides[] = {sizeof(Vector3)};
  UINT offsets[] = {0, 0};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
  pContext->IASetInputLayout(m_pInputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader, nullptr, 0);
  pContext->PSSetShader(m_pPixelShader, nullptr, 0);
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
