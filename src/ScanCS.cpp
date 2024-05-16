//--------------------------------------------------------------------------------------
// File: ScanCS.cpp
//
// A simple inclusive prefix sum(scan) implemented in CS4.0
//
// Note, to maintain the simplicity of the sample, this scan has these
// limitations:
//      - At maximum 16384 elements can be scanned.
//      - The element to be scanned is of type uint2, see comments in
//      ScanCS.hlsl
//        and below for how to change this type
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
#include "ScanCS.h"
#include "pch.h"
#include "utils.h"
#include <cmath>

#define MAX_N 262144

//--------------------------------------------------------------------------------------
CScanCS::CScanCS()
    : m_pScanCS(nullptr), m_pScan2CS(nullptr), m_pcbCS(nullptr),
      m_pAuxBuf(nullptr), m_pAuxBufRV(nullptr), m_pAuxBufUAV(nullptr) {}

//--------------------------------------------------------------------------------------
HRESULT CScanCS::OnD3D11CreateDevice(ID3D11Device *pd3dDevice) {
  HRESULT hr;

  DX::ThrowIfFailed(DX::CompileAndCreateShader(
      L"./shaders/neighbouring/ScanCS.hlsl", (ID3D11DeviceChild **)&m_pScanCS,
      {}, nullptr, "CSScanInBucket", "cs_5_0"));

  DX::ThrowIfFailed(DX::CompileAndCreateShader(
      L"./shaders/neighbouring/ScanCS.hlsl", (ID3D11DeviceChild **)&m_pScan2CS,
      {}, nullptr, "CSScanBucketResult", "cs_5_0"));

  DX::ThrowIfFailed(DX::CompileAndCreateShader(
      L"./shaders/neighbouring/ScanCS.hlsl", (ID3D11DeviceChild **)&m_pScan3CS,
      {}, nullptr, "CSScanAddBucketResult", "cs_5_0"));

  D3D11_BUFFER_DESC Desc = {};
  Desc.Usage = D3D11_USAGE_DYNAMIC;
  Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  Desc.MiscFlags = 0;
  Desc.ByteWidth = sizeof(CB_CS);

  D3D11_SUBRESOURCE_DATA data = {};
  hr = pd3dDevice->CreateBuffer(&Desc, nullptr, &m_pcbCS);
  DX::ThrowIfFailed(DX::SetResourceName(m_pcbCS, "CB_CS"));

  ZeroMemory(&Desc, sizeof(Desc));
  Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  Desc.StructureByteStride =
      sizeof(UINT); // If scan types other than uint2, remember change here
  Desc.ByteWidth = Desc.StructureByteStride * 512;
  Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  Desc.Usage = D3D11_USAGE_DEFAULT;
  hr = pd3dDevice->CreateBuffer(&Desc, nullptr, &m_pAuxBuf);
  DX::ThrowIfFailed(DX::SetResourceName(m_pAuxBuf, "Aux"));

  D3D11_SHADER_RESOURCE_VIEW_DESC DescRV = {};
  DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
  DescRV.Format = DXGI_FORMAT_UNKNOWN;
  DescRV.Buffer.FirstElement = 0;
  DescRV.Buffer.NumElements = 512;
  DX::ThrowIfFailed(
      pd3dDevice->CreateShaderResourceView(m_pAuxBuf, &DescRV, &m_pAuxBufRV));
  DX::ThrowIfFailed(DX::SetResourceName(m_pAuxBufRV, "Aux SRV"));

  D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV = {};
  DescUAV.Format = DXGI_FORMAT_UNKNOWN;
  DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
  DescUAV.Buffer.FirstElement = 0;
  DescUAV.Buffer.NumElements = 512;
  DX::ThrowIfFailed(pd3dDevice->CreateUnorderedAccessView(m_pAuxBuf, &DescUAV,
                                                          &m_pAuxBufUAV));
  DX::ThrowIfFailed(DX::SetResourceName(m_pAuxBufUAV, "Aux UAV"));

  return hr;
}

//--------------------------------------------------------------------------------------
void CScanCS::OnD3D11DestroyDevice() {
  SAFE_RELEASE(m_pAuxBufRV);
  SAFE_RELEASE(m_pAuxBufUAV);
  SAFE_RELEASE(m_pAuxBuf);
  SAFE_RELEASE(m_pcbCS);
  SAFE_RELEASE(m_pScanCS);
  SAFE_RELEASE(m_pScan2CS);
  SAFE_RELEASE(m_pScan3CS);
}

