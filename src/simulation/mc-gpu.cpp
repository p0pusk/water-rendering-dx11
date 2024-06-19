#include "mc-gpu.h"
#include "device-resources.h"
#include "sph-gpu.h"
#include "utils.h"

void MCGpu::Init() {
  auto pDevice = DeviceResources::getInstance().m_pDevice;
  m_states = std::make_unique<CommonStates>(pDevice.Get());
  m_fxFactory = std::make_unique<EffectFactory>(pDevice.Get());

  HRESULT result = S_OK;

  UINT cubeNums = (m_settings.marchingResolution.x + 1) *
                  (m_settings.marchingResolution.y + 1) *
                  (m_settings.marchingResolution.z + 1);

  UINT max_n = 2147483648;

  // create out buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.ByteWidth =
        max_n / sizeof(MarchingOutBuffer) * sizeof(MarchingOutBuffer);
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(MarchingOutBuffer);

    result = pDevice->CreateBuffer(&desc, nullptr, &m_pMarchingOutBuffer);
    DX::ThrowIfFailed(result, "Failed in out buffer");
    result =
        DX::SetResourceName(m_pMarchingOutBuffer.Get(), "MarchingOutBuffer");
    DX::ThrowIfFailed(result);
  }

  // create out buffer uav
  result = DX::CreateBufferUAV(
      m_pMarchingOutBuffer.Get(), max_n / sizeof(MarchingOutBuffer),
      "MarchingOutUAV", &m_pMarchingOutBufferUAV, D3D11_BUFFER_UAV_FLAG_APPEND);

  // create out buffer srv
  result = DX::CreateBufferSRV(m_pMarchingOutBuffer.Get(),
                               max_n / sizeof(MarchingOutBuffer),
                               "MarchingOutSRV", &m_pMarchingOutBufferSRV);

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

    result = pDevice->CreateBuffer(&desc, &data, &m_pIndirectBuffer);
    DX::ThrowIfFailed(result, "Failed in indirect buffer");
    result = DX::SetResourceName(m_pIndirectBuffer.Get(), "CountBuffer");
    DX::ThrowIfFailed(result);
  }

  // create Voxel Grid buffer
  {
    DX::ThrowIfFailed(DX::CreateStructuredBuffer<float>(
        cubeNums, D3D11_USAGE_DEFAULT, 0, nullptr, "VoxelGrid",
        &m_pVoxelGridBuffer1));
    DX::ThrowIfFailed(DX::CreateBufferUAV(m_pVoxelGridBuffer1.Get(), cubeNums,
                                          "VoxelGridUAV",
                                          &m_pVoxelGridBufferUAV1));
  }

  // create VoxelGrid for filters
  {
    DX::ThrowIfFailed(DX::CreateStructuredBuffer<float>(
        cubeNums, D3D11_USAGE_DEFAULT, 0, nullptr, "FilteredGrid",
        &m_pVoxelGridBuffer2));

    DX::ThrowIfFailed(DX::CreateBufferUAV(m_pVoxelGridBuffer2.Get(), cubeNums,
                                          "FilteredGridUAV",
                                          &m_pVoxelGridBufferUAV2));
  }

  // create surface buffer
  {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(CountBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(CountBuffer);

    DX::ThrowIfFailed(pDevice->CreateBuffer(&desc, nullptr, &m_pCountBuffer),
                      "Failed to create surfaceBuffer");

    DX::ThrowIfFailed(
        DX::SetResourceName(m_pCountBuffer.Get(), "SurfaceBuffer"),
        "Failed to set SurfaceBuffer resource name");
  }

  // create surface buffer UAV
  {
    result = DX::CreateBufferUAV(m_pCountBuffer.Get(), 1, "SurfaceBufferUAV",
                                 &m_pCountBufferUAV);
    DX::ThrowIfFailed(result, "Failed in surface UAV");
  }

  result = DX::CompileAndCreateShader(
      L"shaders/MarchingCubesIndirect.vs",
      (ID3D11DeviceChild **)m_pVertexShader.GetAddressOf(), {});
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/MarchingCubes.ps",
      (ID3D11DeviceChild **)m_pPixelShader.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/MarchingCubes.cs",
      (ID3D11DeviceChild **)m_pMarchingComputeShader.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/SurfaceCounter.cs",
      (ID3D11DeviceChild **)m_pSurfaceCountCS.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/SmoothingPreprocess.cs",
      (ID3D11DeviceChild **)m_pSmoothingPreprocessCS.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/BoxFilter.cs",
      (ID3D11DeviceChild **)m_pBoxFilterCS.GetAddressOf());
  DX::ThrowIfFailed(result);

  result = DX::CompileAndCreateShader(
      L"shaders/marching-cubes/GaussFilter.cs",
      (ID3D11DeviceChild **)m_pGaussFilterCS.GetAddressOf());
  DX::ThrowIfFailed(result);

  try {
    CreateQueries();
  } catch (...) {
    throw;
  }
}

