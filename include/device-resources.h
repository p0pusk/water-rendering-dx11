#pragma once

#include <exception>

#include "pch.h"

using Microsoft::WRL::ComPtr;

class DeviceResources {
public:
  static DeviceResources &getInstance() {
    static DeviceResources instance;
    return instance;
  }

  HRESULT Init(const HWND &hWnd);
  bool Resize(UINT w, UINT h);

  ComPtr<ID3D11Device> m_pDevice;
  ComPtr<ID3D11DeviceContext> m_pDeviceContext;
  ComPtr<IDXGISwapChain> m_pSwapChain;
  ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV;
  ComPtr<ID3D11Texture2D> m_pDepthBuffer;
  ComPtr<ID3D11DepthStencilView> m_pDepthBufferDSV;
  ComPtr<ID3D11DepthStencilState> m_pDepthState;
  ComPtr<ID3D11DepthStencilState> m_pTransDepthState;
  ComPtr<ID3D11RasterizerState> m_pRasterizerState;
  ComPtr<ID3D11BlendState> m_pTransBlendState;
  ComPtr<ID3D11BlendState> m_pOpaqueBlendState;
  ComPtr<ID3D11SamplerState> m_pSampler;

private:
  DeviceResources(const HWND &hwnd) : m_width(0), m_height(0) { Init(hwnd); }

  DeviceResources() {}

  ComPtr<IDXGIDebug> m_debugDev;
  HRESULT SetupBackBuffer();

  UINT m_width;
  UINT m_height;
};
