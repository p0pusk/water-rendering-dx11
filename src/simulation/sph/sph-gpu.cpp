#include "sph-gpu.h"

#include "BufferHelpers.h"
#include "device-resources.h"
#include "particle.h"
#include "pch.h"
#include "sph.h"
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
    m_sphCB.diffuseNum = m_settings.diffuseNum;
    m_sphCB.dt = m_settings.dt;
    m_sphCB.diffuseEnabled = m_settings.diffuseEnabled;
    m_sphCB.trappedAirThreshold = m_settings.trappedAirThreshold;
    m_sphCB.wavecrestThreshold = m_settings.wavecrestThreshold;
    m_sphCB.energyThreshold = m_settings.energyThreshold;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &m_sphCB;

    DX::CreateConstantBuffer<SphCB>(&m_pSphCB, 0, &data, "SphConstantBuffer");
    DX::CreateConstantBuffer<SortCB>(&m_pSortCB, 0, nullptr, "SortCB");
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
        1, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, nullptr,
        "SphStateBuffer", &m_pStateBuffer);

    DX::CreateBufferUAV(m_pStateBuffer.Get(), 1, "SphStateBufferUAV",
                        &m_pStateUAV);

    DX::CreateBufferSRV(m_pStateBuffer.Get(), 1, "SphDataBufferSRV",
                        &m_pStateSRV);
  } catch (...) {
    throw;
  }

  try {
    DX::CreateStructuredBuffer<DiffuseParticle>(
        m_settings.diffuseNum, D3D11_USAGE_DEFAULT, 0, nullptr,
        "DiffuseParticlesBuffer1", &m_pDiffuseBuffer1);

    DX::CreateBufferUAV(m_pDiffuseBuffer1.Get(), m_settings.diffuseNum,
                        "SphDataBufferUAV1", &m_pDiffuseBufferUAV1);

    DX::CreateBufferSRV(m_pDiffuseBuffer1.Get(), m_settings.diffuseNum,
                        "SphDataBufferSRV1", &m_pDiffuseBufferSRV1);
  } catch (...) {
    throw;
  }

  try {
    DX::CreateStructuredBuffer<DiffuseParticle>(
        m_settings.diffuseNum, D3D11_USAGE_DEFAULT, 0, nullptr,
        "DiffuseParticlesBuffer2", &m_pDiffuseBuffer2);

    DX::CreateBufferUAV(m_pDiffuseBuffer2.Get(), m_settings.diffuseNum,
                        "SphDataBufferUAV2", &m_pDiffuseBufferUAV2);

    DX::CreateBufferSRV(m_pDiffuseBuffer2.Get(), m_settings.diffuseNum,
                        "SphDataBufferSRV2", &m_pDiffuseBufferSRV2);
  } catch (...) {
    throw;
  }

  try {
    DX::CreateStructuredBuffer<Potential>(m_num_particles, D3D11_USAGE_DEFAULT,
                                          0, nullptr, "PotentialsBuffer",
                                          &m_pPotentialsBuffer);

    DX::CreateBufferUAV(m_pPotentialsBuffer.Get(), m_num_particles,
                        "PotentialsUAV", &m_pPotentialsUAV);

    DX::CreateBufferSRV(m_pPotentialsBuffer.Get(), m_num_particles,
                        "PotentialsSRV", &m_pPotentialsSRV);
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
    DX::CreateStructuredBuffer<Particle>(m_num_particles, D3D11_USAGE_DEFAULT,
                                         0, nullptr, "EntriesBuffer",
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
        L"shaders/sph/Forces.cs",
        (ID3D11DeviceChild **)m_pForcesCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/Positions.cs",
        (ID3D11DeviceChild **)m_pPositionsCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/Potentials.cs",
        (ID3D11DeviceChild **)m_pPotentialsCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/BitonicSort.cs",
        (ID3D11DeviceChild **)m_pBitonicSortCS.GetAddressOf(), {}, nullptr,
        "BitonicSort");

    DX::CompileAndCreateShader(
        L"shaders/BitonicSort.cs",
        (ID3D11DeviceChild **)m_pTransposeCS.GetAddressOf(), {}, nullptr,
        "MatrixTranspose");

    DX::CompileAndCreateShader(
        L"shaders/sph/SpawnDiffuse.cs",
        (ID3D11DeviceChild **)m_pSpawnDiffuseCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/AdvectDiffuse.cs",
        (ID3D11DeviceChild **)m_pAdvectDiffuseCS.GetAddressOf());

    DX::CompileAndCreateShader(
        L"shaders/sph/DeleteDiffuse.cs",
        (ID3D11DeviceChild **)m_pDeleteDiffuseCS.GetAddressOf());
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
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphEnd));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphClear));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPrefix));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphHash));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphDensity));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphForces));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQuerySphPosition));

  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryDiffuseStart));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryDiffusePotentials));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryDiffuseSpawn));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryDiffuseAdvect));
}

