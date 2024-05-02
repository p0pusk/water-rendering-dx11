#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <dxgi.h>
#include <minwindef.h>
#include <winnt.h>

#include <memory>
#include <string>
#include <vector>

#include "cubemap.h"
#include "device-resources.h"
#include "simulationRenderer.h"
#include "surface.h"
#include "water.h"

class Renderer {
  static const double PanSpeed;

public:
  Renderer()
      : m_pSceneBuffer(nullptr), m_rbPressed(false), m_prevMouseX(0),
        m_prevMouseY(0), m_rotateModel(false), m_angle(0.0),
        m_forwardDelta(0.0), m_rightDelta(0.0), m_showLightBulbs(true),
        m_useNormalMaps(true), m_showNormals(false), m_prevUSec(0),
        m_prevSimUSec(0) {}

  bool Init(HWND hWnd);
  void Term();

  bool Update();
  bool Render();
  bool Resize(UINT width, UINT height);

  void MouseRBPressed(bool pressed, int x, int y);
  void MouseMoved(int x, int y);
  void MouseWheel(int delta);
  void KeyPressed(int keyCode);
  void KeyReleased(int keyCode);

private:
  struct Camera {
    Vector3 poi; // Point of interest
    float r;     // Distance to POI
    float phi;   // Angle in plane x0z
    float theta; // Angle from plane x0z

    void GetDirections(Vector3 &forward, Vector3 &right);
  };

  struct Light {
    Vector4 pos = Vector4{0, 0, 0, 0};
    Vector4 color = Vector4{1, 1, 1, 0};
  };

  struct SceneBuffer {
    DirectX::XMMATRIX vp;
    Vector4 cameraPos;
    XMINT4 lightCount; // x - light count (max 10)
    Light lights[10];
    Vector4 ambientColor;
  };

  struct BoundingRect {
    Vector3 v[4];
  };

private:
  void ImGuiRender();
  HRESULT InitScene();

private:
  std::unique_ptr<Surface> m_pSurface;
  std::unique_ptr<Water> m_pWater;
  std::unique_ptr<SimRenderer> m_pSimulationRenderer;
  std::unique_ptr<CubeMap> m_pCubeMap;

  SceneBuffer m_sceneBuffer;
  ComPtr<ID3D11Buffer> m_pSceneBuffer;

  UINT m_width;
  UINT m_height;

  Camera m_camera;
  bool m_rbPressed;
  int m_prevMouseX;
  int m_prevMouseY;
  bool m_rotateModel;
  double m_angle;
  double m_forwardDelta;
  double m_rightDelta;

  bool m_showLightBulbs;
  bool m_useNormalMaps;
  bool m_showNormals;

  size_t m_prevUSec;
  size_t m_prevSimUSec;
};
