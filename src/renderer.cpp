#include "renderer.h"
#include "point.h"
#include "utils.h"

#define _USE_MATH_DEFINES

#include <DirectXMath.h>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <iostream>
#include <math.h>
#include <minwindef.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>
#include <winuser.h>

struct Vertex {
  float x, y, z;
  COLORREF color;
};

struct GeomBuffer {
  DirectX::XMMATRIX m;
};

struct SceneBuffer {
  DirectX::XMMATRIX vp;
};

static const float CameraRotationSpeed = (float)M_PI * 2.0f;
static const float ModelRotationSpeed = (float)M_PI / 15.0f;

bool Renderer::Init(HWND hWnd) {
  HRESULT result;

  // Create a DirectX graphics interface factory.
  IDXGIFactory *pFactory = nullptr;
  result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&pFactory);

  // Select hardware adapter
  IDXGIAdapter *pSelectedAdapter = nullptr;
  if (SUCCEEDED(result)) {
    IDXGIAdapter *pAdapter = nullptr;
    UINT adapterIdx = 0;
    while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter))) {
      DXGI_ADAPTER_DESC desc;
      result = pAdapter->GetDesc(&desc);
      assert(SUCCEEDED(result));

      if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0) {
        pSelectedAdapter = pAdapter;
        break;
      }

      pAdapter->Release();

      adapterIdx++;
    }
  }
  assert(SUCCEEDED(result));

  // Create DirectX 11 device
  D3D_FEATURE_LEVEL level;
  D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
  if (SUCCEEDED(result)) {
    UINT flags = 0;
#ifndef NDEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    result =
        D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
                          flags, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
                          &m_pDevice, &level, &m_pDeviceContext);
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
    assert(SUCCEEDED(result));
  }

  if (SUCCEEDED(result)) {
    result = SetupBackBuffer();
  }

  if (SUCCEEDED(result)) {
    result = InitScene();
  }

  // Initial camera setup
  if (SUCCEEDED(result)) {
    m_camera.poi = Point3f{0, 0, 0};
    m_camera.r = 5.0f;
    m_camera.phi = -(float)M_PI / 4;
    m_camera.theta = (float)M_PI / 4;
  }

  SAFE_RELEASE(pSelectedAdapter);
  SAFE_RELEASE(pFactory);

  if (FAILED(result)) {
    Term();
  }

  return SUCCEEDED(result);
}

bool Renderer::Update() {
  size_t usec = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
  if (usec == 0) {
    m_prevUSec = usec;
  }

  if (m_rotateModel) {
    double deltaSec = (usec - m_prevUSec) / 100000.0;
    m_angle = m_angle + deltaSec * ModelRotationSpeed;

    GeomBuffer geomBuffer;
    geomBuffer.m = DirectX::XMMatrixRotationAxis(
        DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), -(float)m_angle);

    m_pDeviceContext->UpdateSubresource(m_pGeomBuffer, 0, nullptr, &geomBuffer,
                                        0, 0);
  }

  m_prevUSec = usec;

  // Setup camera
  DirectX::XMMATRIX v;
  {
    Point3f pos =
        m_camera.poi + Point3f{cosf(m_camera.theta) * cosf(m_camera.phi),
                               sinf(m_camera.theta),
                               cosf(m_camera.theta) * sinf(m_camera.phi)} *
                           m_camera.r;
    float upTheta = m_camera.theta + (float)M_PI / 2;
    Point3f up = Point3f{cosf(upTheta) * cosf(m_camera.phi), sinf(upTheta),
                         cosf(upTheta) * sinf(m_camera.phi)};

    v = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.0f),
        DirectX::XMVectorSet(m_camera.poi.x, m_camera.poi.y, m_camera.poi.z,
                             0.0f),
        DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f));
  }

  float f = 100.0f;
  float n = 0.1f;
  float fov = (float)M_PI / 3;
  float c = 1.0f / tanf(fov / 2);
  float aspectRatio = (float)m_height / m_width;
  DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveLH(
      tanf(fov / 2) * 2 * n, tanf(fov / 2) * 2 * n * aspectRatio, n, f);

  D3D11_MAPPED_SUBRESOURCE subresource;
  HRESULT result = m_pDeviceContext->Map(
      m_pSceneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
  assert(SUCCEEDED(result));
  if (SUCCEEDED(result)) {
    SceneBuffer &sceneBuffer =
        *reinterpret_cast<SceneBuffer *>(subresource.pData);

    sceneBuffer.vp = DirectX::XMMatrixMultiply(v, p);

    m_pDeviceContext->Unmap(m_pSceneBuffer, 0);
  }

  return SUCCEEDED(result);
}

