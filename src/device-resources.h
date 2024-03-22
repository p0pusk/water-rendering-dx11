#pragma once

#include <exception>

#include "pch.h"

using Microsoft::WRL::ComPtr;

namespace DX {
class DeviceResources {
 public:
  DeviceResources()
      : m_pDevice(nullptr),
        m_pDeviceContext(nullptr),
        m_pSwapChain(nullptr),
        m_pBackBufferRTV(nullptr),
        m_pDepthBuffer(nullptr),
        m_pDepthBufferDSV(nullptr),
        m_pDepthState(nullptr),
        m_pTransDepthState(nullptr),
        m_pRasterizerState(nullptr),
        m_pTransBlendState(nullptr),
        m_pOpaqueBlendState(nullptr),
        m_pSampler(nullptr),
        m_height(0),
        m_width(0) {}

  ~DeviceResources() {
    SAFE_RELEASE(m_pBackBufferRTV);
    SAFE_RELEASE(m_pSampler);
    SAFE_RELEASE(m_pDepthBuffer);
    SAFE_RELEASE(m_pDepthBufferDSV);
    SAFE_RELEASE(m_pDepthState);
    SAFE_RELEASE(m_pTransDepthState);
    SAFE_RELEASE(m_pRasterizerState);
    SAFE_RELEASE(m_pTransBlendState);
    SAFE_RELEASE(m_pOpaqueBlendState);
    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pDeviceContext);
    SAFE_RELEASE(m_pDevice);
  }

  HRESULT Init(const HWND& hWnd);
  bool Resize(UINT w, UINT h);

  auto GetDevice() { return m_pDevice.Get(); };
  auto GetDeviceContext() { return m_pDeviceContext.Get(); };
  auto GetSwapChain() { return m_pSwapChain.Get(); }
  auto GetSampler() { return m_pSampler.Get(); }
  auto GetDepthState() { return m_pDepthState.Get(); };
  auto GetOpaqueBlendState() { return m_pOpaqueBlendState.Get(); };
  auto GetTransBlendState() { return m_pTransBlendState.Get(); };
  auto GetTransDepthState() { return m_pTransDepthState.Get(); };
  auto GetBackBufferRTV() { return m_pBackBufferRTV.Get(); };
  auto GetDepthBufferDSV() { return m_pDepthBufferDSV.Get(); };
  auto GetRasterizerState() { return m_pRasterizerState.Get(); };

  HRESULT CompileAndCreateShader(const std::wstring& path,
                                 ID3D11DeviceChild** ppShader,
                                 const std::vector<std::string>& defines = {},
                                 ID3DBlob** ppCode = nullptr);

  template <typename T>
  HRESULT CreateConstantBuffer(ID3D11Buffer** ppCB, UINT cpuFlags,
                               D3D11_SUBRESOURCE_DATA* data,
                               const std::string& name) {
    HRESULT result = S_OK;
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(T);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = cpuFlags;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    DX::ThrowIfFailed(m_pDevice->CreateBuffer(&desc, data, ppCB),
                      "Failed to create CB: " + name);

    DX::ThrowIfFailed(SetResourceName(*ppCB, name),
                      "Failed to set resource name: " + name);

    return SUCCEEDED(result);
  };

  template <typename T>
  HRESULT CreateConstantBuffer(D3D11_USAGE usageFlag, UINT cpuFlags,
                               D3D11_SUBRESOURCE_DATA* pData,
                               const std::string& name,
                               ID3D11Buffer** ppBufferOut) {
    HRESULT result = S_OK;
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(T);
    desc.Usage = usageFlag;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = cpuFlags;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    DX::ThrowIfFailed(m_pDevice->CreateBuffer(&desc, pData, ppBufferOut),
                      "Failed to create CB: " + name);

    DX::ThrowIfFailed(SetResourceName(*ppBufferOut, name),
                      "Failed to set resource name: " + name);

    return result;
  };

  template <typename T>
  HRESULT CreateStructuredBuffer(UINT numElements, D3D11_USAGE usageFlag,
                                 UINT cpuFlags, D3D11_SUBRESOURCE_DATA* pData,
                                 const std::string& name,
                                 ID3D11Buffer** ppBufferOut) {
    HRESULT result = S_OK;
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = usageFlag;
    desc.ByteWidth = numElements * sizeof(T);
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = cpuFlags;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(T);

    result = m_pDevice->CreateBuffer(&desc, pData, ppBufferOut);
    DX::ThrowIfFailed(result, "Unable to create structured buffer" + name);
    result = SetResourceName(*ppBufferOut, name);
    DX::ThrowIfFailed(result, "Unable to SetResourceName to" + name);

    return result;
  };

