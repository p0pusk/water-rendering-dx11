#include "sph.h"

#include <stdint.h>

#include <algorithm>
#include <random>
#include <vector>

#include "SimpleMath.h"
#include "neighbour-hash.h"
#include "particle.h"

HRESULT SPH::Init() {
  HRESULT result = S_OK;
  try {
    InitSph();
  } catch (std::exception& e) {
    throw e;
  }
  try {
    result = InitMarching();
  } catch (std::exception& e) {
    throw e;
  }
  try {
    result = InitSpheres();
  } catch (std::exception& e) {
    throw e;
  }

  assert(SUCCEEDED(result));

  return result;
}

HRESULT SPH::InitSph() {
  HRESULT result = S_OK;

  // instantiate particles
  float& h = m_props.h;
  float& r = m_props.particleRadius;
  float separation = h - 0.01f;
  Vector3 offset = {h, h + m_props.cubeNum.y * m_props.cubeLen, h};

  m_particles.resize(m_props.cubeNum.x * m_props.cubeNum.y * m_props.cubeNum.z);

  for (int x = 0; x < m_props.cubeNum.x; x++) {
    for (int y = 0; y < m_props.cubeNum.y; y++) {
      for (int z = 0; z < m_props.cubeNum.z; z++) {
        size_t particleIndex =
            x + (y + z * m_props.cubeNum.y) * m_props.cubeNum.x;

        Particle& p = m_particles[particleIndex];
        p.position.x = m_props.pos.x + x * separation + offset.x;
        p.position.y = m_props.pos.y + y * separation + offset.y;
        p.position.z = m_props.pos.z + z * separation + offset.z;
        p.velocity = Vector4::Zero;
      }
    }
  }

  // create sph constant buffer
  {
    m_sphCB.pos = m_props.pos;
    m_sphCB.particleNum = m_num_particles;
    m_sphCB.cubeNum = m_props.cubeNum;
    m_sphCB.cubeLen = m_props.cubeLen;
    m_sphCB.h = m_props.h;
    m_sphCB.mass = m_props.mass;
    m_sphCB.dynamicViscosity = m_props.dynamicViscosity;
    m_sphCB.dampingCoeff = m_props.dampingCoeff;

    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(SphCB);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &m_sphCB;

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pSphCB);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pSphCB, "SphConstantBuffer");
    DX::ThrowIfFailed(result);
  }

  // create SPH DB
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(SphDB);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &m_sphDB;

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pSphDB);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pSphDB, "SphDynamicBuffer");
    DX::ThrowIfFailed(result);
  }

  // create data buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = m_num_particles * sizeof(Particle);
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(Particle);

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = m_particles.data();

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pSphDataBuffer);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pSphDataBuffer, "SphDataBuffer");
    DX::ThrowIfFailed(result);

    m_pDXC->m_pDeviceContext->UpdateSubresource(m_pSphDataBuffer, 0, nullptr,
                                                m_particles.data(), 0, 0);
  }

  // create SPH UAV
  {
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = m_num_particles;
    desc.Buffer.Flags = 0;

    result = m_pDXC->m_pDevice->CreateUnorderedAccessView(
        m_pSphDataBuffer, &desc, &m_pSphBufferUAV);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pSphBufferUAV, "SphBufferUAV");
    DX::ThrowIfFailed(result);
  }

  // create SPH constant buffer for vertex shader
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = m_num_particles * sizeof(Particle);
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = sizeof(Particle);

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, nullptr, &m_pSphBuffer);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pSphBuffer, "SphBuffer");
    DX::ThrowIfFailed(result);
  }

  result = CompileAndCreateShader(L"../shaders/Sph.cs",
                                  (ID3D11DeviceChild**)&m_pSPHComputeShader);
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pSPHComputeShader, "SphComputeShader");
  DX::ThrowIfFailed(result);

  return result;
}

