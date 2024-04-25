#include "simulationRenderer.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include "SimpleMath.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "particle.h"
#include "pch.h"

HRESULT SimRenderer::Init() {
  HRESULT result = S_OK;
  try {
    m_sphAlgo.Init(m_particles);
    result = InitSph();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "InitSph() failed" << std::endl;
    exit(1);
  }
  try {
    result = InitMarching();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "InitMarching() failed" << std::endl;
    exit(1);
  }
  try {
    result = InitSpheres();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "InitSpheres() failed" << std::endl;
    exit(1);
  }

  auto pDevice = m_pDeviceResources->GetDevice();
  D3D11_QUERY_DESC desc{};
  desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  desc.MiscFlags = 0;
  pDevice->CreateQuery(&desc, &m_pQueryDisjoint[0]);
  pDevice->CreateQuery(&desc, &m_pQueryDisjoint[1]);
  desc.Query = D3D11_QUERY_TIMESTAMP;
  pDevice->CreateQuery(&desc, &m_pQuerySphStart[0]);
  pDevice->CreateQuery(&desc, &m_pQuerySphStart[1]);
  pDevice->CreateQuery(&desc, &m_pQuerySphClear[0]);
  pDevice->CreateQuery(&desc, &m_pQuerySphClear[1]);
  pDevice->CreateQuery(&desc, &m_pQuerySphCopy[0]);
  pDevice->CreateQuery(&desc, &m_pQuerySphCopy[1]);
  pDevice->CreateQuery(&desc, &m_pQuerySphHash[0]);
  pDevice->CreateQuery(&desc, &m_pQuerySphHash[1]);
  pDevice->CreateQuery(&desc, &m_pQuerySphDensity[0]);
  pDevice->CreateQuery(&desc, &m_pQuerySphDensity[1]);
  pDevice->CreateQuery(&desc, &m_pQuerySphPressure[0]);
  pDevice->CreateQuery(&desc, &m_pQuerySphPressure[1]);
  pDevice->CreateQuery(&desc, &m_pQuerySphForces[0]);
  pDevice->CreateQuery(&desc, &m_pQuerySphForces[1]);
  pDevice->CreateQuery(&desc, &m_pQuerySphPosition[0]);
  pDevice->CreateQuery(&desc, &m_pQuerySphPosition[1]);
  pDevice->CreateQuery(&desc, &m_pQueryMarchingStart[0]);
  pDevice->CreateQuery(&desc, &m_pQueryMarchingStart[1]);
  pDevice->CreateQuery(&desc, &m_pQueryMarchingClear[0]);
  pDevice->CreateQuery(&desc, &m_pQueryMarchingClear[1]);
  pDevice->CreateQuery(&desc, &m_pQueryMarchingPreprocess[0]);
  pDevice->CreateQuery(&desc, &m_pQueryMarchingPreprocess[1]);
  pDevice->CreateQuery(&desc, &m_pQueryMarchingMain[0]);
  pDevice->CreateQuery(&desc, &m_pQueryMarchingMain[1]);

  assert(SUCCEEDED(result));

  return result;
}

