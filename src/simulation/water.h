#pragma once

#include <DirectXMath.h>
#include <SimpleMath.h>

#include <iostream>
#include <vector>

#include "heightfield.h"
#include "marching-cubes.h"
#include "primitive.h"
#include "sph.h"

using namespace DirectX::SimpleMath;

class Water : public ::Primitive {
  struct Vertex {
    Vector3 pos;
  };

  struct InstanceData {
    Vector3 pos;
    float density;
  };

  struct GeomBuffer {
    DirectX::XMMATRIX m[1000];
    Vector4 color;
  };

 public:
  Water(std::shared_ptr<DX::DeviceResources>& pDeviceResources)
      : Primitive(pDeviceResources),
        // m_sphereIndexCount(0),
        m_numParticles(1000) {}

  ~Water() {
    // delete m_pSph;
    // delete[] m_instanceData;
    delete m_heightfield;
    // SAFE_RELEASE(m_pInstanceBuffer);
  }

  HRESULT Init() override;
  HRESULT Init(UINT boxWidth);
  void Update(float dt);
  void Render(ID3D11Buffer* pSceneBuffer = nullptr) override;
  void StartImpulse(int x, int y, float value, float radius);
  void StopImpulse(int x, int y, float radius);

 private:
  int m_numParticles;

  HeightField* m_heightfield;
  std::vector<Vector3> m_vertecies;
  std::vector<UINT16> m_indecies;

  // MarchingCube* m_pMarchingCube;

  ID3D11Buffer* m_pInstanceBuffer;
  InstanceData* m_instanceData;

  UINT m_sphereIndexCount;
};
