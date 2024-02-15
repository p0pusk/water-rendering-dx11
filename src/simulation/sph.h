#pragma once

#include <stdint.h>

#include "dx-controller.h"
#include "marching-cubes.h"
#include "neighbour-hash.h"
#include "primitive.h"

#define _USE_MATH_DEFINES

#include "pch.h"

using namespace DirectX::SimpleMath;
using namespace DirectX;

struct MarchingVertex {
  Vector3 pos;
  Vector3 normal;
};

struct SphCB {
  Vector3 pos;
  UINT particleNum;
  XMINT3 cubeNum;
  float cubeLen;
  float h;
  float mass;
  float dynamicViscosity;
  float dampingCoeff;
};

struct SphDB {
  Vector4 dt;
};

class SPH : ::GeometricPrimitive {
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
  UINT m_num_particles;
  int n = 0;
  bool m_isCpu = false;

  SPH() = delete;
  SPH(SPH const& s) = delete;
  SPH(SPH&& s) = delete;
  SPH(std::shared_ptr<DXController> dxController, Props& props)
      : GeometricPrimitive(dxController),
        m_props(props),
        m_num_particles(props.cubeNum.x * props.cubeNum.y * props.cubeNum.z) {
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

  std::unordered_multimap<UINT, Particle*> m_hashM;

  UINT GetHash(XMINT3 cell);
  XMINT3 GetCell(Vector3 pos);

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
  ID3D11ComputeShader* m_pSPHComputeShader;
  ID3D11VertexShader* m_pMarchingVertexShader;
  ID3D11PixelShader* m_pMarchingPixelShader;

  ID3D11InputLayout* m_pMarchingInputLayout;

  ID3D11Buffer* m_pSphDataBuffer;
  SphCB m_sphCB;
  ID3D11Buffer* m_pSphCB;
  SphDB m_sphDB;
  ID3D11Buffer* m_pSphDB;
  ID3D11UnorderedAccessView* m_pSphBufferUAV;
  ID3D11ShaderResourceView* m_pSphBufferSRV;

  HRESULT InitSpheres();
  HRESULT InitMarching();
  HRESULT InitSph();
  void CheckBoundary(Particle& p);

  HRESULT UpdatePhysGPU(float dt);

  void RenderMarching(ID3D11Buffer* pSceneBuffer = nullptr);
  void RenderSpheres(ID3D11Buffer* pSceneBuffer = nullptr);
};
