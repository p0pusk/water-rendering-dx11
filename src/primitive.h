#pragma once

#include "pch.h"

#include "dx-controller.h"

using namespace DirectX::SimpleMath;

class GeometricPrimitive {
 public:
  GeometricPrimitive(std::shared_ptr<DXController>& pDXController)
      : m_pDXC(pDXController),
        m_pVertexBuffer(nullptr),
        m_pIndexBuffer(nullptr),
        m_pVertexShader(nullptr),
        m_pPixelShader(nullptr),
        m_pInputLayout(nullptr) {}

  ~GeometricPrimitive() {
    SAFE_RELEASE(m_pVertexBuffer);
    SAFE_RELEASE(m_pIndexBuffer);
    SAFE_RELEASE(m_pVertexShader);
    SAFE_RELEASE(m_pPixelShader);
    SAFE_RELEASE(m_pInputLayout);
  }

  virtual HRESULT Init() = 0;
  virtual void Render(ID3D11Buffer* pSceneBuffer = nullptr) = 0;

 protected:
  std::shared_ptr<DXController> m_pDXC;

  ID3D11Buffer* m_pVertexBuffer;
  ID3D11Buffer* m_pIndexBuffer;

  ID3D11VertexShader* m_pVertexShader;
  ID3D11PixelShader* m_pPixelShader;
  ID3D11InputLayout* m_pInputLayout;

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

  HRESULT CompileAndCreateShader(const std::wstring& path,
                                 ID3D11DeviceChild** ppShader,
                                 const std::vector<std::string>& defines = {},
                                 ID3DBlob** ppCode = nullptr);

  void GetSphereDataSize(size_t latCells, size_t lonCells, size_t& indexCount,
                         size_t& vertexCount);

  void CreateSphere(size_t latCells, size_t lonCells, UINT16* pIndices,
                    Vector3* pPos);
};
