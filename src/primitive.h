#pragma once

#include "device-resources.h"
#include "pch.h"

using namespace DirectX::SimpleMath;

class Primitive {
 public:
  Primitive(std::shared_ptr<DX::DeviceResources> pDeviceResources)
      : m_pDeviceResources(pDeviceResources),
        m_pVertexBuffer(nullptr),
        m_pIndexBuffer(nullptr),
        m_pVertexShader(nullptr),
        m_pPixelShader(nullptr),
        m_pInputLayout(nullptr) {}

  ~Primitive() {
    SAFE_RELEASE(m_pVertexBuffer);
    SAFE_RELEASE(m_pIndexBuffer);
    SAFE_RELEASE(m_pVertexShader);
    SAFE_RELEASE(m_pPixelShader);
    SAFE_RELEASE(m_pInputLayout);
  }

  virtual HRESULT Init() = 0;
  virtual void Render(ID3D11Buffer* pSceneBuffer = nullptr) = 0;

 protected:
  std::shared_ptr<DX::DeviceResources> m_pDeviceResources;

  ID3D11Buffer* m_pVertexBuffer;
  ID3D11Buffer* m_pIndexBuffer;

  ID3D11VertexShader* m_pVertexShader;
  ID3D11PixelShader* m_pPixelShader;
  ID3D11InputLayout* m_pInputLayout;

  void GetSphereDataSize(size_t latCells, size_t lonCells, size_t& indexCount,
                         size_t& vertexCount);

  void CreateSphere(size_t latCells, size_t lonCells, UINT16* pIndices,
                    Vector3* pPos);
};