HRESULT SimRenderer::InitSph() {
  HRESULT hr = S_OK;

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
    m_sphCB.dt.x = 1.f / 120.f;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &m_sphCB;

    m_pDeviceResources->CreateConstantBuffer<SphCB>(&m_pSphCB, 0, &data,
                                                    "SphConstantBuffer");
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    throw e;
  }

  try {
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = m_particles.data();
    data.SysMemPitch = m_particles.size() * sizeof(Particle);

    hr = m_pDeviceResources->CreateStructuredBuffer<Particle>(
        m_num_particles, D3D11_USAGE_DEFAULT,
        D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, &data, "SphDataBuffer",
        &m_pSphDataBuffer);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    throw e;
  }

  // create SPH UAV
  {
    hr = m_pDeviceResources->CreateBufferUAV(
        m_pSphDataBuffer.Get(), m_num_particles, (D3D11_BUFFER_UAV_FLAG)0,
        "SphDataBufferUAV", &m_pSphBufferUAV);
    DX::ThrowIfFailed(hr);
  }

  // create SPH SRV
  {
    hr = m_pDeviceResources->CreateBufferSRV(
        m_pSphDataBuffer.Get(), m_num_particles, "SphDataBufferSRV",
        &m_pSphBufferSRV);
    DX::ThrowIfFailed(hr);
  }

  // create Hash buffer
  {
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = m_hash_table.data();

    hr = m_pDeviceResources->CreateStructuredBuffer<UINT>(
        m_settings.TABLE_SIZE, D3D11_USAGE_DEFAULT, 0, &data, "HashBuffer",
        &m_pHashBuffer);
    DX::ThrowIfFailed(hr);
  }

  // create Hash UAV
  {
    hr = m_pDeviceResources->CreateBufferUAV(
        m_pHashBuffer.Get(), m_settings.TABLE_SIZE, (D3D11_BUFFER_UAV_FLAG)0,
        "HashBufferUAV", &m_pHashBufferUAV);
    DX::ThrowIfFailed(hr);
  }

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/SphClearTable.cs",
      (ID3D11DeviceChild **)m_pClearTableCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pClearTableCS.Get(), "ClearTableShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/SphCreateTable.cs",
      (ID3D11DeviceChild **)m_pTableCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pTableCS.Get(), "TableComputeShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/SphDensity.cs",
      (ID3D11DeviceChild **)m_pDensityCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pDensityCS.Get(), "DensityComputeShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/SphPressure.cs",
      (ID3D11DeviceChild **)m_pPressureCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pPressureCS.Get(), "PressureComputeShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/SphForces.cs",
      (ID3D11DeviceChild **)m_pForcesCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pForcesCS.Get(), "ForcesComputeShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/SphPositions.cs",
      (ID3D11DeviceChild **)m_pPositionsCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pPositionsCS.Get(), "PositionsComputeShader");
  DX::ThrowIfFailed(hr);

  return hr;
}

HRESULT SimRenderer::InitSpheres() {
  auto pDevice = m_pDeviceResources->GetDevice();
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

  for (auto &v : sphereVertices) {
    v = v * m_settings.particleRadius;
  }

  // create input layout
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT result = S_OK;

  // Create vertex buffer
  {
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = sphereVertices.data();
    data.SysMemPitch = (UINT)(sphereVertices.size() * sizeof(Vector3));
    data.SysMemSlicePitch = 0;

    result = m_pDeviceResources->CreateVertexBuffer<Vector3>(
        sphereVertices.size(), D3D11_USAGE_DEFAULT, 0, &data,
        "ParticleVertexBuffer", &m_pVertexBuffer);
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

    result = pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pIndexBuffer, "ParticleIndexBuffer");
    DX::ThrowIfFailed(result);
  }

  ID3DBlob *pVertexShaderCode = nullptr;
  result = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/ParticleInstance.vs", (ID3D11DeviceChild **)&m_pVertexShader,
      {}, &pVertexShaderCode);
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/ParticleInstance.ps", (ID3D11DeviceChild **)&m_pPixelShader);
  DX::ThrowIfFailed(result);

  result = pDevice->CreateInputLayout(
      InputDesc, ARRAYSIZE(InputDesc), pVertexShaderCode->GetBufferPointer(),
      pVertexShaderCode->GetBufferSize(), &m_pInputLayout);
  DX::ThrowIfFailed(result);

  result = SetResourceName(m_pInputLayout, "ParticleInputLayout");
  DX::ThrowIfFailed(result);

  SAFE_RELEASE(pVertexShaderCode);

  return result;
}

