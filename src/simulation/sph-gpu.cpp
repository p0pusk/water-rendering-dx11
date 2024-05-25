#include "sph-gpu.h"

#include "BufferHelpers.h"
#include "device-resources.h"
#include "pch.h"
#include "utils.h"
#include <exception>
#include <format>
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
    m_sphCB.diffuseNum = m_settings.diffuseParticlesNum;
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

    DX::CreateStructuredBuffer<Particle>(particles.size(), D3D11_USAGE_DEFAULT,
                                         0, &data, "SphDataBuffer",
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
    DX::CreateStructuredBuffer<SphStateBuffer>(
        1, D3D11_USAGE_DEFAULT, 0, nullptr, "SphStateBuffer", &m_pStateBuffer);

    DX::CreateBufferUAV(m_pStateBuffer.Get(), 1, "SphStateBufferUAV",
                        &m_pStateUAV);

    DX::CreateBufferSRV(m_pStateBuffer.Get(), 1, "SphDataBufferSRV",
                        &m_pStateSRV);
  } catch (...) {
    throw;
  }

  try {
    DX::CreateStructuredBuffer<Particle>(
        m_settings.diffuseParticlesNum, D3D11_USAGE_DEFAULT, 0, nullptr,
        "DiffuseParticlesBuffer", &m_pDiffuseBuffer);

    DX::CreateBufferUAV(m_pDiffuseBuffer.Get(), m_settings.diffuseParticlesNum,
                        "SphDataBufferUAV", &m_pDiffuseBufferUAV);

    DX::CreateBufferSRV(m_pDiffuseBuffer.Get(), m_settings.diffuseParticlesNum,
                        "SphDataBufferSRV", &m_pDiffuseBufferSRV);
  } catch (...) {
    throw;
  }

  try {
    // create Hash buffer
    DX::CreateStructuredBuffer<UINT>(m_settings.TABLE_SIZE + 1,
                                     D3D11_USAGE_DEFAULT, 0, nullptr,
                                     "HashBuffer", &m_pHashBuffer);

    // create Hash UAV
    DX::CreateBufferUAV(m_pHashBuffer.Get(), m_settings.TABLE_SIZE + 1,
                        "HashBufferUAV", &m_pHashBufferUAV);

    // create Hash SRV
    DX::CreateBufferSRV(m_pHashBuffer.Get(), m_settings.TABLE_SIZE + 1,
                        "HashBufferSRV", &m_pHashBufferSRV);
  } catch (...) {
    throw;
  }

  try {
    // create Scan buffer
    DX::CreateStructuredBuffer<UINT>(m_settings.TABLE_SIZE + 1,
                                     D3D11_USAGE_DEFAULT, 0, nullptr,
                                     "ScanHelperBuffer", &m_pScanHelperBuffer);

    // create Scan UAV
    DX::CreateBufferUAV(m_pScanHelperBuffer.Get(), m_settings.TABLE_SIZE + 1,
                        "ScanHelperBufferUAV", &m_pScanHelperBufferUAV);

    // create Scan SRV
    DX::CreateBufferSRV(m_pScanHelperBuffer.Get(), m_settings.TABLE_SIZE + 1,
                        "ScanHelperBufferSRV", &m_pScanHelperBufferSRV);
  } catch (...) {
    throw;
  }

  try {
    // create Entries buffer
    DX::CreateStructuredBuffer<UINT>(m_num_particles, D3D11_USAGE_DEFAULT, 0,
                                     nullptr, "EntriesBuffer",
                                     &m_pEntriesBuffer);

    // create Hash UAV
    DX::CreateBufferUAV(m_pEntriesBuffer.Get(), m_num_particles,
                        "EntriesBufferUAV", &m_pEntriesBufferUAV);

    // create Hash SRV
    DX::CreateBufferSRV(m_pEntriesBuffer.Get(), m_num_particles,
                        "EntriesBufferSRV", &m_pEntriesBufferSRV);
  } catch (...) {
    throw;
  }

  try {
  } catch (...) {
    throw;
  }

  // shaders
  try {
    DX::CompileAndCreateShader(
        L"shaders/neighbouring/ClearBuffers.cs",
        (ID3D11DeviceChild **)m_pClearTableCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/neighbouring/CreateHashBuffer.cs",
        (ID3D11DeviceChild **)m_pCreateHashCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/neighbouring/PrefixSum.cs",
        (ID3D11DeviceChild **)m_pPrefixSumCS.GetAddressOf());
    DX::CompileAndCreateShader(
        L"shaders/neighbouring/CreateEntriesBuffer.cs",
        (ID3D11DeviceChild **)m_pCreateEntriesCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/Density.cs",
        (ID3D11DeviceChild **)m_pDensityCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/Pressure.cs",
        (ID3D11DeviceChild **)m_pPressureCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/Forces.cs",
        (ID3D11DeviceChild **)m_pForcesCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/Positions.cs",
        (ID3D11DeviceChild **)m_pPositionsCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/SpawnDiffuse.cs",
        (ID3D11DeviceChild **)m_pSpawnDiffuseCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/Density.cs",
        (ID3D11DeviceChild **)m_pDiffuseDensityCS.GetAddressOf(), {"DIFFUSE"});

    DX::CompileAndCreateShader(
        L"shaders/sph/Pressure.cs",
        (ID3D11DeviceChild **)m_pDiffusePressureCS.GetAddressOf(), {"DIFFUSE"});

    DX::CompileAndCreateShader(
        L"shaders/sph/Forces.cs",
        (ID3D11DeviceChild **)m_pDiffuseForcesCS.GetAddressOf(), {"DIFFUSE"});

    DX::CompileAndCreateShader(
        L"shaders/sph/Positions.cs",
        (ID3D11DeviceChild **)m_pDiffusePositionsCS.GetAddressOf(),
        {"DIFFUSE"});
  } catch (...) {
    throw;
  }

  try {
    CreateQueries();
  } catch (...) {
    std::cerr << "Failed to create queires!" << std::endl;
    exit(1);
  }

  try {
    m_cScanCS.OnD3D11CreateDevice(
        DeviceResources::getInstance().m_pDevice.Get());
  } catch (...) {
    std::cerr << "Failed to create cScanCS!" << std::endl;
    throw;
  }
}

