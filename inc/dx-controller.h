#pragma once

#include <assert.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <minwindef.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>
#include <winuser.h>

#include <iostream>
#include <stdexcept>

#include "utils.h"

class DXController {
 public:
  DXController()
      : m_pDevice(nullptr),
        m_pDeviceContext(nullptr),
        m_pSwapChain(nullptr),
        m_pBackBufferRTV(nullptr),
        m_pDepthBuffer(nullptr),
        m_pDepthBufferDSV(nullptr),
        m_pDepthState(nullptr),
        m_pTransDepthState(nullptr),
        m_pRasterizerState(nullptr),
        m_pTransBlendState(nullptr),
        m_pOpaqueBlendState(nullptr),
        m_pSampler(nullptr),
        m_height(0),
        m_width(0) {}

  ~DXController() {
    SAFE_RELEASE(m_pBackBufferRTV);
    SAFE_RELEASE(m_pSampler);
    SAFE_RELEASE(m_pDepthBuffer);
    SAFE_RELEASE(m_pDepthBufferDSV);
    SAFE_RELEASE(m_pDepthState);
    SAFE_RELEASE(m_pTransDepthState);
    SAFE_RELEASE(m_pRasterizerState);
    SAFE_RELEASE(m_pTransBlendState);
    SAFE_RELEASE(m_pOpaqueBlendState);
    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pDeviceContext);
    SAFE_RELEASE(m_pDevice);
  }

  HRESULT Init(const HWND& hWnd);
  bool Resize(UINT w, UINT h);

  ID3D11Device* m_pDevice;
  ID3D11DeviceContext* m_pDeviceContext;
  IDXGISwapChain* m_pSwapChain;
  ID3D11RenderTargetView* m_pBackBufferRTV;

  ID3D11Texture2D* m_pDepthBuffer;
  ID3D11DepthStencilView* m_pDepthBufferDSV;

  ID3D11DepthStencilState* m_pDepthState;
  ID3D11DepthStencilState* m_pTransDepthState;

  ID3D11RasterizerState* m_pRasterizerState;

  ID3D11BlendState* m_pTransBlendState;
  ID3D11BlendState* m_pOpaqueBlendState;

  ID3D11SamplerState* m_pSampler;

 private:
  HRESULT SetupBackBuffer();

  UINT m_width;
  UINT m_height;
};