HRESULT SimRenderer::InitMarching() {
  auto pDevice = m_pDeviceResources->GetDevice();

  // create input layout
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
  };

  HRESULT result = S_OK;

  UINT cubeNums = std::ceil(
      m_settings.boundaryLen.x * m_settings.boundaryLen.y *
      m_settings.boundaryLen.z / pow(m_settings.marchingCubeWidth, 3));
  // UINT max_n = cubeNums * 5;
  UINT max_n = 125.f * 1024.f * 1024.f / (4 * 3);

  // Create vertex buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(max_n * sizeof(Vector3));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = sizeof(Vector3);

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pMarchingVertexBuffer);
    DX::ThrowIfFailed(result);

    result =
        SetResourceName(m_pMarchingVertexBuffer.Get(), "MarchingVertexBuffer");
    DX::ThrowIfFailed(result, "Failed to create vertex buffer");
  }

  // create out buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = max_n * sizeof(MarchingOutBuffer);
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(MarchingOutBuffer);

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pMarchingOutBuffer);
    DX::ThrowIfFailed(result, "Failed in out buffer");
    result = SetResourceName(m_pMarchingOutBuffer.Get(), "MarchingOutBuffer");
    DX::ThrowIfFailed(result);
  }

  // create out buffer uav
  {
    result = m_pDeviceResources->CreateBufferUAV(
        m_pMarchingOutBuffer.Get(), max_n, D3D11_BUFFER_UAV_FLAG_APPEND,
        "MarchingOutUAV", &m_pMarchingOutBufferUAV);
    DX::ThrowIfFailed(result, "Failed in UAV");
  }

  // create out buffer srv
  {
    result = m_pDeviceResources->CreateBufferSRV(m_pMarchingOutBuffer.Get(),
                                                 max_n, "MarchingOutSRV",
                                                 &m_pMarchingOutBufferSRV);
    DX::ThrowIfFailed(result, "Failed in SRV");
  }

  // create counter buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = sizeof(MarchingIndirectBuffer);
    desc.BindFlags = 0;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    desc.StructureByteStride = sizeof(MarchingIndirectBuffer);

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &m_MarchingIndirectBuffer;
    data.SysMemPitch = sizeof(MarchingIndirectBuffer);

    result = pDevice->CreateBuffer(&desc, &data, &m_pCountBuffer);
    DX::ThrowIfFailed(result, "Failed in indirect buffer");
    result = SetResourceName(m_pCountBuffer.Get(), "CountBuffer");
    DX::ThrowIfFailed(result);
  }

  // create Voxel Grid buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = cubeNums * sizeof(int);
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(int);

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pVoxelGridBuffer);
    DX::ThrowIfFailed(result, "Failed in voxel buffer");
    result = SetResourceName(m_pVoxelGridBuffer.Get(), "VoxelGridBuffer");
    DX::ThrowIfFailed(result);
  }

  // create VoxelGrid UAV
  {
    result = m_pDeviceResources->CreateBufferUAV(
        m_pVoxelGridBuffer.Get(), cubeNums, (D3D11_BUFFER_UAV_FLAG)0,
        "VoxelGridUAV", &m_pVoxelGridBufferUAV);
    DX::ThrowIfFailed(result, "Failed in voxel UAV");
  }

  ID3DBlob *pVertexShaderCode = nullptr;
  result = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/MarchingCubes.vs",
      (ID3D11DeviceChild **)m_pMarchingVertexShader.GetAddressOf(), {},
      &pVertexShaderCode);
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pMarchingVertexShader.Get(),
                           "MarchingCubesVertexShader");
  DX::ThrowIfFailed(result);

  ID3DBlob *pVertexShaderCodeIndirect = nullptr;
  result = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/MarchingCubesIndirect.vs",
      (ID3D11DeviceChild **)m_pMarchingIndirectVertexShader.GetAddressOf(), {},
      &pVertexShaderCodeIndirect);
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pMarchingIndirectVertexShader.Get(),
                           "MarchingCubesIndirectVertexShader");
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/MarchingCubes.ps",
      (ID3D11DeviceChild **)m_pMarchingPixelShader.GetAddressOf());
  DX::ThrowIfFailed(result);
  result =
      SetResourceName(m_pMarchingPixelShader.Get(), "MarchingCubesPixelShader");
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/MarchingCubes.cs",
      (ID3D11DeviceChild **)m_pMarchingComputeShader.GetAddressOf());
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pMarchingComputeShader.Get(),
                           "MarchingCubesComputeShader");
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/MarchingPreprocess.cs",
      (ID3D11DeviceChild **)m_pMarchingPreprocessCS.GetAddressOf());
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pMarchingPreprocessCS.Get(),
                           "MarchingCubesPreprocessShader");
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"shaders/MarchingClearVoxel.cs",
      (ID3D11DeviceChild **)m_pMarchingClearCS.GetAddressOf());
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pMarchingClearCS.Get(), "MarchingCubesClearCS");
  DX::ThrowIfFailed(result);

  result = pDevice->CreateInputLayout(
      InputDesc, ARRAYSIZE(InputDesc), pVertexShaderCode->GetBufferPointer(),
      pVertexShaderCode->GetBufferSize(), &m_pMarchingInputLayout);
  DX::ThrowIfFailed(result);

  result = SetResourceName(m_pMarchingInputLayout.Get(), "MarchingInputLayout");
  DX::ThrowIfFailed(result);

  SAFE_RELEASE(pVertexShaderCode);
  SAFE_RELEASE(pVertexShaderCodeIndirect);

  return result;
}

