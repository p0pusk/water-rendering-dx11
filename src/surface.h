#pragma once

#include "pch.h"
#include "primitive.h"

using namespace DirectX::SimpleMath;

class Surface : public ::Primitive {
  struct Vertex {
    Vector3 pos;
    Vector3 norm;
  };

  struct GeomBuffer {
    DirectX::XMMATRIX m;
    DirectX::XMMATRIX norm;
    Vector4 color;
  };

 public:
  Surface(std::shared_ptr<DX::DeviceResources> pDeviceResources,
          ID3D11ShaderResourceView* pCubemapView)

      : Primitive(pDeviceResources),
        m_pCubemapView(pCubemapView),
        m_pGeomBuffer(nullptr) {}

  ~Surface() { SAFE_RELEASE(m_pGeomBuffer); }

  HRESULT Init() override;
  HRESULT Init(Vector3 pos);
  void Render(ID3D11Buffer* pSceneBuffer = nullptr) override;

 private:
  ID3D11Buffer* m_pGeomBuffer;
  ID3D11ShaderResourceView* m_pCubemapView;
};
