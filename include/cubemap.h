#pragma once

#include <DirectXMath.h>

#include "device-resources.h"
#include "pch.h"

class CubeMap {
public:
  struct SphereGeomBuffer {
    DirectX::XMMATRIX m;
    float size;
  };

  CubeMap() { m_sphereIndexCount = 0; }

  HRESULT Init();
  void Render(ID3D11Buffer *pSceneBuffer = nullptr);

  void Resize(UINT width, UINT height);

  ComPtr<ID3D11ShaderResourceView> m_pCubemapView;

private:
  int m_sphereIndexCount;

  ComPtr<ID3D11Buffer> m_pGeomBuffer;
  ComPtr<ID3D11Buffer> m_pVertexBuffer;
  ComPtr<ID3D11Buffer> m_pIndexBuffer;
  ComPtr<ID3D11VertexShader> m_pVertexShader;
  ComPtr<ID3D11PixelShader> m_pPixelShader;
  ComPtr<ID3D11InputLayout> m_pInputLayout;

  HRESULT InitCubemap();
};