//--------------------------------------------------------------------------------------
HRESULT CScanCS::ScanCS(ID3D11DeviceContext *pd3dImmediateContext,
                        INT nNumToScan, ID3D11ShaderResourceView *p0SRV,
                        ID3D11UnorderedAccessView *p0UAV,

                        ID3D11ShaderResourceView *p1SRV,
                        ID3D11UnorderedAccessView *p1UAV) {
  HRESULT hr = S_OK;

  int scanNum = std::ceil((float)nNumToScan / (float)(MAX_N - 1)) - 2 + nNumToScan;
  UINT chunkNums = DivUp(scanNum, MAX_N);

  for (int i = 0; i < chunkNums; ++i) {
    int curNum = std::min((int)MAX_N, (int)(scanNum - i * (MAX_N - 1)));

    // update cb
    {
      D3D11_MAPPED_SUBRESOURCE subresource;
      DX::ThrowIfFailed(pd3dImmediateContext->Map(
          m_pcbCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource));

      m_cb.param[0] = std::max(((MAX_N - 1) * i ), 0);
      memcpy(subresource.pData, &m_cb, sizeof(CB_CS));
      pd3dImmediateContext->Unmap(m_pcbCS, 0);
    }

    // first pass, scan in each bucket
    {
      ID3D11Buffer *cbs[1] = {m_pcbCS};
      pd3dImmediateContext->CSSetConstantBuffers(0, 1, cbs);
      pd3dImmediateContext->CSSetShader(m_pScanCS, nullptr, 0);

      ID3D11ShaderResourceView *aRViews[1] = {p0SRV};
      pd3dImmediateContext->CSSetShaderResources(0, 1, aRViews);

      ID3D11UnorderedAccessView *aUAViews[1] = {p1UAV};
      pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, nullptr);

      pd3dImmediateContext->Dispatch(INT(ceil(curNum / 512.0f)), 1, 1);

      ID3D11UnorderedAccessView *ppUAViewNULL[1] = {nullptr};
      pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL,
                                                      nullptr);
    }

    // second pass, record and scan the sum of each bucket
    {
      pd3dImmediateContext->CSSetShader(m_pScan2CS, nullptr, 0);

      ID3D11Buffer *cbs[1] = {m_pcbCS};
      pd3dImmediateContext->CSSetConstantBuffers(0, 1, cbs);

      ID3D11ShaderResourceView *aRViews[1] = {p1SRV};
      pd3dImmediateContext->CSSetShaderResources(0, 1, aRViews);

      ID3D11UnorderedAccessView *aUAViews[1] = {m_pAuxBufUAV};
      pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, nullptr);

      pd3dImmediateContext->Dispatch(1, 1, 1);

      ID3D11UnorderedAccessView *ppUAViewNULL[1] = {nullptr};
      pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL,
                                                      nullptr);
    }

    // last pass, add the buckets scanned result to each bucket to get the final
    // result
    {
      pd3dImmediateContext->CSSetShader(m_pScan3CS, nullptr, 0);

      ID3D11Buffer *cbs[1] = {m_pcbCS};
      pd3dImmediateContext->CSSetConstantBuffers(0, 1, cbs);

      ID3D11ShaderResourceView *aRViews[2] = {p1SRV, m_pAuxBufRV};
      pd3dImmediateContext->CSSetShaderResources(0, 2, aRViews);

      ID3D11UnorderedAccessView *aUAViews[1] = {p0UAV};
      pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, aUAViews, nullptr);

      pd3dImmediateContext->Dispatch(INT(ceil(curNum / 512.0f)), 1, 1);
    }

    {
      // Unbind resources for CS
      ID3D11UnorderedAccessView *ppUAViewNULL[1] = {nullptr};
      pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, ppUAViewNULL,
                                                      nullptr);
      ID3D11ShaderResourceView *ppSRVNULL[2] = {nullptr, nullptr};
      pd3dImmediateContext->CSSetShaderResources(0, 2, ppSRVNULL);
      ID3D11Buffer *ppBufferNULL[1] = {nullptr};
      pd3dImmediateContext->CSSetConstantBuffers(0, 1, ppBufferNULL);
    }
  }

  return hr;
}
