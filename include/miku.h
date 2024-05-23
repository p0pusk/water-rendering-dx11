#pragma once

#include "CommonStates.h"
#include "DirectXMath.h"
#include "Effects.h"
#include "Model.h"
#include "VertexTypes.h"
#include "device-resources.h"

struct GeomBuffer {
  DirectX::XMMATRIX m;
  DirectX::XMMATRIX norm;
  Vector4 shine;
};

class Miku {
public:
  void Init();
  void Update();
  void Render(ID3D11Buffer *m_pSceneBuffer);

  ~Miku() {
    m_states.reset();
    m_model.reset();
    m_fxFactory.reset();
  }

private:
  SimpleMath::Matrix m_world;
  SimpleMath::Matrix m_view;
  SimpleMath::Matrix m_proj;

  ComPtr<ID3D11Buffer> m_pGeomBuffer;
  ComPtr<ID3D11VertexShader> m_vertexShader;
  ComPtr<ID3D11PixelShader> m_pixelShader;
  ComPtr<ID3D11ShaderResourceView> m_pMikuTexture;
  ComPtr<ID3D11InputLayout> m_pInputLayout;

  std::unique_ptr<DirectX::CommonStates> m_states;
  std::unique_ptr<DirectX::IEffectFactory> m_fxFactory;
  std::unique_ptr<DirectX::Model> m_model;
};
