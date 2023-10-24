#pragma once

#include <DirectXMath.h>
#include <SimpleMath.h>

#include <iostream>
#include <vector>

#include "primitive.h"
#include "sph.h"

using namespace DirectX::SimpleMath;

class Water : public GeometricPrimitive {
  struct Vertex {
    Vector3 pos;
  };

  struct GeomBuffer {
    DirectX::XMMATRIX m[1000];
    Vector4 color;
  };

 public:
  Water(std::shared_ptr<DXController>& pDXController)
      : GeometricPrimitive(pDXController),
        m_pSph(nullptr),
        m_pGeomBuffer(nullptr),
        m_sphereIndexCount(0),
        m_numParticles(1000) {}

  ~Water() {
    delete m_pSph;
    SAFE_RELEASE(m_pGeomBuffer);
  }

  HRESULT Init() override;
  HRESULT Init(int numParticles);
  void Update(float dt);
  void Render(ID3D11Buffer* pSceneBuffer = nullptr) override;

 private:
  int m_numParticles;

  SPH* m_pSph;

  ID3D11Buffer* m_pGeomBuffer;

  GeomBuffer m_gb;

  UINT m_sphereIndexCount;
};