  HRESULT CreateBufferUAV(ID3D11Resource* pResource, UINT numElements,
                          D3D11_BUFFER_UAV_FLAG bufferFlags,
                          const std::string& name,
                          ID3D11UnorderedAccessView** ppResourceUAV) {
    HRESULT result = S_OK;
    D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = numElements;
    desc.Buffer.Flags = bufferFlags;

    DX::ThrowIfFailed(
        m_pDevice->CreateUnorderedAccessView(pResource, &desc, ppResourceUAV),
        "Unable to create UAV" + name);
    DX::ThrowIfFailed(SetResourceName(*ppResourceUAV, name),
                      "Unable to SetResourceName to" + name);

    return result;
  }

  HRESULT CreateBufferSRV(ID3D11Resource* pResource, UINT numElements,
                          const std::string& name,
                          ID3D11ShaderResourceView** ppResourceSRV) {
    HRESULT result = S_OK;
    D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = numElements;

    DX::ThrowIfFailed(
        m_pDevice->CreateShaderResourceView(pResource, &desc, ppResourceSRV),
        "Unable to create SRV" + name);
    DX::ThrowIfFailed(SetResourceName(*ppResourceSRV, name),
                      "Unable to SetResourceName to" + name);

    return result;
  }

  template <typename T>
  HRESULT CreateVertexBuffer(UINT numElements, D3D11_USAGE usageFlag,
                             UINT cpuFlags, D3D11_SUBRESOURCE_DATA* pData,
                             const std::string& name,
                             ID3D11Buffer** ppBufferOUt) {
    HRESULT result = S_OK;
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(numElements * sizeof(T));
    desc.Usage = usageFlag;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = cpuFlags;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    result = m_pDevice->CreateBuffer(&desc, pData, ppBufferOUt);
    DX::ThrowIfFailed(result);

    result = SetResourceName(*ppBufferOUt, name);
    DX::ThrowIfFailed(result);

    return result;
  }

 private:
  ComPtr<ID3D11Device> m_pDevice;
  ComPtr<ID3D11DeviceContext> m_pDeviceContext;
  ComPtr<IDXGISwapChain> m_pSwapChain;
  ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV;
  ComPtr<ID3D11Texture2D> m_pDepthBuffer;
  ComPtr<ID3D11DepthStencilView> m_pDepthBufferDSV;
  ComPtr<ID3D11DepthStencilState> m_pDepthState;
  ComPtr<ID3D11DepthStencilState> m_pTransDepthState;
  ComPtr<ID3D11RasterizerState> m_pRasterizerState;
  ComPtr<ID3D11BlendState> m_pTransBlendState;
  ComPtr<ID3D11BlendState> m_pOpaqueBlendState;
  ComPtr<ID3D11SamplerState> m_pSampler;
  ComPtr<IDXGIDebug> m_debugDev;

  HRESULT SetupBackBuffer();

  UINT m_width;
  UINT m_height;

  class D3DInclude : public ID3DInclude {
    STDMETHOD(Open)
    (THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData,
     LPCVOID* ppData, UINT* pBytes) {
      FILE* pFile = nullptr;
      fopen_s(&pFile, pFileName, "rb");
      assert(pFile != nullptr);
      if (pFile == nullptr) {
        return E_FAIL;
      }

      fseek(pFile, 0, SEEK_END);
      long long size = _ftelli64(pFile);
      fseek(pFile, 0, SEEK_SET);

      VOID* pData = malloc(size);
      if (pData == nullptr) {
        fclose(pFile);
        return E_FAIL;
      }

      size_t rd = fread(pData, 1, size, pFile);
      assert(rd == (size_t)size);

      if (rd != (size_t)size) {
        fclose(pFile);
        free(pData);
        return E_FAIL;
      }

      *ppData = pData;
      *pBytes = (UINT)size;

      return S_OK;
    }
    STDMETHOD(Close)(THIS_ LPCVOID pData) {
      free(const_cast<void*>(pData));
      return S_OK;
    }
  };
};
}  // namespace DX
