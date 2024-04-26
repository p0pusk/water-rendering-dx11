#include "simulation/sph-gpu.h"
#include "device-resources.h"
#include "pch.h"
#include "utils.h"
#include <exception>
#include <execution>
#include <iostream>

void SphGpu::Init(const std::vector<Particle> &particles) {
  m_num_particles = particles.size();

  // create sph constant buffer
  try {
    m_sphCB.worldOffset = m_settings.worldOffset;
    m_sphCB.particleNum = m_num_particles;
    m_sphCB.boundaryLen = m_settings.boundaryLen;
    m_sphCB.h = m_settings.h;
    m_sphCB.mass = m_settings.mass;
    m_sphCB.dynamicViscosity = m_settings.dynamicViscosity;
    m_sphCB.dampingCoeff = m_settings.dampingCoeff;
    m_sphCB.marchingCubeWidth = m_settings.marchingCubeWidth;
    m_sphCB.hashTableSize = m_settings.TABLE_SIZE;
    m_sphCB.dt.x = m_settings.dt;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &m_sphCB;

    DX::CreateConstantBuffer<SphCB>(&m_pSphCB, 0, &data, "SphConstantBuffer");
  } catch (...) {
    throw;
  }

  try {
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = particles.data();
    data.SysMemPitch = particles.size() * sizeof(Particle);

    DX::CreateStructuredBuffer<Particle>(
        particles.size(), D3D11_USAGE_DEFAULT,
        D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, &data, "SphDataBuffer",
        &m_pSphDataBuffer);

    // create SPH UAV
    DX::CreateBufferUAV(m_pSphDataBuffer.Get(), m_num_particles,
                        "SphDataBufferUAV", &m_pSphBufferUAV);

    // create SPh SRV
    DX::CreateBufferSRV(m_pSphDataBuffer.Get(), m_num_particles,
                        "SphDataBufferSRV", &m_pSphBufferSRV);
  } catch (...) {
    throw;
  }

  try {
    // create Hash buffer
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = m_hash_table.data();

    DX::CreateStructuredBuffer<UINT>(m_settings.TABLE_SIZE, D3D11_USAGE_DEFAULT,
                                     0, &data, "HashBuffer", &m_pHashBuffer);

    // create Hash UAV
    DX::CreateBufferUAV(m_pHashBuffer.Get(), m_settings.TABLE_SIZE,
                        "HashBufferUAV", &m_pHashBufferUAV);

    // create Hash SRV
    DX::CreateBufferSRV(m_pHashBuffer.Get(), m_settings.TABLE_SIZE,
                        "HashBufferSRV", &m_pHashBufferSRV);
  } catch (...) {
    throw;
  }

  // shaders
  try {
    DX::CompileAndCreateShader(
        L"shaders/SphClearTable.cs",
        (ID3D11DeviceChild **)m_pClearTableCS.GetAddressOf());

    DX::CompileAndCreateShader(L"shaders/SphCreateTable.cs",
                               (ID3D11DeviceChild **)m_pTableCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/SphDensity.cs",
        (ID3D11DeviceChild **)m_pDensityCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/SphPressure.cs",
        (ID3D11DeviceChild **)m_pPressureCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/SphForces.cs",
        (ID3D11DeviceChild **)m_pForcesCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/SphPositions.cs",
        (ID3D11DeviceChild **)m_pPositionsCS.GetAddressOf());
  } catch (...) {
    throw;
  }

  try {
    CreateQueries();
  } catch (...) {
    std::cerr << "Failed to create queires!" << std::endl;
    throw;
  }
}

void SphGpu::CreateQueries() {
  auto pDevice = DX::DeviceResources::getInstance().m_pDevice;
  D3D11_QUERY_DESC desc{};
  desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  desc.MiscFlags = 0;
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryDisjoint[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryDisjoint[1]));
  desc.Query = D3D11_QUERY_TIMESTAMP;
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphStart[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphStart[1]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphClear[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphClear[1]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphCopy[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphCopy[1]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphHash[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphHash[1]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphDensity[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphDensity[1]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPressure[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPressure[1]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphForces[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphForces[1]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPosition[0]));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPosition[1]));
}

void SphGpu::Update() {
  auto pContext = DX::DeviceResources::getInstance().m_pDeviceContext;
  pContext->Begin(m_pQueryDisjoint[m_frameNum % 2].Get());

  pContext->End(m_pQuerySphStart[m_frameNum % 2].Get());
  D3D11_MAPPED_SUBRESOURCE resource;
  DX::ThrowIfFailed(pContext->Map(m_pSphDataBuffer.Get(), 0,
                                  D3D11_MAP_READ_WRITE, 0, &resource),
                    "SphDataBuffer Map");

  Particle *particles = (Particle *)resource.pData;
  for (int i = 0; i < m_num_particles; i++) {
    particles[i].hash = GetHash(GetCell(particles[i].position));
  }

  auto start = std::chrono::high_resolution_clock::now();
  std::sort(particles, particles + m_num_particles,
            [](Particle &a, Particle &b) { return a.hash < b.hash; });
  auto end = std::chrono::high_resolution_clock::now();
  m_sortTime =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  pContext->Unmap(m_pSphDataBuffer.Get(), 0);
  pContext->End(m_pQuerySphCopy[m_frameNum % 2].Get());

  UINT groupNumber = DivUp(m_num_particles, m_settings.blockSize);

  ID3D11Buffer *cb[1] = {m_pSphCB.Get()};
  ID3D11UnorderedAccessView *uavBuffers[2] = {m_pSphBufferUAV.Get(),
                                              m_pHashBufferUAV.Get()};

  pContext->CSSetConstantBuffers(0, 1, cb);
  pContext->CSSetUnorderedAccessViews(0, 2, uavBuffers, nullptr);

  pContext->CSSetShader(m_pClearTableCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphClear[m_frameNum % 2].Get());
  pContext->CSSetShader(m_pTableCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphHash[m_frameNum % 2].Get());
  pContext->CSSetShader(m_pDensityCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphDensity[m_frameNum % 2].Get());
  pContext->CSSetShader(m_pPressureCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphPressure[m_frameNum % 2].Get());
  pContext->CSSetShader(m_pForcesCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphForces[m_frameNum % 2].Get());
  pContext->CSSetShader(m_pPositionsCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphPosition[m_frameNum % 2].Get());

  ID3D11Buffer *nullBuffers[1] = {NULL};
  ID3D11UnorderedAccessView *nullUAVS[2] = {NULL, NULL};
  pContext->CSSetConstantBuffers(0, 1, nullBuffers);
  pContext->CSSetUnorderedAccessViews(0, 2, nullUAVS, nullptr);
  pContext->End(m_pQueryDisjoint[m_frameNum % 2].Get());
}

UINT SphGpu::GetHash(XMINT3 cell) {
  return ((cell.x * 73856093) ^ (cell.y * 19349663) ^ (cell.z * 83492791)) %
         m_settings.TABLE_SIZE;
}

XMINT3 SphGpu::GetCell(Vector3 position) {
  auto res = (position - m_settings.worldOffset) / m_settings.h;
  return XMINT3(res.x, res.y, res.z);
}
