#include "renderer.h"

#include "DDS.h"

#define _USE_MATH_DEFINES

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

struct TextureTangentVertex {
  Vector3 pos;
  DirectX::XMFLOAT2 tangent;
  Vector3 norm;
  Vector3 uv;
};

struct GeomBuffer {
  DirectX::XMMATRIX m;
  DirectX::XMMATRIX normalM;
  Vector4 shine;  // x - shininess
};

static const float CameraRotationSpeed = (float)M_PI * 2.0f;
static const float ModelRotationSpeed = (float)M_PI / 2.0f;

static const float Eps = 0.00001f;

void Renderer::Camera::GetDirections(Vector3& forward, Vector3& right) {
  Vector3 dir =
      -Vector3{cosf(theta) * cosf(phi), sinf(theta), cosf(theta) * sinf(phi)};
  float upTheta = theta + (float)M_PI / 2;
  Vector3 up = Vector3{cosf(upTheta) * cosf(phi), sinf(upTheta),
                       cosf(upTheta) * sinf(phi)};
  up.Cross(dir, right);
  right.y = 0.0f;
  right.Normalize();

  if (fabs(dir.x) > Eps || fabs(dir.z) > Eps) {
    forward = Vector3{dir.x, 0.0f, dir.z};
  } else {
    forward = Vector3{up.x, 0.0f, up.z};
  }
  forward.Normalize();
}

const double Renderer::PanSpeed = 2.0;

bool Renderer::Init(HWND hWnd) {
  HRESULT result;
  m_pDXController = std::shared_ptr<DXController>(new DXController());

  try {
    result = m_pDXController->Init(hWnd);
  } catch (std::runtime_error e) {
    assert(0);
  }

  if (SUCCEEDED(result)) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    m_width = rc.right - rc.left;
    m_height = rc.bottom - rc.top;

    result = InitScene();
  }

  // Initial camera setup
  if (SUCCEEDED(result)) {
    m_camera.poi = Vector3{0, 0, 0};
    m_camera.r = 5.0f;
    m_camera.phi = -(float)M_PI / 4;
    m_camera.theta = (float)M_PI / 4;
  }

  if (FAILED(result)) {
    Term();
  }

  return SUCCEEDED(result);
}

HRESULT Renderer::InitScene() {
  // Create scene buffer
  HRESULT result = S_OK;
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(SceneBuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    result = m_pDXController->m_pDevice->CreateBuffer(&desc, nullptr,
                                                      &m_pSceneBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pSceneBuffer, "SceneBuffer");
    }
  }

  if (SUCCEEDED(result)) {
    m_pCubeMap = std::unique_ptr<CubeMap>(new CubeMap(m_pDXController));
    result = m_pCubeMap->Init();
  }

  if (SUCCEEDED(result)) {
    m_pSurface = new Surface(m_pDXController, m_pCubeMap->m_pCubemapView);
    result = m_pSurface->Init();
  }

  if (SUCCEEDED(result)) {
    m_pWater = new Water(m_pDXController);
    result = m_pWater->Init(100);
  }

  try {
    SPH::Props props;
    // props.cubeNum = {10, 10, 10};
    m_pSph = new SPH(m_pDXController, props);
    m_pSph->Init();
  } catch (std::exception& e) {
    std::cout << e.what();
    assert(1);
  }

  assert(SUCCEEDED(result));

  return result;
}
void Renderer::Term() { TermScene(); }