void MCGpu::Update(const SphGpu &sph) {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;

  pContext->Begin(m_pQueryDisjoint.Get());
  pContext->End(m_pQueryMarchingStart.Get());
  UINT cubeNums = (m_settings.marchingResolution.x + 1) *
                  (m_settings.marchingResolution.y + 1) *
                  (m_settings.marchingResolution.z + 1);

  UINT groupNumber = DivUp(cubeNums, m_settings.blockSize);
  ID3D11Buffer *cb[1] = {sph.m_pSphCB.Get()};
  pContext->CSSetConstantBuffers(0, 1, cb);

  if (m_frameNum == 0) {
    ID3D11UnorderedAccessView *uavs[1] = {m_pCountBufferUAV.Get()};
    ID3D11ShaderResourceView *srvs[2] = {sph.m_pEntriesBufferSRV.Get(),
                                         sph.m_pHashBufferSRV.Get()};

    pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
    pContext->CSSetShaderResources(0, 2, srvs);
    pContext->CSSetShader(m_pSurfaceCountCS.Get(), nullptr, 0);
    pContext->Dispatch(groupNumber, 1, 1);

    ID3D11UnorderedAccessView *uavsNULL[1] = {nullptr};
    ID3D11ShaderResourceView *srvsNULL[2] = {nullptr, nullptr};
    pContext->CSSetUnorderedAccessViews(0, 1, uavsNULL, nullptr);
    pContext->CSSetShaderResources(0, 2, srvsNULL);
  }

  {
    ID3D11UnorderedAccessView *uavs[2] = {m_pVoxelGridBufferUAV1.Get(),
                                          m_pCountBufferUAV.Get()};
    ID3D11ShaderResourceView *srvs[2] = {sph.m_pEntriesBufferSRV.Get(),
                                         sph.m_pHashBufferSRV.Get()};

    pContext->CSSetShaderResources(0, 2, srvs);
    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    pContext->CSSetShader(m_pSmoothingPreprocessCS.Get(), nullptr, 0);

    pContext->Dispatch(groupNumber, 1, 1);

    ID3D11UnorderedAccessView *uavsNULL[2] = {nullptr, nullptr};
    ID3D11ShaderResourceView *srvsNULL[2] = {nullptr, nullptr};
    pContext->CSSetUnorderedAccessViews(0, 2, uavsNULL, nullptr);
    pContext->CSSetShaderResources(0, 2, srvsNULL);
  }

  {
    ID3D11UnorderedAccessView *uavs[2] = {m_pVoxelGridBufferUAV1.Get(),
                                          m_pVoxelGridBufferUAV2.Get()};
    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
    pContext->CSSetShader(m_pBoxFilterCS.Get(), nullptr, 0);
    // pContext->Dispatch(groupNumber, 1, 1);
  }

  {
    ID3D11UnorderedAccessView *uavs[2] = {m_pVoxelGridBufferUAV2.Get(),
                                          m_pVoxelGridBufferUAV1.Get()};
    pContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
    pContext->CSSetShader(m_pGaussFilterCS.Get(), nullptr, 0);
    // pContext->Dispatch(groupNumber, 1, 1);
  }

  pContext->End(m_pQueryMarchingPreprocess.Get());
  {
    const UINT pCounters[3] = {0, 0, 0};
    ID3D11UnorderedAccessView *uavs[3] = {m_pVoxelGridBufferUAV1.Get(),
                                          m_pMarchingOutBufferUAV.Get(),
                                          m_pCountBufferUAV.Get()};

    pContext->CSSetUnorderedAccessViews(0, 3, uavs, pCounters);

    pContext->CSSetShader(m_pMarchingComputeShader.Get(), nullptr, 0);

    pContext->Dispatch(groupNumber, 1, 1);

    ID3D11UnorderedAccessView *uavsNULL[3] = {nullptr, nullptr, nullptr};
    pContext->CSSetUnorderedAccessViews(0, 3, uavsNULL, nullptr);

    pContext->CopyStructureCount(m_pIndirectBuffer.Get(), sizeof(UINT),
                                 m_pMarchingOutBufferUAV.Get());
  }
  pContext->End(m_pQueryMarchingMain.Get());
  pContext->End(m_pQueryDisjoint.Get());
}

