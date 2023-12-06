#pragma once

#include <stdint.h>

#include "dx-controller.h"
#include "marching-cubes.h"
#include "neighbour-hash.h"
#include "primitive.h"

#define _USE_MATH_DEFINES

#include <SimpleMath.h>
#include <math.h>

#include <iostream>
#include <vector>

using namespace DirectX::SimpleMath;
using namespace DirectX;

class SPH : GeometricPrimitive {
 public:
  struct Props {
    Vector3 pos = Vector3::Zero;
    XMINT3 cubeNum = XMINT3(5, 5, 5);
    float cubeLen = 0.15f;
    float h = 0.1f;
    float mass = 0.02f;
    float dynamicViscosity = 1.1f;
    float particleRadius = 0.1f;
    float dampingCoeff = 0.5f;
  };

  std::vector<Particle> m_particles;
  int n = 0;

  SPH() = delete;
  SPH(SPH const& s) = delete;
  SPH(SPH&& s) = delete;
  SPH(std::shared_ptr<DXController> dxController, Props& props)
      : GeometricPrimitive(dxController), m_props(props) {
    poly6 = 315.0f / (64.0f * M_PI * pow(props.h, 9));
    spikyGrad = -45.0f / (M_PI * pow(props.h, 6));
    spikyLap = 45.0f / (M_PI * pow(props.h, 6));
    h2 = props.h * props.h;
  };

  HRESULT Init() override;
  void Update(float dt);
  void UpdatePhysics(float dt);
  void Render(ID3D11Buffer* pSceneBuffer = nullptr) override;

  Props m_props;
  bool isMarching = false;

 private:
  float poly6;
  float spikyGrad;
  float spikyLap;

  NeighbourHash m_hashM;

  float h2;
  UINT m_sphereIndexCount;

  struct InstanceData {
    Vector3 pos;
    float density;
  };

  ID3D11Buffer* m_pInstanceBuffer;
  MarchingCube* m_pMarchingCube;
  std::vector<InstanceData> m_instanceData;

  std::vector<Vector3> m_vertex;
  std::vector<UINT> m_index;

  ID3D11Buffer* m_pMarchingVertexBuffer;
  ID3D11VertexShader* m_pMarchingVertexShader;
  ID3D11PixelShader* m_pMarchingPixelShader;
  ID3D11InputLayout* m_pMarchingInputLayout;

  HRESULT InitSpheres();
  HRESULT InitMarching();
  void UpdateDensity();
  void UpdateForces();
  void CheckBoundary(Particle& p);

  void RenderMarching(ID3D11Buffer* pSceneBuffer = nullptr);
  void RenderSpheres(ID3D11Buffer* pSceneBuffer = nullptr);
};