void SphGpu::CreateQueries() {
  auto pDevice = DeviceResources::getInstance().m_pDevice;
  D3D11_QUERY_DESC desc{};
  desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  desc.MiscFlags = 0;
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryDisjoint));
  desc.Query = D3D11_QUERY_TIMESTAMP;
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphStart));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphClear));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPrefix));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphHash));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphDensity));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPressure));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphForces));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPosition));
}

void SphGpu::Update() {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;
  pContext->Begin(m_pQueryDisjoint.Get());
  pContext->End(m_pQuerySphStart.Get());

  UINT groupNumber = DivUp(m_num_particles, m_settings.blockSize);

  {
    ID3D11Buffer *cb[1] = {m_pSphCB.Get()};
    ID3D11ShaderResourceView *srvs[1] = {m_pSphBufferSRV.Get()};
    ID3D11UnorderedAccessView *uavs[2] = {m_pHashBufferUAV.Get(),
                                          m_pEntriesBufferUAV.Get()};

    pContext->CSSetConstantBuffers(0, 1, cb);
    pContext->CSSetShaderResources(0, 1, srvs);
    pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

    pContext->CSSetShader(m_pClearTableCS.Get(), nullptr, 0);
    pContext->Dispatch(DivUp(m_settings.TABLE_SIZE + 1, m_settings.blockSize),
                       1, 1);

    pContext->CSSetShader(m_pCreateHashCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);
    pContext->End(m_pQuerySphClear.Get());
    ID3D11UnorderedAccessView *nuavs[2] = {nullptr, nullptr};
    pContext->CSSetUnorderedAccessViews(0, 1, nuavs, nullptr);
    ID3D11ShaderResourceView *nsrvs[1] = {nullptr};
    pContext->CSSetShaderResources(0, 1, nsrvs);

    DX::ThrowIfFailed(
        m_cScanCS.ScanCS(pContext.Get(), (INT)(m_settings.TABLE_SIZE + 1),
                         m_pHashBufferSRV.Get(), m_pHashBufferUAV.Get(),
                         m_pScanHelperBufferSRV.Get(),
                         m_pScanHelperBufferUAV.Get()),
        "Failed in scan");
    pContext->CSSetConstantBuffers(0, 1, cb);
    pContext->CSSetShaderResources(0, 1, srvs);
    pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
    pContext->End(m_pQuerySphPrefix.Get());

    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
    pContext->CSSetShader(m_pCreateEntriesCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);

    pContext->CSSetUnorderedAccessViews(0, 2, nuavs, nullptr);
    pContext->CSSetShaderResources(0, 1, nsrvs);
  }
  pContext->End(m_pQuerySphHash.Get());

  {
    ID3D11ShaderResourceView *srvs[2] = {m_pHashBufferSRV.Get(),
                                         m_pEntriesBufferSRV.Get()};
    ID3D11UnorderedAccessView *uavs[3] = {
        m_pSphBufferUAV.Get(), m_pDiffuseBufferUAV.Get(), m_pStateUAV.Get()};

    pContext->CSSetShaderResources(0, 2, srvs);
    pContext->CSSetUnorderedAccessViews(0, 3, uavs, nullptr);

    pContext->CSSetShader(m_pDensityCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);
    pContext->End(m_pQuerySphDensity.Get());
    pContext->CSSetShader(m_pPressureCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);
    pContext->End(m_pQuerySphPressure.Get());
    pContext->CSSetShader(m_pForcesCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);
    pContext->End(m_pQuerySphForces.Get());
    pContext->CSSetShader(m_pPositionsCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);
    pContext->End(m_pQuerySphPosition.Get());

    ID3D11ShaderResourceView *nsrvs[2] = {nullptr, nullptr};
    ID3D11UnorderedAccessView *nuavs[3] = {nullptr, nullptr, nullptr};
    pContext->CSSetShaderResources(0, 2, nsrvs);
    pContext->CSSetUnorderedAccessViews(0, 3, nuavs, nullptr);
  }

  // {
  //   ID3D11ShaderResourceView *srvs[1] = {m_pSphBufferSRV.Get()};
  //   ID3D11UnorderedAccessView *uavs[2] = {m_pDiffuseBufferUAV.Get(),
  //                                         m_pStateUAV.Get()};
  //
  //   pContext->CSSetShaderResources(0, 1, srvs);
  //   pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
  //
  //   pContext->CSSetShader(m_pSpawnDiffuseCS.Get(), nullptr, 0);
  //   pContext->Dispatch(groupNumber, 1, 1);
  // }
  //
  // {
  //   UINT num = DivUp(m_settings.diffuseParticlesNum, m_settings.blockSize);
  //   ID3D11ShaderResourceView *srvs[2] = {m_pHashBufferSRV.Get(),
  //                                        m_pEntriesBufferSRV.Get()};
  //   ID3D11UnorderedAccessView *uavs[3] = {
  //       m_pSphBufferUAV.Get(), m_pDiffuseBufferUAV.Get(), m_pStateUAV.Get()};
  //
  //   pContext->CSSetShaderResources(0, 2, srvs);
  //   pContext->CSSetUnorderedAccessViews(0, 3, uavs, nullptr);
  //
  //   pContext->CSSetShader(m_pDiffuseDensityCS.Get(), nullptr, 0);
  //   pContext->CSSetShader(m_pDiffusePressureCS.Get(), nullptr, 0);
  //   pContext->Dispatch(num, 1, 1);
  //   pContext->CSSetShader(m_pDiffuseForcesCS.Get(), nullptr, 0);
  //   pContext->Dispatch(num, 1, 1);
  //   pContext->CSSetShader(m_pDiffusePositionsCS.Get(), nullptr, 0);
  //   pContext->Dispatch(num, 1, 1);
  //
  //   ID3D11ShaderResourceView *nsrvs[2] = {nullptr, nullptr};
  //   ID3D11UnorderedAccessView *nuavs[3] = {nullptr, nullptr, nullptr};
  //   pContext->CSSetShaderResources(0, 2, nsrvs);
  //   pContext->CSSetUnorderedAccessViews(0, 3, nuavs, nullptr);
  // }
  pContext->End(m_pQueryDisjoint.Get());
}

