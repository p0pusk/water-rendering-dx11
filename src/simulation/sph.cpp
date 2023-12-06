#include "sph.h"

#include <stdint.h>

#include <algorithm>
#include <random>
#include <vector>

#include "SimpleMath.h"
#include "neighbour-hash.h"
#include "particle.h"

HRESULT SPH::Init() {
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
        p.mass = m_props.mass;
        p.velocity = Vector3::Zero;
      }
    }
  }

  // create hash table
  for (auto& p : m_particles) {
    p.hash = m_hashM.getHash(m_hashM.getCell(p, h, m_props.pos));
  }

  std::sort(
      m_particles.begin(), m_particles.end(),
      [&](const Particle& a, const Particle& b) { return a.hash < b.hash; });

  m_hashM.createTable(m_particles);

  HRESULT result = S_OK;
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

    D3D11_SUBRESOURCE_DATA data;
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
  Vector3 len = Vector3(m_props.cubeNum.x + 1, 2 * m_props.cubeNum.y + 1,
                        m_props.cubeNum.z + 1) *
                m_props.cubeLen;

  m_pMarchingCube =
      new MarchingCube(len, m_props.pos, m_props.h / 2, m_particles, m_props.h);
  // create input layout
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT result = S_OK;

  UINT max_n = 4096;
  m_pMarchingCube->march(m_vertex);

  // Create vertex buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(m_vertex.size() * sizeof(Vector3));
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = m_vertex.data();
    data.SysMemPitch = (UINT)(m_vertex.size() * sizeof(Vector3));
    data.SysMemSlicePitch = 0;

    result =
        m_pDXC->m_pDevice->CreateBuffer(&desc, &data, &m_pMarchingVertexBuffer);
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
        p.density += n.mass * poly6 * pow(h2 - d2, 3);
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
    p.force = Vector3(0, -9.8f * p.density, 0);
    p.viscosity = Vector3::Zero;
    for (auto& n : m_particles) {
      float d = Vector3::Distance(p.position, n.position);
      Vector3 dir = (p.position - n.position);
      dir.Normalize();
      if (d < h) {
        p.pressureGrad += dir * n.mass * (p.pressure + n.pressure) /
                          (2 * n.density) * spikyGrad * std::pow(h - d, 2);
        p.viscosity += m_props.dynamicViscosity * n.mass *
                       (n.velocity - p.velocity) / n.density * spikyLap *
                       (m_props.h - d);
      }
    }
  }
  // UpdateForces();

  // TimeStep
  for (auto& p : m_particles) {
    p.velocity += dt * (p.pressureGrad + p.force + p.viscosity) / p.density;
    p.position += dt * p.velocity;

    // boundary condition
    CheckBoundary(p);
  }
}

void SPH::Update(float dt) {
  UpdatePhysics(dt);

  if (isMarching) {
    m_pMarchingCube->march(m_vertex);
    m_pDXC->m_pDeviceContext->UpdateSubresource(m_pMarchingVertexBuffer, 0,
                                                nullptr, m_vertex.data(), 0, 0);
  } else {
    for (int i = 0; i < m_particles.size(); i++) {
      m_instanceData[i].pos = m_particles[i].position;
      m_instanceData[i].density = m_particles[i].density;
    }

    m_pDXC->m_pDeviceContext->UpdateSubresource(m_pInstanceBuffer, 0, nullptr,
                                                m_instanceData.data(), 0, 0);
  }
}

void SPH::UpdateDensity() {
  for (auto& p : m_particles) {
    p.density = 0;

    XMINT3 cell = m_hashM.getCell(p, m_props.h, m_props.pos);

    for (int x = -1; x <= 1; x++) {
      for (int y = -1; y <= 1; y++) {
        for (int z = -1; z <= 1; z++) {
          uint32_t cellHash =
              m_hashM.getHash({cell.x + x, cell.y + y, cell.z + z});
          uint32_t ni = m_hashM.m[cellHash];
          if (ni == NO_PARTICLE) {
            continue;
          }

          Particle* n = &m_particles[ni];
          while (n->hash == cellHash) {
            float d2 = Vector3::DistanceSquared(p.position, n->position);
            if (d2 < h2) {
              p.density += n->mass * poly6 * std::pow(h2 - d2, 3);
            }

            if (ni == m_particles.size() - 1) break;
            n = &m_particles[++ni];
          }
        }
      }
    }
  }
}

void SPH::UpdateForces() {
  for (auto& p : m_particles) {
    p.pressureGrad = Vector3::Zero;
    p.viscosity = Vector3::Zero;
    p.force = Vector3(0, -9.8 * p.density, 0);

    XMINT3 cell = m_hashM.getCell(p, m_props.h, m_props.pos);

    for (int x = -1; x <= 1; x++) {
      for (int y = -1; y <= 1; y++) {
        for (int z = -1; z <= 1; z++) {
          uint32_t cellHash =
              m_hashM.getHash({cell.x + x, cell.y + y, cell.z + z});

          uint32_t ni = m_hashM.m[cellHash];
          if (ni == NO_PARTICLE) {
            continue;
          }

          Particle* n = &m_particles[ni];
          while (n->hash == cellHash) {
            float d = Vector3::Distance(p.position, n->position);
            Vector3 dir = (p.position - n->position);
            dir.Normalize();

            if (d < m_props.h) {
              p.pressureGrad += dir * n->mass * (p.pressure + n->pressure) /
                                (2 * n->density) * spikyGrad *
                                std::pow(m_props.h - d, 2);

              p.viscosity += m_props.dynamicViscosity * n->mass *
                             (n->velocity - p.velocity) / n->density *
                             spikyLap * (m_props.h - d);
            }

            if (ni == m_particles.size() - 1) break;
            n = &m_particles[++ni];
          }
        }
      }
    }
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
  ID3D11Buffer* cbuffers[] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);
  pContext->DrawIndexedInstanced(m_sphereIndexCount, m_particles.size(), 0, 0,
                                 0);
}
