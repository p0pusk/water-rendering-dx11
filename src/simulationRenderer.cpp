#include "simulationRenderer.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <vector>

#include "SimpleMath.h"
#include "particle.h"

HRESULT SimRenderer::Init() {
  HRESULT result = S_OK;
  try {
    m_sphAlgo.Init(m_particles);
    result = InitSph();
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "InitSph() failed" << std::endl;
    exit(1);
  }
  try {
    result = InitMarching();
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "InitMarching() failed" << std::endl;
    exit(1);
  }
  try {
    result = InitSpheres();
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "InitSpheres() failed" << std::endl;
    exit(1);
  }

  assert(SUCCEEDED(result));

  return result;
}

HRESULT SimRenderer::InitSph() {
  HRESULT hr = S_OK;

  // create sph constant buffer
  try {
    m_sphCB.pos = m_settings.pos;
    m_sphCB.particleNum = m_num_particles;
    m_sphCB.cubeNum = m_settings.cubeNum;
    m_sphCB.cubeLen = m_settings.cubeLen;
    m_sphCB.h = m_settings.h;
    m_sphCB.mass = m_settings.mass;
    m_sphCB.dynamicViscosity = m_settings.dynamicViscosity;
    m_sphCB.dampingCoeff = m_settings.dampingCoeff;
    m_sphCB.marchingCubeWidth = m_settings.marchingCubeWidth;
    m_sphCB.dt.x = 1.f / 120.f;

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &m_sphCB;

    m_pDeviceResources->CreateConstantBuffer<SphCB>(&m_pSphCB, 0, &data,
                                                    "SphConstantBuffer");
  } catch (const std::exception& e) {
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
  } catch (std::exception& e) {
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
        TABLE_SIZE, D3D11_USAGE_DEFAULT, NULL, &data, "HashBuffer",
        &m_pHashBuffer);
    DX::ThrowIfFailed(hr);
  }

  // create Hash UAV
  {
    hr = m_pDeviceResources->CreateBufferUAV(
        m_pHashBuffer.Get(), TABLE_SIZE, (D3D11_BUFFER_UAV_FLAG)0,
        "HashBufferUAV", &m_pHashBufferUAV);
    DX::ThrowIfFailed(hr);
  }

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/SphClearTable.cs",
      (ID3D11DeviceChild**)m_pClearTableCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pClearTableCS.Get(), "ClearTableShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/SphCreateTable.cs",
      (ID3D11DeviceChild**)m_pTableCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pTableCS.Get(), "TableComputeShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/SphDensity.cs",
      (ID3D11DeviceChild**)m_pDensityCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pDensityCS.Get(), "DensityComputeShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/SphPressure.cs",
      (ID3D11DeviceChild**)m_pPressureCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pPressureCS.Get(), "PressureComputeShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/SphForces.cs",
      (ID3D11DeviceChild**)m_pForcesCS.GetAddressOf());
  DX::ThrowIfFailed(hr);
  hr = SetResourceName(m_pForcesCS.Get(), "ForcesComputeShader");
  DX::ThrowIfFailed(hr);

  hr = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/SphPositions.cs",
      (ID3D11DeviceChild**)m_pPositionsCS.GetAddressOf());
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

  for (auto& v : sphereVertices) {
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

  ID3DBlob* pVertexShaderCode = nullptr;
  result = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/ParticleInstance.vs", (ID3D11DeviceChild**)&m_pVertexShader,
      {}, &pVertexShaderCode);
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/ParticleInstance.ps", (ID3D11DeviceChild**)&m_pPixelShader);
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

  Vector3 len = Vector3(m_settings.cubeNum.x + 2, m_settings.cubeNum.y + 2,
                        m_settings.cubeNum.z + 2) *
                m_settings.cubeLen;

  UINT cubeNums =
      std::ceil(len.x * len.y * len.z / pow(m_settings.marchingCubeWidth, 3));
  UINT max_n = cubeNums * 16;

  // Create vertex buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(max_n * sizeof(Vector3));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pMarchingVertexBuffer);
    DX::ThrowIfFailed(result);

    result =
        SetResourceName(m_pMarchingVertexBuffer.Get(), "MarchingVertexBuffer");
    DX::ThrowIfFailed(result);
  }

  // create out buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth = max_n * sizeof(Vector3);
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(Vector3);

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pMarchingOutBuffer);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pMarchingOutBuffer.Get(), "MarchingOutBuffer");
    DX::ThrowIfFailed(result);
  }

  // create out buffer uav
  {
    result = m_pDeviceResources->CreateBufferUAV(
        m_pMarchingOutBuffer.Get(), max_n, D3D11_BUFFER_UAV_FLAG_COUNTER,
        "MarchingOutUAV", &m_pMarchingOutBufferUAV);
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
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pVoxelGridBuffer.Get(), "VoxelGridBuffer");
    DX::ThrowIfFailed(result);
  }

  // create VoxelGrid UAV
  {
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = cubeNums;
    desc.Buffer.Flags = 0;

    result = pDevice->CreateUnorderedAccessView(m_pVoxelGridBuffer.Get(), &desc,
                                                &m_pVoxelGridBufferUAV);
    DX::ThrowIfFailed(result);
    result = SetResourceName(m_pVoxelGridBufferUAV.Get(), "VoxelGridUAV");
    DX::ThrowIfFailed(result);
  }

  ID3DBlob* pVertexShaderCode = nullptr;
  result = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/MarchingCubes.vs",
      (ID3D11DeviceChild**)m_pMarchingVertexShader.GetAddressOf(), {},
      &pVertexShaderCode);
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pMarchingVertexShader.Get(),
                           "MarchingCubesVertexShader");
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/MarchingCubes.ps",
      (ID3D11DeviceChild**)m_pMarchingPixelShader.GetAddressOf());
  DX::ThrowIfFailed(result);
  result =
      SetResourceName(m_pMarchingPixelShader.Get(), "MarchingCubesPixelShader");
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/MarchingCubes.cs",
      (ID3D11DeviceChild**)m_pMarchingComputeShader.GetAddressOf());
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pMarchingPixelShader.Get(),
                           "MarchingCubesComputeShader");
  DX::ThrowIfFailed(result);

  result = m_pDeviceResources->CompileAndCreateShader(
      L"../shaders/MarchingPreprocess.cs",
      (ID3D11DeviceChild**)m_pClearBufferCS.GetAddressOf());
  DX::ThrowIfFailed(result);
  result = SetResourceName(m_pMarchingPixelShader.Get(),
                           "MarchingCubesPreprocessShader");
  DX::ThrowIfFailed(result);

  result = pDevice->CreateInputLayout(
      InputDesc, ARRAYSIZE(InputDesc), pVertexShaderCode->GetBufferPointer(),
      pVertexShaderCode->GetBufferSize(), &m_pMarchingInputLayout);
  DX::ThrowIfFailed(result);

  result = SetResourceName(m_pMarchingInputLayout.Get(), "MarchingInputLayout");
  DX::ThrowIfFailed(result);

  SAFE_RELEASE(pVertexShaderCode);

  return result;
}

