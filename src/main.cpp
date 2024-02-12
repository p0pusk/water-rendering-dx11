#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE

#include <assert.h>
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <windowsx.h>

#include <iostream>

#include "renderer.h"

HINSTANCE hInst;
Renderer *pRenderer;

bool PressedKeys[0xff] = {};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  LRESULT result = 0;
  switch (msg) {
    case WM_RBUTTONDOWN:
      if (pRenderer != nullptr) {
        pRenderer->MouseRBPressed(true, GET_X_LPARAM(lparam),
                                  GET_Y_LPARAM(lparam));
      }
      break;

    case WM_RBUTTONUP:
      if (pRenderer != nullptr) {
        pRenderer->MouseRBPressed(false, GET_X_LPARAM(lparam),
                                  GET_Y_LPARAM(lparam));
      }
      break;

    case WM_MOUSEMOVE:
      if (pRenderer != nullptr) {
        pRenderer->MouseMoved(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
      }
      break;

    case WM_MOUSEWHEEL:
      if (pRenderer != nullptr) {
        pRenderer->MouseWheel(GET_WHEEL_DELTA_WPARAM(wparam));
      }
      break;

    case WM_KEYDOWN:
      if (wparam == VK_ESCAPE)
        DestroyWindow(hwnd);
      else if (!PressedKeys[wparam]) {
        if (pRenderer != nullptr) {
          pRenderer->KeyPressed((int)wparam);
        }
        PressedKeys[wparam] = true;
      }
      break;

    case WM_KEYUP:
      PressedKeys[wparam] = false;
      if (pRenderer != nullptr) {
        pRenderer->KeyReleased((int)wparam);
      }
      break;
    case WM_DESTROY: {
      PostQuitMessage(0);
      break;
    }
    case WM_SIZE: {
      if (pRenderer != nullptr) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        pRenderer->Resize(rc.right - rc.left, rc.bottom - rc.top);
      }
      break;
    }
    default:
      result = DefWindowProcW(hwnd, msg, wparam, lparam);
  }
  return result;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd) {

  // Fix working folder
  std::wstring dir;
  dir.resize(MAX_PATH + 1);
  GetCurrentDirectory(MAX_PATH + 1, &dir[0]);
  size_t configPos = dir.find(L"bin");
  if (configPos != std::wstring::npos) {
    dir.resize(configPos);
    SetCurrentDirectory(dir.c_str());
  }

  // Open a window
  HWND hwnd;
  {
    WNDCLASSEXW winClass = {};
    winClass.cbSize = sizeof(WNDCLASSEXW);
    winClass.style = CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc = &WndProc;
    winClass.hInstance = hInstance;
    winClass.hIcon = LoadIconW(0, IDI_APPLICATION);
    winClass.hCursor = LoadCursorW(0, IDC_ARROW);
    winClass.lpszClassName = L"MyWindowClass";
    winClass.hIconSm = LoadIconW(0, IDI_APPLICATION);

    if (!RegisterClassExW(&winClass)) {
      MessageBoxA(0, "RegisterClassEx failed", "Fatal Error", MB_OK);
      return GetLastError();
    }

    RECT initialRect = {0, 0, 1024, 768};
    AdjustWindowRectEx(&initialRect, WS_OVERLAPPEDWINDOW, FALSE,
                       WS_EX_OVERLAPPEDWINDOW);
    LONG initialWidth = initialRect.right - initialRect.left;
    LONG initialHeight = initialRect.bottom - initialRect.top;

    hwnd = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, winClass.lpszClassName,
                           L"water", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, initialWidth,
                           initialHeight, 0, 0, hInstance, 0);

    if (!hwnd) {
      MessageBoxA(0, "CreateWindowEx failed", "Fatal Error", MB_OK);
      return GetLastError();
    }
  }

  pRenderer = new Renderer();
  if (!pRenderer->Init(hwnd)) {
    delete pRenderer;
    return false;
  }

  // Main Loop
  bool isRunning = true;
  while (isRunning) {
    MSG msg = {};
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) isRunning = false;
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    if (pRenderer->Update()) {
      pRenderer->Render();
    }
  }
  pRenderer->Term();
  delete pRenderer;
  return 0;
}