void SphGpu::Update() {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;
  pContext->Begin(m_pQueryDisjoint.Get());
  pContext->End(m_pQuerySphStart.Get());

  try {
    pContext->End(m_pQuerySphStart.Get());
    CreateHash();
    UpdateSPH();
    pContext->End(m_pQuerySphEnd.Get());
  } catch (...) {
    throw;
  }

  if (m_settings.diffuseEnabled) {
    try {
      pContext->End(m_pQueryDiffuseStart.Get());
      UpdateDiffuse();
    } catch (...) {
      throw;
    }
  }
  pContext->End(m_pQueryDisjoint.Get());
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
  ImGui::Text("Diffuse particles number: %d", m_settings.diffuseNum);

  if (ImGui::CollapsingHeader("Time"), ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("Frame time %f ms", ImGui::GetIO().DeltaTime * 1000.f);
    ImGui::Text("Sph pass: %.3f ms", m_sphOverallTime);
    ImGui::Text("Sph avg: %.3f ms", m_sphSum / m_frameNum);
    ImGui::Text("Diffuse pass: %.3f ms", m_diffusePotentialsTime +
                                             m_diffuseAdvectTime +
                                             m_diffusePotentialsTime);
    ImGui::Text("Diffuse avg: %.3f ms", m_diffuseSum / m_frameNum);
  }
  if (ImGui::CollapsingHeader("SPH"), ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("Clear time: %.3f ms", m_sphClearTime);
    ImGui::Text("Prefix time: %f ms", m_sphPrefixTime);
    ImGui::Text("Hash time: %.3f ms", m_sphCreateHashTime);
    ImGui::Text("Prefix overall time: %f ms",
                m_sphClearTime + m_sphPrefixTime + m_sphCreateHashTime);
    ImGui::Text("Density time: %.3f ms", m_sphDensityTime);
    ImGui::Text("Forces time: %.3f ms", m_sphForcesTime);
    ImGui::Text("Positions time: %.3f ms", m_sphPositionsTime);
  }

  if (m_settings.diffuseEnabled) {
    if (ImGui::CollapsingHeader("SPH"), ImGuiTreeNodeFlags_DefaultOpen) {
      ImGui::Text("Potentials time: %.3f ms", m_diffusePotentialsTime);
      ImGui::Text("Spawn time: %.3f ms", m_diffuseSpawnTime);
      ImGui::Text("Advect time: %.3f ms", m_diffuseAdvectTime);
    }
  }
  m_frameNum++;
}

void SphGpu::CreateHash() {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;
  ID3D11ShaderResourceView *srvs[1] = {m_pSphBufferSRV.Get()};
  ID3D11UnorderedAccessView *uavs[2] = {m_pHashBufferUAV.Get(),
                                        m_pEntriesBufferUAV.Get()};

  ID3D11UnorderedAccessView *nuavs[2] = {nullptr, nullptr};
  ID3D11ShaderResourceView *nsrvs[1] = {nullptr};

  UINT groupNumber =
      (m_num_particles + m_settings.blockSize) / m_settings.blockSize;
  {

    pContext->CSSetConstantBuffers(0, 1, m_pSphCB.GetAddressOf());
    pContext->CSSetShaderResources(0, 1, srvs);
    pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

    pContext->CSSetShader(m_pClearTableCS.Get(), nullptr, 0);
    pContext->Dispatch(DivUp(m_settings.TABLE_SIZE + 1, m_settings.blockSize),
                       1, 1);

    pContext->CSSetShader(m_pCreateHashCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);

    pContext->CSSetShaderResources(0, 1, nsrvs);
    pContext->CSSetUnorderedAccessViews(0, 1, nuavs, nullptr);
  }
  pContext->End(m_pQuerySphClear.Get());

  DX::ThrowIfFailed(
      m_cScanCS.ScanCS(pContext.Get(), (INT)(m_settings.TABLE_SIZE + 1),
                       m_pHashBufferSRV.Get(), m_pHashBufferUAV.Get(),
                       m_pScanHelperBufferSRV.Get(),
                       m_pScanHelperBufferUAV.Get()),
      "Failed in scan");
  pContext->End(m_pQuerySphPrefix.Get());

  {
    pContext->CSSetConstantBuffers(0, 1, m_pSphCB.GetAddressOf());
    pContext->CSSetShaderResources(0, 1, srvs);
    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    pContext->CSSetShader(m_pCreateEntriesCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);

    pContext->CSSetUnorderedAccessViews(0, 2, nuavs, nullptr);
    pContext->CSSetShaderResources(0, 1, nsrvs);
  }

  pContext->End(m_pQuerySphHash.Get());
}

