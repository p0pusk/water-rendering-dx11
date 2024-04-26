#pragma once

#include "device-resources.h"

namespace DX {

inline HRESULT SetResourceName(ID3D11DeviceChild *pResource,
                               const std::string &name) {
  return pResource->SetPrivateData(WKPDID_D3DDebugObjectName,
                                   (UINT)name.length(), name.c_str());
}

HRESULT CompileAndCreateShader(const std::wstring &path,
                               ID3D11DeviceChild **ppShader,
                               const std::vector<std::string> &defines = {},
                               ID3DBlob **ppCode = nullptr);

template <typename T>
HRESULT CreateConstantBuffer(ID3D11Buffer **ppCB, UINT cpuFlags,
                             D3D11_SUBRESOURCE_DATA *data,
                             const std::string &name) {
  HRESULT result = S_OK;
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = sizeof(T);
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags = cpuFlags;
  desc.MiscFlags = 0;
  desc.StructureByteStride = 0;

  DX::ThrowIfFailed(
      DeviceResources::getInstance().m_pDevice->CreateBuffer(&desc, data, ppCB),
      "Failed to create CB: " + name);

  DX::ThrowIfFailed(SetResourceName(*ppCB, name),
                    "Failed to set resource name: " + name);

  return SUCCEEDED(result);
};

template <typename T>
HRESULT CreateConstantBuffer(D3D11_USAGE usageFlag, UINT cpuFlags,
                             D3D11_SUBRESOURCE_DATA *pData,
                             const std::string &name,
                             ID3D11Buffer **ppBufferOut) {
  HRESULT result = S_OK;
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = sizeof(T);
  desc.Usage = usageFlag;
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags = cpuFlags;
  desc.MiscFlags = 0;
  desc.StructureByteStride = 0;

  DX::ThrowIfFailed(DeviceResources::getInstance().m_pDevice->CreateBuffer(
                        &desc, pData, ppBufferOut),
                    "Failed to create CB: " + name);

  DX::ThrowIfFailed(SetResourceName(*ppBufferOut, name),
                    "Failed to set resource name: " + name);

  return result;
};

template <typename T>
HRESULT CreateStructuredBuffer(UINT numElements, D3D11_USAGE usageFlag,
                               UINT cpuFlags, D3D11_SUBRESOURCE_DATA *pData,
                               const std::string &name,
                               ID3D11Buffer **ppBufferOut) {
  HRESULT result = S_OK;
  D3D11_BUFFER_DESC desc = {};
  desc.Usage = usageFlag;
  desc.ByteWidth = numElements * sizeof(T);
  desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = cpuFlags;
  desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  desc.StructureByteStride = sizeof(T);

  result = DeviceResources::getInstance().m_pDevice->CreateBuffer(&desc, pData,
                                                                  ppBufferOut);
  DX::ThrowIfFailed(result, "Unable to create structured buffer: " + name);
  result = SetResourceName(*ppBufferOut, name);
  DX::ThrowIfFailed(result, "Unable to SetResourceName to: " + name);

  return result;
};

static HRESULT
CreateBufferUAV(ID3D11Resource *pResource, UINT numElements,
                const std::string &name,
                ID3D11UnorderedAccessView **ppResourceUAV,
                D3D11_BUFFER_UAV_FLAG bufferFlags = (D3D11_BUFFER_UAV_FLAG)0) {
  HRESULT result = S_OK;
  D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
  desc.Buffer.FirstElement = 0;
  desc.Buffer.NumElements = numElements;
  desc.Buffer.Flags = bufferFlags;

  DX::ThrowIfFailed(
      DeviceResources::getInstance().m_pDevice->CreateUnorderedAccessView(
          pResource, &desc, ppResourceUAV),
      "Unable to create UAV: " + name);
  DX::ThrowIfFailed(SetResourceName(*ppResourceUAV, name),
                    "Unable to SetResourceName to: " + name);

  return result;
}

static HRESULT CreateBufferSRV(ID3D11Resource *pResource, UINT numElements,
                               const std::string &name,
                               ID3D11ShaderResourceView **ppResourceSRV) {
  HRESULT result = S_OK;
  D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
  desc.Buffer.FirstElement = 0;
  desc.Buffer.NumElements = numElements;

  DX::ThrowIfFailed(
      DeviceResources::getInstance().m_pDevice->CreateShaderResourceView(
          pResource, &desc, ppResourceSRV),
      "Unable to create SRV" + name);
  DX::ThrowIfFailed(SetResourceName(*ppResourceSRV, name),
                    "Unable to SetResourceName to " + name);

  return result;
}

template <typename T>
HRESULT CreateVertexBuffer(UINT numElements, D3D11_USAGE usageFlag,
                           UINT cpuFlags, D3D11_SUBRESOURCE_DATA *pData,
                           const std::string &name,
                           ID3D11Buffer **ppBufferOUt) {

  HRESULT result = S_OK;
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = (UINT)(numElements * sizeof(T));
  desc.Usage = usageFlag;
  desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  desc.CPUAccessFlags = cpuFlags;
  desc.MiscFlags = 0;
  desc.StructureByteStride = 0;

  auto pDevice = DeviceResources::getInstance().m_pDevice;
  result = pDevice->CreateBuffer(&desc, pData, ppBufferOUt);
  DX::ThrowIfFailed(result, "Unable to CreateVertexBuffer" + name);

  result = SetResourceName(*ppBufferOUt, name);
  DX::ThrowIfFailed(result, "Unable to SetResourceName to" + name);

  return result;
}

class D3DInclude : public ID3DInclude {
  STDMETHOD(Open)
  (THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData,
   LPCVOID *ppData, UINT *pBytes) {
    FILE *pFile = nullptr;
    fopen_s(&pFile, pFileName, "rb");
    assert(pFile != nullptr);
    if (pFile == nullptr) {
      return E_FAIL;
    }

    fseek(pFile, 0, SEEK_END);
    long long size = _ftelli64(pFile);
    fseek(pFile, 0, SEEK_SET);

    VOID *pData = malloc(size);
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
    free(const_cast<void *>(pData));
    return S_OK;
  }
};

static inline void GetSphereDataSize(size_t latCells, size_t lonCells,
                                     size_t &indexCount, size_t &vertexCount) {
  vertexCount = (latCells + 1) * (lonCells + 1);
  indexCount = latCells * lonCells * 6;
}

static inline void CreateSphere(size_t latCells, size_t lonCells,
                                UINT16 *pIndices, Vector3 *pPos) {
  for (size_t lat = 0; lat < latCells + 1; lat++) {
    for (size_t lon = 0; lon < lonCells + 1; lon++) {
      int index = (int)(lat * (lonCells + 1) + lon);
      float lonAngle = 2.0f * (float)M_PI * lon / lonCells + (float)M_PI;
      float latAngle = -(float)M_PI / 2 + (float)M_PI * lat / latCells;

      Vector3 r = Vector3{sinf(lonAngle) * cosf(latAngle), sinf(latAngle),
                          cosf(lonAngle) * cosf(latAngle)};

      pPos[index] = r * 0.5f;
    }
  }

  for (size_t lat = 0; lat < latCells; lat++) {
    for (size_t lon = 0; lon < lonCells; lon++) {
      size_t index = lat * lonCells * 6 + lon * 6;
      pIndices[index + 0] = (UINT16)(lat * (latCells + 1) + lon + 0);
      pIndices[index + 2] = (UINT16)(lat * (latCells + 1) + lon + 1);
      pIndices[index + 1] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon);
      pIndices[index + 3] = (UINT16)(lat * (latCells + 1) + lon + 1);
      pIndices[index + 5] =
          (UINT16)(lat * (latCells + 1) + latCells + 1 + lon + 1);
      pIndices[index + 4] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon);
    }
  }
}

} // namespace DX
