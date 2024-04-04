#include "renderer.h"

#include <exception>
#include <memory>

#include "DDS.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "pch.h"
#include "simulationRenderer.h"

#define _USE_MATH_DEFINES

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

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

Settings g_settings;

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
  m_pDeviceResources = std::make_shared<DX::DeviceResources>();

  try {
    result = m_pDeviceResources->Init(hWnd);
  } catch (std::exception e) {
    std::cerr << e.what() << std::endl;
    exit(1);
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
    m_camera.r = 10.f;
    m_camera.phi = -(float)M_PI / 3;
    m_camera.theta = (float)M_PI / 8;
  }

  // Setup Dear ImGui context
  if (SUCCEEDED(result)) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(m_pDeviceResources->GetDevice(),
                        m_pDeviceResources->GetDeviceContext());
  }

  if (FAILED(result)) {
    Term();
  }

  return SUCCEEDED(result);
}

HRESULT Renderer::InitScene() {
  // Create scene buffer
  HRESULT result = S_OK;

  result = m_pDeviceResources->CreateConstantBuffer<SceneBuffer>(
      D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, nullptr, "SceneBuffer",
      &m_pSceneBuffer);

  if (SUCCEEDED(result)) {
    m_pCubeMap = std::unique_ptr<CubeMap>(new CubeMap(m_pDeviceResources));
    result = m_pCubeMap->Init();
  }

  if (SUCCEEDED(result)) {
    m_pSurface = new Surface(m_pDeviceResources, m_pCubeMap->m_pCubemapView);
    result = m_pSurface->Init();
  }

  if (SUCCEEDED(result)) {
    // m_pWater = new Water(m_pDeviceResources);
    // result = m_pWater->Init(10);
  }

  try {
    m_pSimulationRenderer =
        std::make_unique<SimRenderer>(m_pDeviceResources, g_settings);
    m_pSimulationRenderer->Init();
  } catch (std::exception& e) {
    std::cout << e.what();
    exit(1);
  }

  assert(SUCCEEDED(result));

  return result;
}
void Renderer::Term() {
  TermScene();
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

bool Renderer::Update() {
  size_t usec = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();

  if (m_prevUSec == 0) {
    m_prevUSec = usec;  // Initial update
  }

  double deltaSec = (usec - m_prevUSec) / 1000000.0;

  // m_pWater->Update(deltaSec);
  m_pSimulationRenderer->Update(1.f / 120.f);

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
  HRESULT result = m_pDeviceResources->GetDeviceContext()->Map(
      m_pSceneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
  assert(SUCCEEDED(result));

  if (SUCCEEDED(result)) {
    m_sceneBuffer.vp = DirectX::XMMatrixMultiply(v, p);
    m_sceneBuffer.cameraPos = cameraPos;

    memcpy(subresource.pData, &m_sceneBuffer, sizeof(SceneBuffer));

    m_pDeviceResources->GetDeviceContext()->Unmap(m_pSceneBuffer, 0);
  }

  return SUCCEEDED(result);
}

void Renderer::ImGuiRender() {
  ImGui::Begin("test");
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::End();
}

bool Renderer::Render() {
  auto deviceContext = m_pDeviceResources->GetDeviceContext();
  deviceContext->ClearState();

  ID3D11RenderTargetView* views[] = {m_pDeviceResources->GetBackBufferRTV()};
  deviceContext->OMSetRenderTargets(1, views,
                                    m_pDeviceResources->GetDepthBufferDSV());

  static const FLOAT BackColor[4] = {0.25f, 0.25f, 0.55f, 1.0f};

  deviceContext->ClearRenderTargetView(m_pDeviceResources->GetBackBufferRTV(),
                                       BackColor);
  deviceContext->ClearDepthStencilView(m_pDeviceResources->GetDepthBufferDSV(),
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

  deviceContext->RSSetState(m_pDeviceResources->GetRasterizerState());

  // m_pCubeMap->Render(m_pSceneBuffer);
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  ImGuiRender();
  try {
    m_pSurface->Render(m_pSceneBuffer);
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }
  // m_pWater->Render(m_pSceneBuffer);
  m_pSimulationRenderer->Render(m_pSceneBuffer);

  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  // Rendering
  HRESULT result = m_pDeviceResources->GetSwapChain()->Present(0, 0);
  assert(SUCCEEDED(result));

  return SUCCEEDED(result);
}

bool Renderer::Resize(UINT width, UINT height) {
  if (m_width != width || height != m_height) {
    m_width = width;
    m_height = height;

    bool result = m_pDeviceResources->Resize(width, height);
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
        std::min(std::max(m_camera.theta, -(float)M_PI / 2), (float)M_PI / 2);

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
      g_settings.marching = !g_settings.marching;
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