void SphGpu::UpdateSPH() {
  try {

    auto pContext = DeviceResources::getInstance().m_pDeviceContext;

    UINT groupNumber = DivUp(m_num_particles, m_settings.blockSize);

    ID3D11ShaderResourceView *srvs[1] = {m_pHashBufferSRV.Get()};

    ID3D11UnorderedAccessView *uavs[2] = {m_pEntriesBufferUAV.Get(),
                                          m_pStateUAV.Get()};

    pContext->CSSetShaderResources(0, 1, srvs);
    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    pContext->CSSetShader(m_pDensityCS.Get(), nullptr, 0);
    pContext->Dispatch((27 * m_num_particles + m_settings.blockSize) /
                           m_settings.blockSize,
                       1, 1);
    pContext->End(m_pQuerySphDensity.Get());

    pContext->CSSetUnorderedAccessViews(1, 1, m_pPotentialsUAV.GetAddressOf(),
                                        nullptr);
    pContext->CSSetShader(m_pForcesCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);
    pContext->End(m_pQuerySphForces.Get());

    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
    pContext->CSSetShader(m_pPositionsCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);
    pContext->End(m_pQuerySphPosition.Get());

    ID3D11ShaderResourceView *nsrvs[1] = {nullptr};
    ID3D11UnorderedAccessView *nuavs[2] = {nullptr, nullptr};

    pContext->CSSetShaderResources(0, 1, nsrvs);
    pContext->CSSetUnorderedAccessViews(0, 2, nuavs, nullptr);

    pContext->CopyResource(m_pSphDataBuffer.Get(), m_pEntriesBuffer.Get());
  } catch (...) {
    throw;
  }
}

void SphGpu::UpdateDiffuse() {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;

  UINT groupNumber = DivUp(m_num_particles, m_settings.blockSize);
  try {
    ID3D11ShaderResourceView *srvs[2] = {m_pHashBufferSRV.Get(),
                                         m_pEntriesBufferSRV.Get()};
    ID3D11UnorderedAccessView *uavs[1] = {m_pPotentialsUAV.Get()};

    pContext->CSSetShaderResources(0, 2, srvs);
    pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
    pContext->CSSetShader(m_pPotentialsCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);

    ID3D11ShaderResourceView *nsrvs[3] = {nullptr, nullptr, nullptr};
    ID3D11UnorderedAccessView *nuavs[1] = {nullptr};
    pContext->CSSetShaderResources(0, 2, nsrvs);
    pContext->CSSetUnorderedAccessViews(0, 1, nuavs, nullptr);
  } catch (...) {
    throw;
  }

  pContext->End(m_pQueryDiffusePotentials.Get());

  try {
    ID3D11ShaderResourceView *srvs[2] = {m_pEntriesBufferSRV.Get(),
                                         m_pPotentialsSRV.Get()};
    ID3D11UnorderedAccessView *uavs[2] = {m_pDiffuseBufferUAV1.Get(),
                                          m_pStateUAV.Get()};
    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
    pContext->CSSetShaderResources(0, 2, srvs);
    pContext->CSSetShader(m_pSpawnDiffuseCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);

    ID3D11UnorderedAccessView *nuavs[2] = {nullptr, nullptr};
    ID3D11ShaderResourceView *nsrvs[2] = {nullptr, nullptr};
    pContext->CSSetShaderResources(0, 2, nsrvs);
    pContext->CSSetUnorderedAccessViews(0, 2, nuavs, nullptr);
  } catch (...) {
    throw;
  }

  pContext->End(m_pQueryDiffuseSpawn.Get());

  try {
    ID3D11ShaderResourceView *srvs[2] = {m_pHashBufferSRV.Get(),
                                         m_pSphBufferSRV.Get()};
    ID3D11UnorderedAccessView *uavs[2] = {m_pDiffuseBufferUAV1.Get(),
                                          m_pStateUAV.Get()};
    pContext->CSSetShader(m_pAdvectDiffuseCS.Get(), nullptr, 0);
    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
    pContext->CSSetShaderResources(0, 2, srvs);
    pContext->Dispatch(m_settings.diffuseNum / m_settings.blockSize, 1, 1);

    ID3D11ShaderResourceView *nsrvs[2] = {nullptr, nullptr};
    ID3D11UnorderedAccessView *nuavs[2] = {nullptr, nullptr};
    pContext->CSSetShaderResources(0, 2, nsrvs);
    pContext->CSSetUnorderedAccessViews(0, 2, nuavs, nullptr);
  } catch (...) {
    throw;
  }

  pContext->End(m_pQueryDiffuseAdvect.Get());

  try {
    ID3D11UnorderedAccessView *uavs[2] = {m_pDiffuseBufferUAV1.Get(),
                                          m_pStateUAV.Get()};
    pContext->CSSetShader(m_pDeleteDiffuseCS.Get(), nullptr, 0);
    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
    pContext->Dispatch(m_settings.diffuseNum / m_settings.blockSize, 1, 1);

    ID3D11UnorderedAccessView *nuavs[2] = {nullptr, nullptr};
    pContext->CSSetUnorderedAccessViews(0, 2, nuavs, nullptr);
  } catch (...) {
    throw;
  }
}