bool Renderer::Render() {
  assert(m_pDeviceContext != nullptr);
  m_pDeviceContext->ClearState();

  ID3D11RenderTargetView *views[] = {m_pBackBufferRTV};
  m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);

  static const FLOAT BackColor[4] = {0.25f, 0.25f, 0.25f, 1.0f};
  m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);

  D3D11_VIEWPORT viewport;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = (FLOAT)m_width;
  viewport.Height = (FLOAT)m_height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  m_pDeviceContext->RSSetViewports(1, &viewport);

  D3D11_RECT rect;
  rect.left = 0;
  rect.top = 0;
  rect.right = m_width;
  rect.bottom = m_height;
  m_pDeviceContext->RSSetScissorRects(1, &rect);

  m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer};
  UINT strides[] = {16};
  UINT offsets[] = {0};
  ID3D11Buffer *cbuffers[] = {m_pGeomBuffer, m_pSceneBuffer};
  m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
  m_pDeviceContext->IASetInputLayout(m_pInputLayout);
  m_pDeviceContext->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
  m_pDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
  m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
  m_pDeviceContext->DrawIndexed(36, 0, 0);

  HRESULT result = m_pSwapChain->Present(0, 0);
  assert(SUCCEEDED(result));

  return SUCCEEDED(result);
}

bool Renderer::Resize(UINT width, UINT height) {
  if (width != m_width || height != m_height) {
    SAFE_RELEASE(m_pBackBufferRTV);

    HRESULT result = m_pSwapChain->ResizeBuffers(2, width, height,
                                                 DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      m_width = width;
      m_height = height;

      result = SetupBackBuffer();
    }

    return SUCCEEDED(result);
  }

  return true;
}

HRESULT Renderer::SetupBackBuffer() {
  ID3D11Texture2D *pBackBuffer = NULL;
  HRESULT result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                           (LPVOID *)&pBackBuffer);
  assert(SUCCEEDED(result));
  if (SUCCEEDED(result)) {
    result =
        m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &m_pBackBufferRTV);
    assert(SUCCEEDED(result));

    SAFE_RELEASE(pBackBuffer);
  }

  return result;
}

