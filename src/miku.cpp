#include "miku.h"
#include "VertexTypes.h"
#include "WICTextureLoader.h"
#include "pch.h"
#include "utils.h"

void Miku::Init() {
  auto pDevice = DeviceResources::getInstance().m_pDevice.Get();
  auto pContext = DeviceResources::getInstance().m_pDeviceContext.Get();
  m_states = std::make_unique<DirectX::CommonStates>(pDevice);
  m_fxFactory = std::make_unique<DirectX::EffectFactory>(pDevice);
  DX::ThrowIfFailed(
      CreateWICTextureFromFile(pDevice, L"./Common/miku.png", nullptr,
                               m_pMikuTexture.ReleaseAndGetAddressOf()));
  m_model = Model::CreateFromCMO(pDevice, L"./Common/miku.cmo", *m_fxFactory);

  {
    ID3DBlob *pVertexShaderCode;

    DX::ThrowIfFailed(DX::CompileAndCreateShader(
        L"shaders/SimpleTexture.vs",
        (ID3D11DeviceChild **)m_vertexShader.GetAddressOf(), {},
        &pVertexShaderCode));

    DX::ThrowIfFailed(DX::CompileAndCreateShader(
        L"shaders/SimpleTexture.ps",
        (ID3D11DeviceChild **)m_pixelShader.GetAddressOf(), {}));

    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
         D3D11_INPUT_PER_VERTEX_DATA, 0}};

    DX::ThrowIfFailed(
        pDevice->CreateInputLayout(
            VertexPositionNormalTangentColorTexture::InputElements,
            VertexPositionNormalTangentColorTexture::InputElementCount,
            pVertexShaderCode->GetBufferPointer(),
            pVertexShaderCode->GetBufferSize(), &m_pInputLayout),
        "Failed in inputLayout");
  }

  try {
    GeomBuffer geomBuffer;
    geomBuffer.m = XMMatrixScaling(0.02f, 0.02f, 0.02f) *
                   XMMatrixTranslation(0.0f, 0, 0.0f) *
                   XMMatrixRotationY(acos(-1));
    geomBuffer.norm = XMMatrixInverse(nullptr, XMMatrixTranspose(geomBuffer.m));
    geomBuffer.shine = Vector4(1.0f, 0, 0, 0);

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &geomBuffer;
    data.SysMemPitch = sizeof(geomBuffer);
    data.SysMemSlicePitch = 0;

    DX::CreateConstantBuffer<GeomBuffer>(&m_pGeomBuffer, 0, &data,
                                         "MikuGeomBuffer");
  } catch (...) {
    throw;
  }
}

void Miku::Render(ID3D11Buffer *pSceneBuffer) {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext.Get();
  ModelMesh::SetDepthBufferMode(true);
  m_world = XMMatrixIdentity();
  m_view = XMMatrixIdentity();
  m_proj = XMMatrixIdentity();

  m_model->Draw(pContext, *m_states, m_world, m_view, m_proj, false, [&]() {
    ID3D11Buffer *cbuffers[2] = {pSceneBuffer, m_pGeomBuffer.Get()};
    pContext->IASetInputLayout(m_pInputLayout.Get());
    pContext->VSSetConstantBuffers(0, 2, cbuffers);
    pContext->PSSetConstantBuffers(0, 2, cbuffers);

    pContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    pContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    ID3D11ShaderResourceView *srvs[1] = {m_pMikuTexture.Get()};

    pContext->PSSetShaderResources(0, 1, srvs);
    auto samplerState = m_states->LinearWrap();
    pContext->PSSetSamplers(0, 1, &samplerState);
    pContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xffffffff);
    pContext->OMSetDepthStencilState(m_states->DepthReverseZ(), 0);
  });
}
