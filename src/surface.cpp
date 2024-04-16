#include "surface.h"

HRESULT Surface::Init() { return Init(Vector3::Zero); }

HRESULT Surface::Init(Vector3 pos) {
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
       D3D11_INPUT_PER_VERTEX_DATA, 0}};

  static const Vertex Vertices[] = {{{-1, 0, -1}, {0, 1, 0}},
                                    {{1, 0, -1}, {0, 1, 0}},
                                    {{1, 0, 1}, {0, 1, 0}},
                                    {{-1, 0, 1}, {0, 1, 0}}};

  static const UINT16 Indices[] = {0, 1, 2, 0, 2, 3};

  HRESULT result = S_OK;
  auto pDevice = m_pDeviceResources->GetDevice();

  // Create vertex buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)sizeof(Vertices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = Vertices;
    data.SysMemPitch = (UINT)sizeof(Vertices);
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pVertexBuffer, "RectVertexBuffer");
    }
  }

  // Create index buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)sizeof(Indices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = Indices;
    data.SysMemPitch = (UINT)sizeof(Indices);
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pIndexBuffer, "RectIndexBuffer");
    }
  }

  ID3DBlob *pRectVertexShaderCode = nullptr;
  if (SUCCEEDED(result)) {
    result = m_pDeviceResources->CompileAndCreateShader(
        L"shaders/Surface.vs", (ID3D11DeviceChild **)&m_pVertexShader, {},
        &pRectVertexShaderCode);
  }
  if (SUCCEEDED(result)) {
    result = m_pDeviceResources->CompileAndCreateShader(
        L"shaders/Surface.ps", (ID3D11DeviceChild **)&m_pPixelShader,
        {"NO_LIGHTS"}, {});
  }

  if (SUCCEEDED(result)) {
    result = pDevice->CreateInputLayout(
        InputDesc, 2, pRectVertexShaderCode->GetBufferPointer(),
        pRectVertexShaderCode->GetBufferSize(), &m_pInputLayout);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pInputLayout, "RectInputLayout");
    }
  }

  SAFE_RELEASE(pRectVertexShaderCode);

  // Create geometry buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(GeomBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    GeomBuffer geomBuffer;
    geomBuffer.m = DirectX::XMMatrixScaling(10, 10, 10) *
                   DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
    auto m = DirectX::XMMatrixInverse(nullptr, geomBuffer.m);
    m = DirectX::XMMatrixTranspose(m);
    geomBuffer.norm = m;
    geomBuffer.color = Vector4(0.75f, 0.75f, 0.75f, 1.f);

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &geomBuffer;
    data.SysMemPitch = sizeof(geomBuffer);
    data.SysMemSlicePitch = 0;

    result = pDevice->CreateBuffer(&desc, &data, &m_pGeomBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pGeomBuffer, "RectGeomBuffer");
    }
  }

  return result;
}

void Surface::Render(ID3D11Buffer *pSceneBuffer) {
  auto pContext = m_pDeviceResources->GetDeviceContext();
  auto pSampler = m_pDeviceResources->GetSampler();
  auto pDepthState = m_pDeviceResources->GetDepthState();

  ID3D11SamplerState *samplers[] = {pSampler};
  pContext->PSSetSamplers(0, 1, samplers);

  ID3D11ShaderResourceView *resources[] = {m_pCubemapView};
  pContext->PSSetShaderResources(0, 1, resources);

  pContext->OMSetDepthStencilState(pDepthState, 0);

  auto pOpaqueBlendState = m_pDeviceResources->GetOpaqueBlendState();
  pContext->OMSetBlendState(pOpaqueBlendState, nullptr, 0xFFFFFFFF);

  pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer};
  UINT strides[] = {24};
  UINT offsets[] = {0};
  ID3D11Buffer *cbuffers[] = {pSceneBuffer, m_pGeomBuffer};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
  pContext->IASetInputLayout(m_pInputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader, nullptr, 0);
  pContext->VSSetConstantBuffers(0, 2, cbuffers);
  pContext->PSSetConstantBuffers(0, 2, cbuffers);
  pContext->PSSetShader(m_pPixelShader, nullptr, 0);

  pContext->DrawIndexed(6, 0, 0);
}