HRESULT SimRenderer::UpdatePhysGPU() {
  HRESULT result = S_OK;
  auto pContext = m_pDeviceResources->GetDeviceContext();

  pContext->End(m_pQuerySphStart[m_frameNum % 2]);
  D3D11_MAPPED_SUBRESOURCE resource;
  result = pContext->Map(m_pSphDataBuffer.Get(), 0, D3D11_MAP_READ_WRITE, 0,
                         &resource);
  DX::ThrowIfFailed(result);
  Particle *particles = (Particle *)resource.pData;
  for (int i = 0; i < m_num_particles; i++) {
    particles[i].hash =
        m_sphAlgo.GetHash(m_sphAlgo.GetCell(particles[i].position));
  }

  auto start = std::chrono::high_resolution_clock::now();
  std::sort(particles, particles + m_num_particles,
            [](Particle &a, Particle &b) { return a.hash < b.hash; });
  auto end = std::chrono::high_resolution_clock::now();
  m_sortTime =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  pContext->Unmap(m_pSphDataBuffer.Get(), 0);
  pContext->End(m_pQuerySphCopy[m_frameNum % 2]);

  UINT groupNumber = DivUp(m_num_particles, m_settings.blockSize);

  ID3D11Buffer *cb[1] = {m_pSphCB.Get()};
  ID3D11UnorderedAccessView *uavBuffers[2] = {m_pSphBufferUAV.Get(),
                                              m_pHashBufferUAV.Get()};

  pContext->CSSetConstantBuffers(0, 1, cb);
  pContext->CSSetUnorderedAccessViews(0, 2, uavBuffers, nullptr);

  pContext->CSSetShader(m_pClearTableCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphClear[m_frameNum % 2]);
  pContext->CSSetShader(m_pTableCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphHash[m_frameNum % 2]);
  pContext->CSSetShader(m_pDensityCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphDensity[m_frameNum % 2]);
  pContext->CSSetShader(m_pPressureCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphPressure[m_frameNum % 2]);
  pContext->CSSetShader(m_pForcesCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphForces[m_frameNum % 2]);
  pContext->CSSetShader(m_pPositionsCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->End(m_pQuerySphPosition[m_frameNum % 2]);

  ID3D11UnorderedAccessView *nullUavBuffers[2] = {NULL, NULL};
  pContext->CSSetUnorderedAccessViews(0, 2, nullUavBuffers, nullptr);

  return result;
}

void SimRenderer::Update(float dt) {
  m_pDeviceResources->GetDeviceContext()->Begin(
      m_pQueryDisjoint[m_frameNum % 2]);
  auto pContext = m_pDeviceResources->GetDeviceContext();
  if (m_settings.cpu) {
    m_sphAlgo.Update(dt, m_particles);
    if (m_settings.marching) {
      m_marchingCubesAlgo.march(m_vertex);
      HRESULT result = S_OK;
      D3D11_MAPPED_SUBRESOURCE resource;
      result = pContext->Map(m_pMarchingVertexBuffer.Get(), 0,
                             D3D11_MAP_WRITE_DISCARD, 0, &resource);
      DX::ThrowIfFailed(result);
      memcpy(resource.pData, m_vertex.data(),
             sizeof(Vector3) * m_vertex.size());
      pContext->Unmap(m_pMarchingVertexBuffer.Get(), 0);
    } else {
      pContext->UpdateSubresource(m_pSphDataBuffer.Get(), 0, nullptr,
                                  m_particles.data(), 0, 0);
    }
  } else {
    UpdatePhysGPU();
    if (m_settings.marching) {
      UINT cubeNums = m_settings.boundaryLen.x * m_settings.boundaryLen.y *
                          m_settings.boundaryLen.z /
                          pow(m_settings.marchingCubeWidth, 3) +
                      0.5f;

      UINT groupNumber = DivUp(cubeNums, m_settings.blockSize);

      ID3D11Buffer *cb[1] = {m_pSphCB.Get()};
      pContext->CSSetConstantBuffers(0, 1, cb);

      ID3D11UnorderedAccessView *uavBuffers1[1] = {m_pVoxelGridBufferUAV.Get()};
      pContext->CSSetUnorderedAccessViews(0, 1, uavBuffers1, nullptr);

      pContext->End(m_pQueryMarchingStart[m_frameNum % 2]);
      pContext->CSSetShader(m_pMarchingClearCS.Get(), nullptr, 0);
      pContext->Dispatch(groupNumber, 1, 1);
      pContext->End(m_pQueryMarchingClear[m_frameNum % 2]);

      ID3D11ShaderResourceView *srvBuffers[1] = {m_pSphBufferSRV.Get()};
      pContext->CSSetShaderResources(0, 1, srvBuffers);

      pContext->CSSetShader(m_pMarchingPreprocessCS.Get(), nullptr, 0);
      // groupNumber = DivUp(m_num_particles, m_settings.blockSize);
      pContext->Dispatch(groupNumber, 1, 1);
      pContext->End(m_pQueryMarchingPreprocess[m_frameNum % 2]);

      const UINT pCounters2[2] = {0, 0};
      ID3D11UnorderedAccessView *uavBuffers2[2] = {
          m_pVoxelGridBufferUAV.Get(), m_pMarchingOutBufferUAV.Get()};

      pContext->CSSetUnorderedAccessViews(0, 2, uavBuffers2, pCounters2);

      groupNumber = DivUp(cubeNums, m_settings.blockSize);
      pContext->CSSetShader(m_pMarchingComputeShader.Get(), nullptr, 0);

      pContext->Dispatch(groupNumber, 1, 1);
      pContext->End(m_pQueryMarchingMain[m_frameNum % 2]);

      ID3D11UnorderedAccessView *nullUavBuffers[2] = {NULL, NULL};
      ID3D11ShaderResourceView *nullSrvs[1] = {NULL};
      pContext->CSSetUnorderedAccessViews(0, 2, nullUavBuffers, nullptr);
      pContext->CSSetShaderResources(0, 1, nullSrvs);

      pContext->CopyStructureCount(m_pCountBuffer.Get(), sizeof(UINT),
                                   m_pMarchingOutBufferUAV.Get());
    }
  }
  m_pDeviceResources->GetDeviceContext()->End(m_pQueryDisjoint[m_frameNum % 2]);
}

void SimRenderer::ImGuiRender() {
  ImGui::Begin("Simulation");
  CollectTimestamps();
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::Text("Particles number: %d", m_num_particles);

  if (ImGui::CollapsingHeader("Time"), ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("Frame time %f ms", ImGui::GetIO().DeltaTime * 1000.f);
    ImGui::Text("Sph pass: %.3f ms", m_sphOverallTime);
  }
  if (ImGui::CollapsingHeader("SPH"), ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("Copy time: %f ms", m_sphCopyTime);
    ImGui::Text("Sort time: %.3f ms", m_sortTime / 1000000.f);
    ImGui::Text("Clear time: %.3f ms", m_sphClearTime);
    ImGui::Text("Hash time: %.3f ms", m_sphCreateHashTime);
    ImGui::Text("Density time: %.3f ms", m_sphDensityTime);
    ImGui::Text("Pressure time: %.3f ms", m_sphPressureTime);
    ImGui::Text("Forces time: %.3f ms", m_sphForcesTime);
    ImGui::Text("Positions time: %.3f ms", m_sphPositionsTime);
  }
  if (m_settings.marching) {
    if (ImGui::CollapsingHeader("Marching Cubes"),
        ImGuiTreeNodeFlags_DefaultOpen) {
      ImGui::Text("Marching cube width: %0.2f * radius",
                  m_settings.marchingCubeWidth / m_settings.h);
      ImGui::Text("Marching Clear: %.3f ms", m_marchingClear);
      ImGui::Text("Marching Preprocess: %.3f ms", m_marchingPrep);
      ImGui::Text("Marching Main: %.3f ms", m_marchingMain);
    }
  }
  ImGui::End();
}

void SimRenderer::Render(ID3D11Buffer *pSceneBuffer) {
  if (m_settings.marching) {
    RenderMarching(pSceneBuffer);
  } else {
    RenderSpheres(pSceneBuffer);
  }
  ImGuiRender();
  m_frameNum++;
}

void SimRenderer::RenderMarching(ID3D11Buffer *pSceneBuffer) {
  auto pContext = m_pDeviceResources->GetDeviceContext();
  auto pTransDepthState = m_pDeviceResources->GetTransDepthState();
  auto pTransBlendState = m_pDeviceResources->GetTransBlendState();
  pContext->OMSetDepthStencilState(pTransDepthState, 0);
  pContext->OMSetBlendState(pTransBlendState, nullptr, 0xffffffff);

  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->PSSetShader(m_pMarchingPixelShader.Get(), nullptr, 0);
  ID3D11Buffer *cbuffers[] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);

  if (m_settings.cpu) {
    ID3D11Buffer *vertexBuffers[] = {m_pMarchingVertexBuffer.Get()};
    UINT strides[] = {sizeof(Vector3)};
    UINT offsets[] = {0};
    pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pContext->IASetInputLayout(m_pMarchingInputLayout.Get());
    pContext->VSSetShader(m_pMarchingVertexShader.Get(), nullptr, 0);
    pContext->Draw(m_vertex.size(), 0);
  } else {
    pContext->IASetInputLayout(nullptr);
    pContext->VSSetShader(m_pMarchingIndirectVertexShader.Get(), nullptr, 0);
    ID3D11ShaderResourceView *srvs = {m_pMarchingOutBufferSRV.Get()};
    pContext->VSSetShaderResources(0, 1, &srvs);
    pContext->DrawInstancedIndirect(m_pCountBuffer.Get(), 0);
    ID3D11ShaderResourceView *nullSRV[1] = {NULL};
    pContext->VSSetShaderResources(0, 1, nullSRV);
  }
}

