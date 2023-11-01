#include "water.h"

HRESULT Water::Init() { return Init(5); }

HRESULT Water::Init(UINT boxWidth) {
  SPH::Props props;
  m_pSph = new SPH(props);
  m_pSph->Init();

  m_numParticles = m_pSph->m_particles.size();
  m_instanceData =
      (InstanceData *)malloc(sizeof(InstanceData) * m_numParticles);
  for (int i = 0; i < m_numParticles; i++) {
    m_instanceData[i].pos = m_pSph->m_particles[i].position;
    m_instanceData[i].density = m_pSph->m_particles[i].density;
  }

  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},

      {"INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,
       D3D11_INPUT_PER_INSTANCE_DATA, 1},
      {"INSTANCEDENSITY", 0, DXGI_FORMAT_R32_FLOAT, 1, 12,
       D3D11_INPUT_PER_INSTANCE_DATA, 1},
  };

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

  // Create Instance buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(sizeof(InstanceData) * m_numParticles);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = m_instanceData;
    data.SysMemPitch = (UINT)(sizeof(InstanceData) * m_numParticles);
    data.SysMemSlicePitch = 0;

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pInstanceBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pInstanceBuffer, "ParticleInstanceBuffer");
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
  m_pSph->Update(dt);
  for (int i = 0; i < m_numParticles; i++) {
    m_instanceData[i].pos = m_pSph->m_particles[i].position;
    m_instanceData[i].density = m_pSph->m_particles[i].density;
  }

  m_pDXC->m_pDeviceContext->UpdateSubresource(m_pInstanceBuffer, 0, nullptr,
                                              m_instanceData, 0, 0);
}

void Water::Render(ID3D11Buffer *pSceneBuffer) {
  auto &pContext = m_pDXC->m_pDeviceContext;
  pContext->OMSetDepthStencilState(m_pDXC->m_pDepthState, 0);
  pContext->OMSetBlendState(m_pDXC->m_pOpaqueBlendState, nullptr, 0xffffffff);

  pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer, m_pInstanceBuffer};
  UINT strides[] = {sizeof(Vector3), sizeof(InstanceData)};
  UINT offsets[] = {0, 0};
  pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
  pContext->IASetInputLayout(m_pInputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader, nullptr, 0);
  pContext->PSSetShader(m_pPixelShader, nullptr, 0);
  ID3D11Buffer *cbuffers[] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);
  pContext->DrawIndexedInstanced(m_sphereIndexCount, m_numParticles, 0, 0, 0);
}