void MCGpu::Render(ID3D11Buffer *pSceneBuffer) {
  auto dxResources = DeviceResources::getInstance();

  auto pContext = dxResources.m_pDeviceContext;
  auto pTransDepthState = dxResources.m_pTransDepthState;
  auto pTransBlendState = dxResources.m_pTransBlendState;
  pContext->OMSetDepthStencilState(m_states->DepthReverseZ(), 0);
  pContext->OMSetBlendState(m_states->AlphaBlend(), nullptr, 0xffffffff);

  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

  ID3D11Buffer *cbuffers[1] = {pSceneBuffer};
  pContext->VSSetConstantBuffers(0, 1, cbuffers);
  pContext->PSSetConstantBuffers(0, 1, cbuffers);
  pContext->IASetInputLayout(nullptr);
  pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);

  ID3D11ShaderResourceView *srvs = {m_pMarchingOutBufferSRV.Get()};
  pContext->VSSetShaderResources(0, 1, &srvs);
  pContext->DrawInstancedIndirect(m_pIndirectBuffer.Get(), 0);

  ID3D11Buffer *nullBuf[1] = {nullptr};
  ID3D11ShaderResourceView *nullSRV[1] = {nullptr};
  pContext->VSSetShaderResources(0, 1, nullSRV);
  pContext->VSSetConstantBuffers(0, 1, nullBuf);
}

void MCGpu::CreateQueries() {
  auto pDevice = DeviceResources::getInstance().m_pDevice;

  D3D11_QUERY_DESC desc{};
  desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  desc.MiscFlags = 0;
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryDisjoint));
  desc.Query = D3D11_QUERY_TIMESTAMP;
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryMarchingStart));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryMarchingPreprocess));
  DX::ThrowIfFailed(pDevice->CreateQuery(&desc, &m_pQueryMarchingMain));
}

void MCGpu::CollectTimestamps() {
  auto pContext = DeviceResources::getInstance().m_pDeviceContext;

  while (pContext->GetData(m_pQueryDisjoint.Get(), nullptr, 0, 0) == S_FALSE)
    ;

  D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
  pContext->GetData(m_pQueryDisjoint.Get(), &tsDisjoint,
                    sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);

  if (tsDisjoint.Disjoint) {
    return;
  }

  // Get all the timestamps
  UINT64 tsMarchingBegin, tsMarchingPreproc, tsMarchingMain;

  pContext->GetData(m_pQueryMarchingStart.Get(), &tsMarchingBegin,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQueryMarchingPreprocess.Get(), &tsMarchingPreproc,
                    sizeof(UINT64), 0);
  pContext->GetData(m_pQueryMarchingMain.Get(), &tsMarchingMain, sizeof(UINT64),
                    0);

  // Convert to real time
  m_preprocessTime = float(tsMarchingPreproc - tsMarchingBegin) /
                     float(tsDisjoint.Frequency) * 1000.0f;
  m_mainTime = float(tsMarchingMain - tsMarchingPreproc) /
               float(tsDisjoint.Frequency) * 1000.0f;
  m_sumTime += m_mainTime + m_preprocessTime;
}

void MCGpu::ImGuiRender() {
  CollectTimestamps();
  if (ImGui::CollapsingHeader("MarchingCubes"),
      ImGuiTreeNodeFlags_DefaultOpen) {
    ImGui::Text("MC pass: %.3f ms", m_preprocessTime + m_mainTime);
    ImGui::Text("MC Avg time: %.3f ms", m_sumTime / m_frameNum);
    ImGui::Text("Preprocess time: %.3f ms", m_preprocessTime);
    ImGui::Text("Main time: %.3f ms", m_mainTime);
  }
}