HRESULT SimRenderer::UpdatePhysGPU() {
  HRESULT result = S_OK;
  auto pContext = m_pDeviceResources->GetDeviceContext();

  D3D11_MAPPED_SUBRESOURCE resource;
  result = pContext->Map(m_pSphDataBuffer.Get(), 0, D3D11_MAP_READ_WRITE, 0,
                         &resource);
  DX::ThrowIfFailed(result);
  Particle* particles = (Particle*)resource.pData;
  for (int i = 0; i < m_num_particles; i++) {
    particles[i].hash =
        m_sphAlgo.GetHash(m_sphAlgo.GetCell(particles[i].position));
  }

  std::sort(particles, particles + m_num_particles,
            [](Particle& a, Particle& b) { return a.hash < b.hash; });

  pContext->Unmap(m_pSphDataBuffer.Get(), 0);

  UINT groupNumber = DivUp(m_num_particles, m_settings.blockSize);

  ID3D11Buffer* cb[1] = {m_pSphCB.Get()};
  ID3D11UnorderedAccessView* uavBuffers[2] = {m_pSphBufferUAV.Get(),
                                              m_pHashBufferUAV.Get()};

  pContext->CSSetConstantBuffers(0, 1, cb);
  pContext->CSSetUnorderedAccessViews(0, 2, uavBuffers, nullptr);

  pContext->CSSetShader(m_pClearTableCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->CSSetShader(m_pTableCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->CSSetShader(m_pDensityCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->CSSetShader(m_pPressureCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->CSSetShader(m_pForcesCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);
  pContext->CSSetShader(m_pPositionsCS.Get(), nullptr, 0);
  pContext->Dispatch(groupNumber, 1, 1);

  ID3D11UnorderedAccessView* nullUavBuffers[1] = {NULL};
  pContext->CSSetUnorderedAccessViews(0, 1, nullUavBuffers, nullptr);

  return result;
}

void SimRenderer::Update(float dt) {
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
      Vector3 len = Vector3(m_settings.cubeNum.x + 2, m_settings.cubeNum.y + 2,
                            m_settings.cubeNum.z + 2) *
                    m_settings.cubeLen;

      UINT cubeNums =
          len.x * len.y * len.z / pow(m_settings.marchingCubeWidth, 3) + 0.5f;
      UINT max_n = cubeNums * 16;

      UINT groupNumber = DivUp(cubeNums, m_settings.blockSize);

      ID3D11Buffer* cb[1] = {m_pSphCB.Get()};
      ID3D11UnorderedAccessView* uavBuffers[3] = {
          m_pSphBufferUAV.Get(), m_pVoxelGridBufferUAV.Get(),
          m_pMarchingOutBufferUAV.Get()};

      pContext->CSSetConstantBuffers(0, 1, cb);

      const UINT pCounters1[3] = {-1, -1, -1};

      pContext->CSSetUnorderedAccessViews(0, 3, uavBuffers, pCounters1);
      pContext->CSSetShader(m_pClearBufferCS.Get(), nullptr, 0);
      pContext->Dispatch(groupNumber, 1, 1);

      const UINT pCounters2[3] = {0, 0, 0};
      pContext->CSSetUnorderedAccessViews(0, 3, uavBuffers, pCounters2);

      pContext->CSSetShader(m_pMarchingComputeShader.Get(), nullptr, 0);

      pContext->Dispatch(groupNumber, 1, 1);

      pContext->CopyResource(m_pMarchingVertexBuffer.Get(),
                             m_pMarchingOutBuffer.Get());
    }
  }
}

void SimRenderer::Render(ID3D11Buffer* pSceneBuffer) {
  if (m_settings.marching) {
    RenderMarching(pSceneBuffer);
  } else {
    RenderSpheres(pSceneBuffer);
  }
}

void SimRenderer::RenderMarching(ID3D11Buffer* pSceneBuffer) {
  auto pContext = m_pDeviceResources->GetDeviceContext();
  auto pTransDepthState = m_pDeviceResources->GetTransDepthState();
  auto pTransBlendState = m_pDeviceResources->GetTransBlendState();
  pContext->OMSetDepthStencilState(pTransDepthState, 0);
  pContext->OMSetBlendState(pTransBlendState, nullptr, 0xffffffff);

  ID3D11Buffer* vertexBuffers[] = {m_pMarchingVertexBuffer.Get()};
  UINT strides[] = {sizeof(Vector3)};
  UINT offsets[] = {0};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

  pContext->IASetInputLayout(m_pMarchingInputLayout.Get());
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pMarchingVertexShader.Get(), nullptr, 0);
  pContext->PSSetShader(m_pMarchingPixelShader.Get(), nullptr, 0);
  ID3D11Buffer* cbuffers[] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);

  Vector3 len = Vector3(m_settings.cubeNum.x + 2, m_settings.cubeNum.y + 2,
                        m_settings.cubeNum.z + 2) *
                m_settings.cubeLen;

  UINT cubeNums =
      std::ceil(len.x * len.y * len.z / pow(m_settings.marchingCubeWidth, 3));
  UINT max_n = cubeNums * 16;
  pContext->Draw(max_n, 0);
}

void SimRenderer::RenderSpheres(ID3D11Buffer* pSceneBuffer) {
  auto pContext = m_pDeviceResources->GetDeviceContext();
  pContext->OMSetDepthStencilState(m_pDeviceResources->GetTransDepthState(), 0);
  pContext->OMSetBlendState(m_pDeviceResources->GetTransBlendState(), nullptr,
                            0xffffffff);

  pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

  ID3D11Buffer* vertexBuffers[] = {m_pVertexBuffer};
  UINT strides[] = {sizeof(Vector3)};
  UINT offsets[] = {0, 0};
  pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

  pContext->IASetInputLayout(m_pInputLayout);
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->VSSetShader(m_pVertexShader, nullptr, 0);
  pContext->PSSetShader(m_pPixelShader, nullptr, 0);

  ID3D11Buffer* cbuffers[1] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  ID3D11ShaderResourceView* srvs[1] = {m_pSphBufferSRV.Get()};
  pContext->VSSetShaderResources(0, 1, srvs);

  pContext->PSSetConstantBuffers(0, 1, cbuffers);
  pContext->DrawIndexedInstanced(m_sphereIndexCount, m_particles.size(), 0, 0,
                                 0);
}
