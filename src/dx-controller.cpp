#include "dx-controller.h"

HRESULT DXController::Init(const HWND& hWnd) {
  HRESULT result;

  // Create a DirectX graphics interface factory.
  IDXGIFactory* pFactory = nullptr;
  result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);

  // Select hardware adapter
  IDXGIAdapter* pSelectedAdapter = nullptr;
  if (SUCCEEDED(result)) {
    IDXGIAdapter* pAdapter = nullptr;
    UINT adapterIdx = 0;
    while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter))) {
      DXGI_ADAPTER_DESC desc;
      pAdapter->GetDesc(&desc);

      if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0) {
        pSelectedAdapter = pAdapter;
        break;
      }

      pAdapter->Release();

      adapterIdx++;
    }
  }
  if (FAILED(result)) {
    throw std::runtime_error("Failed to select hardware adapter");
  }

  // Create DirectX 11 device
  D3D_FEATURE_LEVEL level;
  D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
  if (SUCCEEDED(result)) {
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif  // _DEBUG
    result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
                               flags, levels, 1, D3D11_SDK_VERSION, &m_pDevice,
                               &level, &m_pDeviceContext);
    assert(level == D3D_FEATURE_LEVEL_11_0);
    assert(SUCCEEDED(result));
  }

  // Create swapchain
  if (SUCCEEDED(result)) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    m_width = rc.right - rc.left;
    m_height = rc.bottom - rc.top;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {0};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = m_width;
    swapChainDesc.BufferDesc.Height = m_height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = true;
    swapChainDesc.BufferDesc.ScanlineOrdering =
        DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    result =
        pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
  }

  if (FAILED(result)) {
    throw std::runtime_error("Failed to create swapchain");
  }

  result = SetupBackBuffer();

  // CCW culling rasterizer state
  if (SUCCEEDED(result)) {
    D3D11_RASTERIZER_DESC desc = {};
    desc.AntialiasedLineEnable = FALSE;
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthBias = 0;
    desc.SlopeScaledDepthBias = 0.0f;
    desc.DepthBiasClamp = 0.0f;
    desc.DepthClipEnable = TRUE;
    desc.ScissorEnable = FALSE;
    desc.MultisampleEnable = FALSE;

    result = m_pDevice->CreateRasterizerState(&desc, &m_pRasterizerState);

    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pRasterizerState, "RasterizerState");
    }
  }

  // Create blend states
  if (SUCCEEDED(result)) {
    D3D11_BLEND_DESC desc = {};
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].RenderTargetWriteMask =
        D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN |
        D3D11_COLOR_WRITE_ENABLE_BLUE;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;

    result = m_pDevice->CreateBlendState(&desc, &m_pTransBlendState);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pTransBlendState, "TransBlendState");
    }
    if (SUCCEEDED(result)) {
      desc.RenderTarget[0].BlendEnable = FALSE;
      result = m_pDevice->CreateBlendState(&desc, &m_pOpaqueBlendState);
    }
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pOpaqueBlendState, "OpaqueBlendState");
    }
  }

  // Create reverse depth state
  if (SUCCEEDED(result)) {
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    desc.StencilEnable = FALSE;

    result = m_pDevice->CreateDepthStencilState(&desc, &m_pDepthState);

    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pDepthState, "DepthState");
    }
  }

  // Create reverse transparent depth state
  if (SUCCEEDED(result)) {
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_GREATER;
    desc.StencilEnable = FALSE;

    result = m_pDevice->CreateDepthStencilState(&desc, &m_pTransDepthState);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pTransDepthState, "TransDepthState");
    }
  }

  if (SUCCEEDED(result)) {
    D3D11_SAMPLER_DESC desc = {};

    // desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    // desc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    desc.Filter = D3D11_FILTER_ANISOTROPIC;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.MinLOD = -FLT_MAX;
    desc.MaxLOD = FLT_MAX;
    desc.MipLODBias = 0.0f;
    desc.MaxAnisotropy = 16;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] =
        desc.BorderColor[3] = 1.0f;

    result = m_pDevice->CreateSamplerState(&desc, &m_pSampler);
  }

  SAFE_RELEASE(pSelectedAdapter);
  SAFE_RELEASE(pFactory);

  assert(SUCCEEDED(result));

  return SUCCEEDED(result);
}

bool DXController::Resize(UINT w, UINT h) {
  if (w != m_width || h != m_height) {
    SAFE_RELEASE(m_pBackBufferRTV);
    SAFE_RELEASE(m_pDepthBuffer);
    SAFE_RELEASE(m_pDepthBufferDSV);

    HRESULT result =
        m_pSwapChain->ResizeBuffers(2, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      m_width = w;
      m_height = h;

      result = SetupBackBuffer();
    }
    return SUCCEEDED(result);
  }

  return true;
}

HRESULT DXController::SetupBackBuffer() {
  ID3D11Texture2D* pBackBuffer = nullptr;
  HRESULT result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                           (LPVOID*)&pBackBuffer);
  if (SUCCEEDED(result)) {
    result =
        m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferRTV);
  }

  SAFE_RELEASE(pBackBuffer);

  if (SUCCEEDED(result)) {
    D3D11_TEXTURE2D_DESC desc;
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Height = m_height;
    desc.Width = m_width;
    desc.MipLevels = 1;

    result = m_pDevice->CreateTexture2D(&desc, nullptr, &m_pDepthBuffer);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pDepthBuffer, "DepthBuffer");
    }
  }

  if (SUCCEEDED(result)) {
    result = m_pDevice->CreateDepthStencilView(m_pDepthBuffer, nullptr,
                                               &m_pDepthBufferDSV);

    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pDepthBuffer, "DepthBufferView");
    }
  }

  if (FAILED(result)) {
    throw std::runtime_error("Failed to create backbuffer");
  }

  return result;
}
