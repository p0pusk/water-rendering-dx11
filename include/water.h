#pragma once

#include <DirectXMath.h>
#include <SimpleMath.h>

#include <iostream>
#include <vector>

#include "device-resources.h"
#include "heightfield.h"
#include "marching-cubes.h"
#include "sph.h"

using namespace DirectX::SimpleMath;

class Water {
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
  Water()
      : // m_sphereIndexCount(0),
        m_numParticles(1000) {}

  HRESULT Init(UINT boxWidth);
  void Update(float dt);
  void Render(ID3D11Buffer *pSceneBuffer = nullptr);
  void StartImpulse(int x, int y, float value, float radius);
  void StopImpulse(int x, int y, float radius);

private:
  int m_numParticles;

  HeightField *m_heightfield;
  std::vector<Vector3> m_vertecies;
  std::vector<UINT16> m_indecies;

  ComPtr<ID3D11Buffer> m_pVertexBuffer;
  ComPtr<ID3D11Buffer> m_pIndexBuffer;
  ComPtr<ID3D11InputLayout> m_pInputLayout;
  ComPtr<ID3D11VertexShader> m_pVertexShader;
  ComPtr<ID3D11PixelShader> m_pPixelShader;

  // MarchingCube* m_pMarchingCube;

  ComPtr<ID3D11Buffer> m_pInstanceBuffer;
  InstanceData *m_instanceData;

  UINT m_sphereIndexCount;
};