HRESULT Renderer::InitScene() {
  static const Vertex Vertices[] = {
      {-0.5f, 0.5f, 0.0f, RGB(0, 0, 255)},   // vertex 0
      {0.5f, 0.5f, 0.0f, RGB(0, 255, 0)},    // vertex 1
      {-0.5f, -0.5f, 0.0f, RGB(255, 0, 0)},  // 2
      {0.5f, -0.5f, 0.0f, RGB(0, 255, 255)}, // 3
      {-0.5f, 0.5f, 1.0f, RGB(0, 0, 255)},   // ...
      {0.5f, 0.5f, 1.0f, RGB(255, 0, 0)},
      {-0.5f, -0.5f, 1.0f, RGB(0, 255, 0)},
      {0.5f, -0.5f, 1.0f, RGB(0, 255, 255)},
  };

  static const USHORT Indices[] = {
      0, 1, 2, 2, 1, 3, // front
      4, 0, 6, 6, 0, 2, // left
      4, 6, 5, 5, 6, 7, // back
      3, 1, 7, 7, 1, 5, // right
      4, 5, 0, 0, 5, 1, // top
      3, 7, 2, 2, 7, 6, // bot
  };

  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12,
       D3D11_INPUT_PER_VERTEX_DATA, 0}};

  HRESULT result = S_OK;

  // Create vertex buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(Vertices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &Vertices;
    data.SysMemPitch = sizeof(Vertices);
    data.SysMemSlicePitch = 0;

    result = m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pVertexBuffer, "VertexBuffer");
    }
  }

  // Create index buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(Indices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &Indices;
    data.SysMemPitch = sizeof(Indices);
    data.SysMemSlicePitch = 0;

    result = m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pIndexBuffer, "IndexBuffer");
    }
  }

  ID3DBlob *pVertexShaderCode = nullptr;
  if (SUCCEEDED(result)) {
    result = CompileAndCreateShader(L"../shaders/SimpleColor.vs",
                                    (ID3D11DeviceChild **)&m_pVertexShader,
                                    &pVertexShaderCode);
  }
  if (SUCCEEDED(result)) {
    result = CompileAndCreateShader(L"../shaders/SimpleColor.ps",
                                    (ID3D11DeviceChild **)&m_pPixelShader);
  }

  if (SUCCEEDED(result)) {
    result = m_pDevice->CreateInputLayout(
        InputDesc, 2, pVertexShaderCode->GetBufferPointer(),
        pVertexShaderCode->GetBufferSize(), &m_pInputLayout);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pInputLayout, "InputLayout");
    }
  }

  SAFE_RELEASE(pVertexShaderCode);

  // Create Geometry buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(GeomBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    GeomBuffer geomBuffer;
    geomBuffer.m = DirectX::XMMatrixIdentity();

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &data;
    data.SysMemPitch = sizeof(geomBuffer);
    data.SysMemSlicePitch = 0;

    result = m_pDevice->CreateBuffer(&desc, &data, &m_pGeomBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pGeomBuffer, "GeomBuffer");
    }
  }

  // Create Scene buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(SceneBuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.StructureByteStride = 0;
    desc.MiscFlags = 0;

    SceneBuffer sceneBuffer;
    sceneBuffer.vp = DirectX::XMMatrixIdentity();

    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &sceneBuffer;
    data.SysMemPitch = sizeof(sceneBuffer);
    data.SysMemSlicePitch = 0;

    result = m_pDevice->CreateBuffer(&desc, &data, &m_pSceneBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pSceneBuffer, "SceneBuffer");
    }
  }

  return result;
}

void Renderer::TermScene() {
  SAFE_RELEASE(m_pInputLayout);
  SAFE_RELEASE(m_pPixelShader);
  SAFE_RELEASE(m_pVertexShader);

  SAFE_RELEASE(m_pIndexBuffer);
  SAFE_RELEASE(m_pVertexBuffer);
  SAFE_RELEASE(m_pSceneBuffer);
  SAFE_RELEASE(m_pGeomBuffer);
}

void Renderer::Term() {
  TermScene();

  SAFE_RELEASE(m_pBackBufferRTV);
  SAFE_RELEASE(m_pSwapChain);
  SAFE_RELEASE(m_pDeviceContext);

#ifndef NDEBUG
  if (m_pDevice != nullptr) {
    ID3D11Debug *pDebug = nullptr;
    HRESULT result =
        m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void **)&pDebug);
    assert(SUCCEEDED(result));
    if (pDebug != nullptr) {
      if (pDebug->AddRef() !=
          3) // ID3D11Device && ID3D11Debug && after AddRef()
      {
        pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL |
                                        D3D11_RLDO_IGNORE_INTERNAL);
      }
      pDebug->Release();

      SAFE_RELEASE(pDebug);
    }
  }
#endif

  SAFE_RELEASE(m_pDevice);
}

