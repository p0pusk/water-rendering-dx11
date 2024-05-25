#include "terrain.h"

#include "VertexTypes.h"
#include "WICTextureLoader.h"
#include "pch.h"
#include "utils.h"

void Terrain::Init() {
  auto pDevice = DeviceResources::getInstance().m_pDevice.Get();
  auto pContext = DeviceResources::getInstance().m_pDeviceContext.Get();
  m_states = std::make_unique<DirectX::CommonStates>(pDevice);
  m_fxFactory = std::make_unique<DirectX::EffectFactory>(pDevice);
  m_model =
      Model::CreateFromSDKMESH(pDevice, L"./Common/city.sdkmesh", *m_fxFactory);

  {
    ID3DBlob *pVertexShaderCode;

    DX::ThrowIfFailed(DX::CompileAndCreateShader(
        L"shaders/SimpleTexture.vs",
        (ID3D11DeviceChild **)m_vertexShader.GetAddressOf(), {},
        &pVertexShaderCode));

    DX::ThrowIfFailed(DX::CompileAndCreateShader(
        L"shaders/Surface.ps",
        (ID3D11DeviceChild **)m_pixelShader.GetAddressOf(), {}));

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
                   XMMatrixRotationY(acos(-1)) *
                   XMMatrixTranslation(0.0, 0, 0.0);
    geomBuffer.norm = XMMatrixInverse(nullptr, XMMatrixTranspose(geomBuffer.m));
    geomBuffer.shines = Vector4(100, 0, 0, 0);
    geomBuffer.color = Vector4(1, 1, 1, 1);

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &geomBuffer;
    data.SysMemPitch = sizeof(geomBuffer);
    data.SysMemSlicePitch = 0;

    DX::CreateConstantBuffer<GeomBuffer>(&m_pGeomBuffer, 0, &data,
                                         "TerrainGeomBuffer");
  } catch (...) {
    throw;
  }
}

void Terrain::Render(ID3D11Buffer *pSceneBuffer) {
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

    // pContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xffffffff);
    // pContext->OMSetDepthStencilState(m_states->DepthReverseZ(), 0);
  });
}