void SphGpu::CollectTimestamps() {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;

  while (pContext->GetData(m_pQueryDisjoint.Get(), nullptr, 0, 0) == S_FALSE)
    ;
  // Check whether timestamps were disjoint during the last frame
  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
  pContext->GetData(m_pQueryDisjoint.Get(), &tsDisjoint,
                    sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);

  if (tsDisjoint.Disjoint) {
    return;
  }

  // Get all the timestamps
  UINT64 tsSphStart = 0, tsSphPrefix = 0, tsSphClear = 0, tsSphHash = 0,
         tsSphDensity = 0, tsSphPressure = 0, tsSphForces = 0,
         tsSphPositions = 0;

  DX::ThrowIfFailed(
      pContext->GetData(m_pQuerySphStart.Get(), &tsSphStart, sizeof(UINT64), 0),
      "Failed in GetData() SphStart");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphPrefix.Get(), &tsSphPrefix,
                                      sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(
      pContext->GetData(m_pQuerySphClear.Get(), &tsSphClear, sizeof(UINT64), 0),
      "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(
      pContext->GetData(m_pQuerySphHash.Get(), &tsSphHash, sizeof(UINT64), 0),
      "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphDensity.Get(), &tsSphDensity,
                                      sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphPressure.Get(), &tsSphPressure,
                                      sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphForces.Get(), &tsSphForces,
                                      sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphPosition.Get(),
                                      &tsSphPositions, sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");

  // Convert to real time
  m_sphClearTime =
      float(tsSphClear - tsSphStart) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphPrefixTime =
      float(tsSphPrefix - tsSphClear) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphCreateHashTime =
      float(tsSphHash - tsSphPrefix) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphDensityTime =
      float(tsSphDensity - tsSphHash) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphPressureTime = float(tsSphPressure - tsSphDensity) /
                      float(tsDisjoint.Frequency) * 1000.0f;
  m_sphForcesTime = float(tsSphForces - tsSphPressure) /
                    float(tsDisjoint.Frequency) * 1000.0f;
  m_sphPositionsTime = float(tsSphPositions - tsSphForces) /
                       float(tsDisjoint.Frequency) * 1000.0f;

  m_sphOverallTime = m_sphPrefixTime + m_sphClearTime + m_sphCreateHashTime +
                     m_sphDensityTime + m_sphPressureTime + m_sphForcesTime +
                     m_sphPositionsTime;
}

void SphGpu::ImGuiRender() {
  try {
    CollectTimestamps();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }

  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("Particles number: %d", m_num_particles);

  if (ImGui::CollapsingHeader("Time"), ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("Frame time %f ms", ImGui::GetIO().DeltaTime * 1000.f);
    ImGui::Text("Sph pass: %.3f ms", m_sphOverallTime);
  }
  if (ImGui::CollapsingHeader("SPH"), ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("Clear time: %.3f ms", m_sphClearTime);
    ImGui::Text("Prefix time: %f ms", m_sphPrefixTime);
    ImGui::Text("Hash time: %.3f ms", m_sphCreateHashTime);
    ImGui::Text("Density time: %.3f ms", m_sphDensityTime);
    ImGui::Text("Pressure time: %.3f ms", m_sphPressureTime);
    ImGui::Text("Forces time: %.3f ms", m_sphForcesTime);
    ImGui::Text("Positions time: %.3f ms", m_sphPositionsTime);
  }
  m_frameNum++;
}