void SimRenderer::RenderSpheres(ID3D11Buffer *pSceneBuffer) {
  auto pContext = m_pDeviceResources->GetDeviceContext();
  pContext->OMSetDepthStencilState(m_pDeviceResources->GetDepthState(), 0);
  pContext->OMSetBlendState(m_pDeviceResources->GetOpaqueBlendState(), nullptr,
                            0xffffffff);

  pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer};
  UINT strides[] = {sizeof(Vector3)};
  UINT offsets[] = {0, 0};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

  pContext->IASetInputLayout(m_pInputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader, nullptr, 0);
  pContext->PSSetShader(m_pPixelShader, nullptr, 0);

  ID3D11Buffer *cbuffers[1] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  ID3D11ShaderResourceView *srvs[1] = {m_pSphBufferSRV.Get()};
  pContext->VSSetShaderResources(0, 1, srvs);

  cbuffers[0] = m_pSphCB.Get();
  pContext->PSSetShaderResources(0, 1, srvs);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);
  pContext->DrawIndexedInstanced(m_sphereIndexCount, m_particles.size(), 0, 0,
                                 0);
}

void SimRenderer::CollectTimestamps() {
  auto pContext = m_pDeviceResources->GetDeviceContext();

  // Check whether timestamps were disjoint during the last frame
  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
  pContext->GetData(m_pQueryDisjoint[m_frameNum % 2 + 1], &tsDisjoint,
                    sizeof(tsDisjoint), 0);

  if (tsDisjoint.Disjoint) {
    return;
  }

  // Get all the timestamps
  UINT64 tsMarchingBegin, tsMarchingClear, tsMarchingPreproc, tsMarchingMain,
      tsSphStart, tsSphClear, tsSphHash, tsSphDensity, tsSphPressure,
      tsSphForces, tsSphPosition, tsSphCopy;

  pContext->GetData(m_pQuerySphStart[m_frameNum % 2 + 1], &tsSphStart,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQuerySphCopy[m_frameNum % 2 + 1], &tsSphCopy,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQuerySphClear[m_frameNum % 2 + 1], &tsSphClear,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQuerySphHash[m_frameNum % 2 + 1], &tsSphHash,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQuerySphDensity[m_frameNum % 2 + 1], &tsSphDensity,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQuerySphPressure[m_frameNum % 2 + 1], &tsSphPressure,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQuerySphForces[m_frameNum % 2 + 1], &tsSphForces,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQuerySphPosition[m_frameNum % 2 + 1], &tsSphPosition,
                    sizeof(UINT64), 0);

  pContext->GetData(m_pQueryMarchingStart[m_frameNum % 2 + 1], &tsMarchingBegin,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQueryMarchingClear[m_frameNum % 2 + 1], &tsMarchingClear,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQueryMarchingPreprocess[m_frameNum % 2 + 1],
                    &tsMarchingPreproc, sizeof(UINT64), 0);
  pContext->GetData(m_pQueryMarchingMain[m_frameNum % 2 + 1], &tsMarchingMain,
                    sizeof(UINT64), 0);

  // Convert to real time
  m_sphCopyTime =
      float(tsSphCopy - tsSphStart) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphClearTime =
      float(tsSphClear - tsSphCopy) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphCreateHashTime =
      float(tsSphHash - tsSphClear) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphDensityTime =
      float(tsSphDensity - tsSphHash) / float(tsDisjoint.Frequency) * 1000.0f;
  m_sphPressureTime = float(tsSphPressure - tsSphDensity) /
                      float(tsDisjoint.Frequency) * 1000.0f;
  m_sphForcesTime = float(tsSphForces - tsSphPressure) /
                    float(tsDisjoint.Frequency) * 1000.0f;
  m_sphPositionsTime = float(tsSphPosition - tsSphForces) /
                       float(tsDisjoint.Frequency) * 1000.0f;
  m_sphOverallTime =
      float(tsSphPosition - tsSphStart) / float(tsDisjoint.Frequency) * 1000.0f;
  m_marchingClear = float(tsMarchingClear - tsMarchingBegin) /
                    float(tsDisjoint.Frequency) * 1000.0f;
  m_marchingPrep = float(tsMarchingPreproc - tsMarchingClear) /
                   float(tsDisjoint.Frequency) * 1000.0f;
  m_marchingMain = float(tsMarchingMain - tsMarchingPreproc) /
                   float(tsDisjoint.Frequency) * 1000.0f;
}