HRESULT SPH::InitSpheres() {
  // get sphere data
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

  for (auto& v : sphereVertices) {
    v = v * m_props.particleRadius;
  }

  // create input layout
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},

      {"INSTANCEPOS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,
       D3D11_INPUT_PER_INSTANCE_DATA, 1},
      {"INSTANCEDENSITY", 0, DXGI_FORMAT_R32_FLOAT, 1, 12,
       D3D11_INPUT_PER_INSTANCE_DATA, 1},
  };

  HRESULT result = S_OK;

  // Create vertex buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(sphereVertices.size() * sizeof(Vector3));
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = sphereVertices.data();
    data.SysMemPitch = (UINT)(sphereVertices.size() * sizeof(Vector3));
    data.SysMemSlicePitch = 0;

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
    DX::ThrowIfFailed(result);

    result = SetResourceName(m_pVertexBuffer, "ParticleVertexBuffer");
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

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pIndexBuffer, "ParticleIndexBuffer");
    DX::ThrowIfFailed(result);
  }

  // create instance buffer
  {
    m_instanceData.resize(m_particles.size());
    for (int i = 0; i < m_particles.size(); i++) {
      m_instanceData[i].pos = m_particles[i].position;
      m_instanceData[i].density = m_particles[i].density;
    }

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(sizeof(InstanceData) * m_instanceData.size());
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = m_instanceData.data();
    data.SysMemPitch = (UINT)(sizeof(InstanceData) * m_instanceData.size());
    data.SysMemSlicePitch = 0;

    result = m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pInstanceBuffer);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pInstanceBuffer, "ParticleInstanceBuffer");
    DX::ThrowIfFailed(result);
  }

  ID3DBlob* pVertexShaderCode = nullptr;
  result = CompileAndCreateShader(L"../shaders/ParticleInstance.vs",
                                  (ID3D11DeviceChild**)&m_pVertexShader, {},
                                  &pVertexShaderCode);
  DX::ThrowIfFailed(result);

  result = CompileAndCreateShader(L"../shaders/ParticleInstance.ps",
                                  (ID3D11DeviceChild**)&m_pPixelShader);
  DX::ThrowIfFailed(result);

  result = m_pDXC->m_pDevice->CreateInputLayout(
      InputDesc, ARRAYSIZE(InputDesc), pVertexShaderCode->GetBufferPointer(),
      pVertexShaderCode->GetBufferSize(), &m_pInputLayout);
  DX::ThrowIfFailed(result);

  result = SetResourceName(m_pInputLayout, "ParticleInputLayout");
  DX::ThrowIfFailed(result);

  SAFE_RELEASE(pVertexShaderCode);

  return result;
}

HRESULT SPH::InitMarching() {
  Vector3 len = Vector3(m_props.cubeNum.x + 2, 2 * m_props.cubeNum.y,
                        m_props.cubeNum.z + 2) *
                m_props.cubeLen;

  float cubeWidth = m_props.h / 2;
  m_pMarchingCube =
      new MarchingCube(len, m_props.pos, cubeWidth, m_particles, m_props.h / 2);
  // create input layout
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT result = S_OK;

  UINT max_n = (len.x + len.y + len.z) / pow(cubeWidth, 3) * 16;
  m_pMarchingCube->march(m_vertex);

  // Create vertex buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(max_n * sizeof(Vector3));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    result =
        m_pDXC->m_pDevice->CreateBuffer(&desc, 0, &m_pMarchingVertexBuffer);
    DX::ThrowIfFailed(result);

    result = SetResourceName(m_pMarchingVertexBuffer, "MarchingVertexBuffer");
    DX::ThrowIfFailed(result);
  }

  ID3DBlob* pVertexShaderCode = nullptr;
  result = CompileAndCreateShader(L"../shaders/MarchingCubes.vs",
                                  (ID3D11DeviceChild**)&m_pMarchingVertexShader,
                                  {}, &pVertexShaderCode);
  DX::ThrowIfFailed(result);

  result = CompileAndCreateShader(L"../shaders/MarchingCubes.ps",
                                  (ID3D11DeviceChild**)&m_pMarchingPixelShader);
  DX::ThrowIfFailed(result);

  result = m_pDXC->m_pDevice->CreateInputLayout(
      InputDesc, ARRAYSIZE(InputDesc), pVertexShaderCode->GetBufferPointer(),
      pVertexShaderCode->GetBufferSize(), &m_pMarchingInputLayout);
  DX::ThrowIfFailed(result);

  result = SetResourceName(m_pMarchingInputLayout, "MarchingInputLayout");
  DX::ThrowIfFailed(result);

  SAFE_RELEASE(pVertexShaderCode);

  return result;
}