void Renderer::MouseRBPressed(bool pressed, int x, int y) {
  m_rbPressed = pressed;
  if (m_rbPressed) {
    m_prevMouseX = x;
    m_prevMouseY = y;
  }
}

void Renderer::MouseMoved(int x, int y) {
  if (m_rbPressed) {
    float dx = -(float)(x - m_prevMouseX) / m_width * CameraRotationSpeed;
    float dy = (float)(y - m_prevMouseY) / m_width * CameraRotationSpeed;

    m_camera.phi += dx;
    m_camera.theta += dy;
    m_camera.theta =
        min(max(m_camera.theta, -(float)M_PI / 2), (float)M_PI / 2);

    m_prevMouseX = x;
    m_prevMouseY = y;
  }
}

void Renderer::MouseWheel(int delta) {
  m_camera.r -= delta / 100.0f;
  if (m_camera.r < 1.0f) {
    m_camera.r = 1.0f;
  }
}

void Renderer::KeyPressed(int keyCode) {
  if (keyCode == ' ') {
    m_rotateModel = !m_rotateModel;
  }
}

HRESULT Renderer::CompileAndCreateShader(const std::wstring &path,
                                         ID3D11DeviceChild **ppShader,
                                         ID3DBlob **ppCode) {
  // Try to load shader's source code first
  FILE *pFile = nullptr;
  _wfopen_s(&pFile, path.c_str(), L"rb");
  assert(pFile != nullptr);
  if (pFile == nullptr) {
    return E_FAIL;
  }

  fseek(pFile, 0, SEEK_END);
  long long size = _ftelli64(pFile);
  fseek(pFile, 0, SEEK_SET);

  std::vector<char> data;
  data.resize(size + 1);

  size_t rd = fread(data.data(), 1, size, pFile);
  assert(rd == (size_t)size);

  fclose(pFile);

  // Determine shader's type
  std::wstring ext = Extension(path);

  std::string entryPoint = "";
  std::string platform = "";

  if (ext == L"vs") {
    entryPoint = "vs";
    platform = "vs_5_0";
  } else if (ext == L"ps") {
    entryPoint = "ps";
    platform = "ps_5_0";
  }

  // Setup flags
  UINT flags1 = 0;
#ifndef NDEBUG
  flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  // Try to compile
  ID3DBlob *pCode = nullptr;
  ID3DBlob *pErrMsg = nullptr;
  HRESULT result = D3DCompile(data.data(), data.size(), WCSToMBS(path).c_str(),
                              nullptr, nullptr, entryPoint.c_str(),
                              platform.c_str(), flags1, 0, &pCode, &pErrMsg);
  if (!SUCCEEDED(result) && pErrMsg != nullptr) {
    OutputDebugStringA((const char *)pErrMsg->GetBufferPointer());
  }
  assert(SUCCEEDED(result));
  SAFE_RELEASE(pErrMsg);

  // Create shader itself if anything else is OK
  if (SUCCEEDED(result)) {
    if (ext == L"vs") {
      ID3D11VertexShader *pVertexShader = nullptr;
      result = m_pDevice->CreateVertexShader(pCode->GetBufferPointer(),
                                             pCode->GetBufferSize(), nullptr,
                                             &pVertexShader);
      if (SUCCEEDED(result)) {
        *ppShader = pVertexShader;
      }
    } else if (ext == L"ps") {
      ID3D11PixelShader *pPixelShader = nullptr;
      result = m_pDevice->CreatePixelShader(pCode->GetBufferPointer(),
                                            pCode->GetBufferSize(), nullptr,
                                            &pPixelShader);
      if (SUCCEEDED(result)) {
        *ppShader = pPixelShader;
      }
    }
  }
  if (SUCCEEDED(result)) {
    result = SetResourceName(*ppShader, WCSToMBS(path).c_str());
  }

  if (ppCode) {
    *ppCode = pCode;
  } else {
    SAFE_RELEASE(pCode);
  }

  return result;
}
