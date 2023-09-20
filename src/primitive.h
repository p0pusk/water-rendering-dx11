#pragma once

#include "point.h"
#include "utils.h"

#include <DirectXMath.h>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <iostream>
#include <math.h>
#include <minwindef.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>
#include <winuser.h>

class Primitive {
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
  Primitive(ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pSceneBuffer, ID3D11SamplerState * pSampler, ID3D11ShaderResourceView* pCubemapView)
    :m_pDevice(pDevice),
    m_pDeviceContext(pDeviceContext),
    m_pSceneBuffer(pSceneBuffer),
    m_pSampler(pSampler),
    m_pCubemapView(pCubemapView)

  { }

  HRESULT Init(Point3f &&pos);
  void Render();

private:
  HRESULT CompileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, const std::vector<std::string>& defines, ID3DBlob** ppCode);

  ID3D11Device* m_pDevice;
  ID3D11DeviceContext* m_pDeviceContext;

  ID3D11Buffer* m_pVertexBuffer;
  ID3D11Buffer* m_pIndexBuffer;

  ID3D11VertexShader* m_pVertexShader;
  ID3D11PixelShader* m_pPixelShader;
  ID3D11InputLayout* m_pInputLayout;

  ID3D11Buffer* m_pSceneBuffer;
  ID3D11Buffer* m_pGeomBuffer;

  ID3D11BlendState* m_pBlendState;
  ID3D11DepthStencilState* m_pDepthState;

  ID3D11ShaderResourceView* m_pCubemapView;
  ID3D11SamplerState* m_pSampler;
};
