#pragma once

#include <DirectXMath.h>
#include <assert.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <math.h>
#include <minwindef.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>
#include <winuser.h>

#include <algorithm>
#include <chrono>
#include <iostream>

#include "point.h"
#include "primitive.h"
#include "utils.h"

using namespace DirectX::SimpleMath;

class Surface : public GeometricPrimitive {
  struct Vertex {
    Point3f pos;
    Point3f norm;
  };

  struct GeomBuffer {
    DirectX::XMMATRIX m;
    DirectX::XMMATRIX norm;
    Point4f color;
  };

 public:
  Surface(std::shared_ptr<DXController>& pDXController,
          ID3D11ShaderResourceView* pCubemapView)

      : GeometricPrimitive(pDXController),
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
