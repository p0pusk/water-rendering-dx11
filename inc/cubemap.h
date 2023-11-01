#pragma once

#include <DirectXMath.h>

#include "primitive.h"

class CubeMap : public GeometricPrimitive {
 public:
  struct SphereGeomBuffer {
    DirectX::XMMATRIX m;
    float size;
  };

  CubeMap(std::shared_ptr<DXController>& pDXController)
      : GeometricPrimitive(pDXController),
        m_pCubemapView(nullptr),
        m_pGeomBuffer(nullptr){};

  ~CubeMap() { SAFE_RELEASE(m_pGeomBuffer); }

  HRESULT Init() override;
  void Render(ID3D11Buffer* pSceneBuffer = nullptr) override;

  void Resize(UINT width, UINT height);

  ID3D11ShaderResourceView* m_pCubemapView;

 private:
  ID3D11Buffer* m_pGeomBuffer;
  UINT m_sphereIndexCount;

  HRESULT InitCubemap();
};