void SPH::UpdatePhysics(float dt) {
  n++;
  float& h = m_props.h;

  // Compute density
  for (auto& p : m_particles) {
    p.density = 0;
    for (auto& n : m_particles) {
      float d2 = Vector3::DistanceSquared(p.position, n.position);
      if (d2 < h2) {
        p.density += m_props.mass * poly6 * pow(h2 - d2, 3);
      }
    }
  }

  // UpdateDensity();

  // Compute pressure
  for (auto& p : m_particles) {
    float k = 1;
    float p0 = 1000;
    p.pressure = k * (p.density - p0);
  }

  // Compute pressure force
  for (auto& p : m_particles) {
    p.pressureGrad = Vector3::Zero;
    p.force = Vector4(0, -9.8f * p.density, 0, 0);
    p.viscosity = Vector4::Zero;
    for (auto& n : m_particles) {
      float d = Vector3::Distance(p.position, n.position);
      Vector3 dir = (p.position - n.position);
      dir.Normalize();
      if (d < h) {
        p.pressureGrad += dir * m_props.mass * (p.pressure + n.pressure) /
                          (2 * n.density) * spikyGrad * std::pow(h - d, 2);
        p.viscosity += m_props.dynamicViscosity * m_props.mass *
                       (n.velocity - p.velocity) / n.density * spikyLap *
                       (m_props.h - d);
      }
    }
  }
  // UpdateForces();

  // TimeStep
  for (auto& p : m_particles) {
    p.velocity +=
        dt * (Vector4(p.pressureGrad) + p.force + p.viscosity) / p.density;
    p.position += dt * Vector3(p.velocity);

    // boundary condition
    CheckBoundary(p);
  }
}

HRESULT SPH::UpdatePhysGPU(float dt) {
  HRESULT result = S_OK;

  m_sphDB.dt.x = dt;
  D3D11_MAPPED_SUBRESOURCE resource;
  m_pDXC->m_pDeviceContext->Map(m_pSphDB, 0, D3D11_MAP_WRITE_DISCARD, 0,
                                &resource);
  memcpy(resource.pData, &m_sphDB, sizeof(m_sphDB));
  m_pDXC->m_pDeviceContext->Unmap(m_pSphDB, 0);

  UINT groupNumber = DivUp(m_num_particles, 64u);

  ID3D11UnorderedAccessView* uavBuffers[1] = {m_pSphBufferUAV};
  ID3D11Buffer* cb[2] = {m_pSphCB, m_pSphDB};

  m_pDXC->m_pDeviceContext->CSSetConstantBuffers(0, 2, cb);
  m_pDXC->m_pDeviceContext->CSSetUnorderedAccessViews(0, 1, uavBuffers,
                                                      nullptr);

  m_pDXC->m_pDeviceContext->CSSetShader(m_pSPHComputeShader, nullptr, 0);
  m_pDXC->m_pDeviceContext->Dispatch(groupNumber, 1, 1);

  m_pDXC->m_pDeviceContext->CopyResource(m_pSphBuffer, m_pSphDataBuffer);

  return result;
}

