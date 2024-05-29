#pragma once

#include "CommonStates.h"
#include "device-resources.h"
#include "pch.h"

using namespace DirectX::SimpleMath;

class Surface {
  struct Vertex {
    Vector3 pos;
    Vector3 norm;
  };

  struct GeomBuffer {
    DirectX::XMMATRIX m;
    DirectX::XMMATRIX norm;
    Vector4 shine;
    Vector4 color;
  };

public:
  Surface(ID3D11ShaderResourceView *pCubemapView)
      : m_pCubemapView(pCubemapView) {}

  HRESULT Init(Vector3 pos);
  void Render(ID3D11Buffer *pSceneBuffer = nullptr);

private:
  ComPtr<ID3D11Buffer> m_pGeomBuffer;
  ComPtr<ID3D11Buffer> m_pVertexBuffer;
  ComPtr<ID3D11Buffer> m_pIndexBuffer;
  ComPtr<ID3D11InputLayout> m_pInputLayout;
  ComPtr<ID3D11VertexShader> m_pVertexShader;
  ComPtr<ID3D11PixelShader> m_pPixelShader;
  ComPtr<ID3D11ShaderResourceView> m_pCubemapView;
  std::unique_ptr<DirectX::CommonStates> m_states;
};