bool Renderer::Update() {
  size_t usec = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();

  if (m_prevUSec == 0) {
    m_prevUSec = usec;  // Initial update
  }

  double deltaSec = (usec - m_prevUSec) / 1000000.0;

  // m_pWater->Update(deltaSec);
  m_pSph->Update(deltaSec / 40);

  // Move camera
  {
    Vector3 cf, cr;
    m_camera.GetDirections(cf, cr);
    Vector3 d = (cf * (float)m_forwardDelta + cr * (float)m_rightDelta) *
                (float)deltaSec;
    m_camera.poi = m_camera.poi + d;
  }

  m_prevUSec = usec;

  // Setup camera
  DirectX::XMMATRIX v;
  Vector4 cameraPos;
  {
    Vector3 pos =
        m_camera.poi + Vector3{cosf(m_camera.theta) * cosf(m_camera.phi),
                               sinf(m_camera.theta),
                               cosf(m_camera.theta) * sinf(m_camera.phi)} *
                           m_camera.r;
    float upTheta = m_camera.theta + (float)M_PI / 2;
    Vector3 up = Vector3{cosf(upTheta) * cosf(m_camera.phi), sinf(upTheta),
                         cosf(upTheta) * sinf(m_camera.phi)};

    v = DirectX::XMMatrixLookAtLH(
        DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.0f),
        DirectX::XMVectorSet(m_camera.poi.x, m_camera.poi.y, m_camera.poi.z,
                             0.0f),
        DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f));

    cameraPos = Vector4(pos);
  }

  float f = 100.0f;
  float n = 0.1f;
  float fov = (float)M_PI / 3;
  float c = 1.0f / tanf(fov / 2);
  float aspectRatio = (float)m_height / m_width;
  DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveLH(
      tanf(fov / 2) * 2 * f, tanf(fov / 2) * 2 * f * aspectRatio, f, n);

  D3D11_MAPPED_SUBRESOURCE subresource;
  HRESULT result = m_pDXController->m_pDeviceContext->Map(
      m_pSceneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
  assert(SUCCEEDED(result));

  if (SUCCEEDED(result)) {
    m_sceneBuffer.vp = DirectX::XMMatrixMultiply(v, p);
    m_sceneBuffer.cameraPos = cameraPos;

    memcpy(subresource.pData, &m_sceneBuffer, sizeof(SceneBuffer));

    m_pDXController->m_pDeviceContext->Unmap(m_pSceneBuffer, 0);
  }

  return SUCCEEDED(result);
}

bool Renderer::Render() {
  auto& deviceContext = m_pDXController->m_pDeviceContext;
  deviceContext->ClearState();

  ID3D11RenderTargetView* views[] = {m_pDXController->m_pBackBufferRTV};
  m_pDXController->m_pDeviceContext->OMSetRenderTargets(
      1, views, m_pDXController->m_pDepthBufferDSV);

  static const FLOAT BackColor[4] = {0.25f, 0.25f, 0.55f, 1.0f};

  deviceContext->ClearRenderTargetView(m_pDXController->m_pBackBufferRTV,
                                       BackColor);
  deviceContext->ClearDepthStencilView(m_pDXController->m_pDepthBufferDSV,
                                       D3D11_CLEAR_DEPTH, 0.0f, 0);

  D3D11_VIEWPORT viewport;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = (FLOAT)m_width;
  viewport.Height = (FLOAT)m_height;
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  deviceContext->RSSetViewports(1, &viewport);

  D3D11_RECT rect;
  rect.left = 0;
  rect.top = 0;
  rect.right = m_width;
  rect.bottom = m_height;
  deviceContext->RSSetScissorRects(1, &rect);

  m_pDXController->m_pDeviceContext->RSSetState(
      m_pDXController->m_pRasterizerState);

  // m_pCubeMap->Render(m_pSceneBuffer);
  m_pSurface->Render(m_pSceneBuffer);
  // m_pWater->Render(m_pSceneBuffer);
  m_pSph->Render(m_pSceneBuffer);

  // Rendering
  HRESULT result = m_pDXController->m_pSwapChain->Present(0, 0);
  assert(SUCCEEDED(result));

  return SUCCEEDED(result);
}

bool Renderer::Resize(UINT width, UINT height) {
  if (m_width != width || height != m_height) {
    m_width = width;
    m_height = height;

    bool result = m_pDXController->Resize(width, height);
    m_pCubeMap->Resize(width, height);

    return result;
  }
  return true;
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
  switch (keyCode) {
    case ' ':
      m_rotateModel = !m_rotateModel;
      break;

    case 'W':
    case 'w':
      m_forwardDelta += PanSpeed;
      break;

    case 'S':
    case 's':
      m_forwardDelta -= PanSpeed;
      break;

    case 'D':
    case 'd':
      m_rightDelta += PanSpeed;
      break;

    case 'A':
    case 'a':
      m_rightDelta -= PanSpeed;
      break;
    case 'I':
    case 'i':
      // m_pWater->StartImpulse(25, 25, 2, 2);
      m_pSph->isMarching = !m_pSph->isMarching;
      break;
  }
}

void Renderer::KeyReleased(int keyCode) {
  switch (keyCode) {
    case 'W':
    case 'w':
      m_forwardDelta -= PanSpeed;
      break;

    case 'S':
    case 's':
      m_forwardDelta += PanSpeed;
      break;

    case 'D':
    case 'd':
      m_rightDelta -= PanSpeed;
      break;

    case 'A':
    case 'a':
      m_rightDelta += PanSpeed;
      break;

    case 'I':
    case 'i':
      break;
  }
}

void Renderer::TermScene() { m_pCubeMap.release(); }
