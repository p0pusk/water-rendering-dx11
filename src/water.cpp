#include "water.h"

HRESULT Water::Init() { return Init(m_numParticles); }

HRESULT Water::Init(int numParticles) {
  m_numParticles = numParticles;
  m_pSph = new SPH(m_numParticles);
  m_pSph->Init();

  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0}};

  HRESULT result = S_OK;

  static const size_t SphereSteps = 32;

  std::vector<Vector3> sphereVertices;
  std::vector<UINT16> indices;

  size_t indexCount;
  size_t vertexCount;

  GetSphereDataSize(SphereSteps, SphereSteps, indexCount, vertexCount);

  sphereVertices.resize(vertexCount);
  indices.resize(indexCount);

  m_sphereIndexCount = (UINT)indexCount;

  CreateSphere(SphereSteps, SphereSteps, indices.data(), sphereVertices.data());

  for (auto &v : sphereVertices) {
    v = v * 0.1f;
  }

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

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pVertexBuffer, "ParticleVertexBuffer");
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

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pIndexBuffer, "ParticleIndexBuffer");
    }
  }

  ID3DBlob *pSmallSphereVertexShaderCode = nullptr;
  if (SUCCEEDED(result)) {
    result = CompileAndCreateShader(L"../shaders/Particle.vs",
                                    (ID3D11DeviceChild **)&m_pVertexShader, {},
                                    &pSmallSphereVertexShaderCode);
  }
  if (SUCCEEDED(result)) {
    result = CompileAndCreateShader(L"../shaders/Particle.ps",
                                    (ID3D11DeviceChild **)&m_pPixelShader);
  }

  if (SUCCEEDED(result)) {
    result = m_pDXC->m_pDevice->CreateInputLayout(
        InputDesc, 1, pSmallSphereVertexShaderCode->GetBufferPointer(),
        pSmallSphereVertexShaderCode->GetBufferSize(), &m_pInputLayout);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pInputLayout, "ParticleInputLayout");
    }
  }

  SAFE_RELEASE(pSmallSphereVertexShaderCode);

  // Create geometry buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(GeomBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    m_gb.color = {0, 0.25, 0.75, 1};
    for (int i = 0; i < m_numParticles; i++) {
      m_gb.m[i] = DirectX::XMMatrixIdentity();
    }

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &m_gb;
    data.SysMemPitch = sizeof(m_gb);
    data.SysMemSlicePitch = 0;
    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pGeomBuffer);

    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pGeomBuffer, "ParticlesGeomBuffer");
    }
  }

  return result;
}

void Water::Update(float dt) {
  m_pSph->Update(dt);
  for (int i = 0; i < m_numParticles; i++) {
    auto &pos = m_pSph->m_particles[i].position;
    m_gb.m[i] = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
  }
  m_pDXC->m_pDeviceContext->UpdateSubresource(m_pGeomBuffer, 0, nullptr, &m_gb,
                                              0, 0);
}

void Water::Render(ID3D11Buffer *pSceneBuffer) {
  auto &pContext = m_pDXC->m_pDeviceContext;
  pContext->OMSetDepthStencilState(m_pDXC->m_pDepthState, 0);
  pContext->OMSetBlendState(m_pDXC->m_pOpaqueBlendState, nullptr, 0xffffffff);

  pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer};
  UINT strides[] = {sizeof(Vector3)};
  UINT offsets[] = {0};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
  pContext->IASetInputLayout(m_pInputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader, nullptr, 0);
  pContext->PSSetShader(m_pPixelShader, nullptr, 0);
  ID3D11Buffer *cbuffers[] = {pSceneBuffer, m_pGeomBuffer};
  pContext->VSSetConstantBuffers(0, 2, cbuffers);
  pContext->PSSetConstantBuffers(0, 2, cbuffers);
  pContext->DrawIndexedInstanced(m_sphereIndexCount, m_numParticles, 0, 0, 0);
}