void SphGpu::DiffuseBitonicSort() {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;
  pContext->CSSetConstantBuffers(0, 1, m_pSortCB.GetAddressOf());

  const UINT NUM_ELEMENTS = m_settings.diffuseNum;
  const UINT MATRIX_WIDTH = BITONIC_BLOCK_SIZE;
  const UINT MATRIX_HEIGHT = NUM_ELEMENTS / BITONIC_BLOCK_SIZE;

  // Sort the data
  // First sort the rows for the levels <= to the block size
  for (UINT level = 2; level <= BITONIC_BLOCK_SIZE; level <<= 1) {
    SortCB constants = {level, level, MATRIX_HEIGHT, MATRIX_WIDTH};
    pContext->UpdateSubresource(m_pSortCB.Get(), 0, nullptr, &constants, 0, 0);

    // Sort the row data
    UINT UAVInitialCounts = 0;
    pContext->CSSetUnorderedAccessViews(
        0, 1, m_pDiffuseBufferUAV1.GetAddressOf(), &UAVInitialCounts);
    pContext->CSSetShader(m_pBitonicSortCS.Get(), nullptr, 0);
    pContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
  }

  // Then sort the rows and columns for the levels > than the block size
  // Transpose. Sort the Columns. Transpose. Sort the Rows.
  for (UINT level = (BITONIC_BLOCK_SIZE << 1); level <= NUM_ELEMENTS;
       level <<= 1) {
    SortCB constants1 = {(level / BITONIC_BLOCK_SIZE),
                         (level & ~NUM_ELEMENTS) / BITONIC_BLOCK_SIZE,
                         MATRIX_WIDTH, MATRIX_HEIGHT};
    pContext->UpdateSubresource(m_pSortCB.Get(), 0, nullptr, &constants1, 0, 0);

    // Transpose the data from buffer 1 into buffer 2
    ID3D11ShaderResourceView *pViewNULL = nullptr;
    UINT UAVInitialCounts = 0;
    pContext->CSSetShaderResources(0, 1, &pViewNULL);
    pContext->CSSetUnorderedAccessViews(
        0, 1, m_pDiffuseBufferUAV2.GetAddressOf(), &UAVInitialCounts);
    pContext->CSSetShaderResources(0, 1, m_pDiffuseBufferSRV1.GetAddressOf());
    pContext->CSSetShader(m_pTransposeCS.Get(), nullptr, 0);
    pContext->Dispatch(MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE,
                       MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE, 1);

    // Sort the transposed column data
    pContext->CSSetShader(m_pBitonicSortCS.Get(), nullptr, 0);
    pContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);

    SortCB constants2 = {BITONIC_BLOCK_SIZE, level, MATRIX_HEIGHT,
                         MATRIX_WIDTH};
    pContext->UpdateSubresource(m_pSortCB.Get(), 0, nullptr, &constants2, 0, 0);

    // Transpose the data from buffer 2 back into buffer 1
    pContext->CSSetShaderResources(0, 1, &pViewNULL);
    pContext->CSSetUnorderedAccessViews(
        0, 1, m_pDiffuseBufferUAV1.GetAddressOf(), &UAVInitialCounts);
    pContext->CSSetShaderResources(0, 1, m_pDiffuseBufferSRV2.GetAddressOf());
    pContext->CSSetShader(m_pTransposeCS.Get(), nullptr, 0);
    pContext->Dispatch(MATRIX_HEIGHT / TRANSPOSE_BLOCK_SIZE,
                       MATRIX_WIDTH / TRANSPOSE_BLOCK_SIZE, 1);

    // Sort the row data
    pContext->CSSetShader(m_pBitonicSortCS.Get(), nullptr, 0);
    pContext->Dispatch(NUM_ELEMENTS / BITONIC_BLOCK_SIZE, 1, 1);
  }
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
         tsSphDensity = 0, tsSphForces = 0, tsSphPositions = 0;

  DX::ThrowIfFailed(
      pContext->GetData(m_pQuerySphStart.Get(), &tsSphStart, sizeof(UINT64), 0),
      "Failed in GetData() SphStart");
  DX::ThrowIfFailed(
      pContext->GetData(m_pQuerySphClear.Get(), &tsSphClear, sizeof(UINT64), 0),
      "Failed in GetData Clear");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphPrefix.Get(), &tsSphPrefix,
                                      sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(
      pContext->GetData(m_pQuerySphHash.Get(), &tsSphHash, sizeof(UINT64), 0),
      "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphDensity.Get(), &tsSphDensity,
                                      sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphForces.Get(), &tsSphForces,
                                      sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");
  DX::ThrowIfFailed(pContext->GetData(m_pQuerySphPosition.Get(),
                                      &tsSphPositions, sizeof(UINT64), 0),
                    "Failed in GetData sphPrefix");

  UINT64 tsSphEnd;
  DX::ThrowIfFailed(
      pContext->GetData(m_pQuerySphEnd.Get(), &tsSphEnd, sizeof(UINT64), 0),
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
  m_sphForcesTime =
      float(tsSphForces - tsSphDensity) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphPositionsTime = float(tsSphPositions - tsSphForces) /
                       float(tsDisjoint.Frequency) * 1000.0f;

  m_sphOverallTime = m_sphPrefixTime + m_sphClearTime + m_sphCreateHashTime +
                     m_sphDensityTime + m_sphForcesTime + m_sphPositionsTime;
  m_sphSum += m_sphOverallTime;

  if (m_settings.diffuseEnabled) {

    UINT64 tsDiffuseStart, tsDiffuseSpawn, tsDiffusePotentials, tsDiffuseAdvect;

    DX::ThrowIfFailed(pContext->GetData(m_pQueryDiffuseStart.Get(),
                                        &tsDiffuseStart, sizeof(UINT64), 0),
                      "Failed in GetData sphPrefix");
    DX::ThrowIfFailed(pContext->GetData(m_pQueryDiffusePotentials.Get(),
                                        &tsDiffusePotentials, sizeof(UINT64),
                                        0));
    DX::ThrowIfFailed(pContext->GetData(m_pQueryDiffuseSpawn.Get(),
                                        &tsDiffuseSpawn, sizeof(UINT64), 0));
    DX::ThrowIfFailed(pContext->GetData(m_pQueryDiffuseAdvect.Get(),
                                        &tsDiffuseAdvect, sizeof(UINT64), 0));
    m_diffusePotentialsTime = float(tsDiffusePotentials - tsDiffuseStart) /
                              float(tsDisjoint.Frequency) * 1000.0f;
    m_diffuseSpawnTime = float(tsDiffuseSpawn - tsDiffusePotentials) /
                         float(tsDisjoint.Frequency) * 1000.0f;
    m_diffuseAdvectTime = float(tsDiffuseAdvect - tsDiffuseSpawn) /
                          float(tsDisjoint.Frequency) * 1000.0f;
    m_diffuseSum +=
        m_diffuseAdvectTime + m_diffuseSpawnTime + m_diffusePotentialsTime;
  }
}
