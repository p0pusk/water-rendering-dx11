#include "surface.h"
#include "device-resources.h"
#include "utils.h"

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
  auto pDevice = DeviceResources::getInstance().m_pDevice;

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
      result = DX::SetResourceName(m_pVertexBuffer.Get(), "RectVertexBuffer");
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
      result = DX::SetResourceName(m_pIndexBuffer.Get(), "RectIndexBuffer");
    }
  }

  ID3DBlob *pRectVertexShaderCode = nullptr;
  if (SUCCEEDED(result)) {
    result = DX::CompileAndCreateShader(
        L"shaders/Surface.vs",
        (ID3D11DeviceChild **)m_pVertexShader.GetAddressOf(), {},
        &pRectVertexShaderCode);
  }

  if (SUCCEEDED(result)) {
    result = DX::CompileAndCreateShader(
        L"shaders/Surface.ps",
        (ID3D11DeviceChild **)m_pPixelShader.GetAddressOf(), {"NO_LIGHTS"}, {});
  }

  if (SUCCEEDED(result)) {
    result = pDevice->CreateInputLayout(
        InputDesc, 2, pRectVertexShaderCode->GetBufferPointer(),
        pRectVertexShaderCode->GetBufferSize(), &m_pInputLayout);
    if (SUCCEEDED(result)) {
      result = DX::SetResourceName(m_pInputLayout.Get(), "RectInputLayout");
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
      result = DX::SetResourceName(m_pGeomBuffer.Get(), "RectGeomBuffer");
    }
  }

  return result;
}

void Surface::Render(ID3D11Buffer *pSceneBuffer) {
  auto dxResources = DeviceResources::getInstance();
  auto pContext = dxResources.m_pDeviceContext;
  auto pSampler = dxResources.m_pSampler;
  auto pDepthState = dxResources.m_pDepthState;

  ID3D11SamplerState *samplers[] = {pSampler.Get()};
  pContext->PSSetSamplers(0, 1, samplers);

  ID3D11ShaderResourceView *resources[] = {m_pCubemapView.Get()};
  pContext->PSSetShaderResources(0, 1, resources);

  pContext->OMSetDepthStencilState(pDepthState.Get(), 0);

  auto pOpaqueBlendState = dxResources.m_pOpaqueBlendState;
  pContext->OMSetBlendState(pOpaqueBlendState.Get(), nullptr, 0xFFFFFFFF);

  pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer.Get()};
  UINT strides[] = {24};
  UINT offsets[] = {0};
  ID3D11Buffer *cbuffers[] = {pSceneBuffer, m_pGeomBuffer.Get()};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
  pContext->IASetInputLayout(m_pInputLayout.Get());
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
  pContext->VSSetConstantBuffers(0, 2, cbuffers);
  pContext->PSSetConstantBuffers(0, 2, cbuffers);
  pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

  pContext->DrawIndexed(6, 0, 0);
}