void SPH::Update(float dt) {
  if (m_isCpu) {
    UpdatePhysics(dt);
  } else {
    UpdatePhysGPU(dt);
  }

  if (isMarching) {
    m_pMarchingCube->march(m_vertex);
    D3D11_MAPPED_SUBRESOURCE resource;
    HRESULT result = S_OK;
    result = m_pDXC->m_pDeviceContext->Map(
        m_pMarchingVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
    DX::ThrowIfFailed(result);
    memcpy(resource.pData, m_vertex.data(), sizeof(Vector3) * m_vertex.size());
    m_pDXC->m_pDeviceContext->Unmap(m_pMarchingVertexBuffer, 0);
  } else {
    for (int i = 0; i < m_particles.size(); i++) {
      m_instanceData[i].pos = m_particles[i].position;
      m_instanceData[i].density = m_particles[i].density;
    }

    m_pDXC->m_pDeviceContext->UpdateSubresource(m_pInstanceBuffer, 0, nullptr,
                                                m_instanceData.data(), 0, 0);
  }
}

void SPH::CheckBoundary(Particle& p) {
  float& h = m_props.h;
  float& dampingCoeff = m_props.dampingCoeff;
  Vector3& pos = m_props.pos;
  Vector3 len =
      Vector3(m_props.cubeNum.x, m_props.cubeNum.y, m_props.cubeNum.z) *
      m_props.cubeLen;

  if (p.position.y < h + pos.y) {
    p.position.y = -p.position.y + 2 * (h + pos.y);
    p.velocity.y = -p.velocity.y * dampingCoeff;
  }

  if (p.position.x < h + pos.x) {
    p.position.x = -p.position.x + 2 * (pos.x + h);
    p.velocity.x = -p.velocity.x * dampingCoeff;
  }

  if (p.position.x > -h + pos.x + len.x) {
    p.position.x = -p.position.x + 2 * (-h + pos.x + len.x);
    p.velocity.x = -p.velocity.x * dampingCoeff;
  }

  if (p.position.z < h + pos.z) {
    p.position.z = -p.position.z + 2 * (h + pos.z);
    p.velocity.z = -p.velocity.z * dampingCoeff;
  }

  if (p.position.z > -h + pos.z + len.z) {
    p.position.z = -p.position.z + 2 * (-h + pos.z + len.z);
    p.velocity.z = -p.velocity.z * dampingCoeff;
  }
}

void SPH::Render(ID3D11Buffer* pSceneBuffer) {
  if (isMarching)
    RenderMarching(pSceneBuffer);
  else
    RenderSpheres(pSceneBuffer);
}

void SPH::RenderMarching(ID3D11Buffer* pSceneBuffer) {
  auto& pContext = m_pDXC->m_pDeviceContext;
  pContext->OMSetDepthStencilState(m_pDXC->m_pTransDepthState, 0);
  pContext->OMSetBlendState(m_pDXC->m_pTransBlendState, nullptr, 0xffffffff);

  ID3D11Buffer* vertexBuffers[] = {m_pMarchingVertexBuffer};
  UINT strides[] = {sizeof(Vector3)};
  UINT offsets[] = {0};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

  pContext->IASetInputLayout(m_pMarchingInputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pMarchingVertexShader, nullptr, 0);
  pContext->PSSetShader(m_pMarchingPixelShader, nullptr, 0);
  ID3D11Buffer* cbuffers[] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);

  pContext->Draw(m_vertex.size(), 0);
}

void SPH::RenderSpheres(ID3D11Buffer* pSceneBuffer) {
  auto& pContext = m_pDXC->m_pDeviceContext;
  pContext->OMSetDepthStencilState(m_pDXC->m_pTransDepthState, 0);
  pContext->OMSetBlendState(m_pDXC->m_pTransBlendState, nullptr, 0xffffffff);

  pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

  ID3D11Buffer* vertexBuffers[] = {m_pVertexBuffer, m_pInstanceBuffer};
  UINT strides[] = {sizeof(Vector3), sizeof(InstanceData)};
  UINT offsets[] = {0, 0};
  pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);

  pContext->IASetInputLayout(m_pInputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader, nullptr, 0);
  pContext->PSSetShader(m_pPixelShader, nullptr, 0);

  ID3D11Buffer* cbuffers[2] = {pSceneBuffer, m_pSphBuffer};
  pContext->VSSetConstantBuffers(0, 2, cbuffers);

  pContext->PSSetConstantBuffers(0, 1, cbuffers);
  pContext->DrawIndexedInstanced(m_sphereIndexCount, m_particles.size(), 0, 0,
                                 0);
}