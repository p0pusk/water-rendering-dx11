//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <winsdkver.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <Windows.h>
#include <assert.h>
#include <d3d11_1.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <malloc.h>
#include <stdlib.h>
#include <tchar.h>
#include <wrl/client.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <format>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "BufferHelpers.h"
#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "GamePad.h"
#include "GeometricPrimitive.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PostProcess.h"
#include "PrimitiveBatch.h"
#include "ScreenGrab.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "WICTextureLoader.h"

#define M_PI std::acos(-1)

using namespace DirectX::SimpleMath;
using namespace DirectX;

namespace DX {
// Helper class for COM exceptions
class com_exception : public std::exception {
 public:
  com_exception(HRESULT hr, const std::string &message)
      : result(hr), mes(message) {}

  const char *what() const noexcept override {
    static char s_str[128] = {};
    sprintf_s(s_str, "EXCEPTION: Failure with HRESULT of %08X. %s",
              static_cast<unsigned int>(result), mes.c_str());
    return s_str;
  }

 private:
  HRESULT result;
  const std::string &mes;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr, const std::string &message = "") {
  if (FAILED(hr)) {
    throw com_exception(hr, message);
  }
}
}  // namespace DX

#define ASSERT_RETURN(expr, returnValue) \
  {                                      \
    bool value = (expr);                 \
    assert(value);                       \
    if (!value) {                        \
      return returnValue;                \
    }                                    \
  }

#define SAFE_RELEASE(p) \
  {                     \
    if (p != nullptr) { \
      p->Release();     \
      p = nullptr;      \
    }                   \
  }

inline HRESULT SetResourceName(ID3D11DeviceChild *pResource,
                               const std::string &name) {
  return pResource->SetPrivateData(WKPDID_D3DDebugObjectName,
                                   (UINT)name.length(), name.c_str());
}

inline std::wstring Extension(const std::wstring &filename) {
  size_t dotPos = filename.rfind(L'.');
  if (dotPos != std::wstring::npos) {
    return filename.substr(dotPos + 1);
  }
  return L"";
}

inline std::string WCSToMBS(const std::wstring &wstr) {
  size_t len = wstr.length();

  // We suppose that on Windows platform wchar_t is 2 byte length
  std::vector<char> res;
  res.resize(len + 1);

  size_t resLen = 0;
  wcstombs_s(&resLen, res.data(), res.size(), wstr.c_str(), len);

  return res.data();
}

template <typename T>
T DivUp(const T &a, const T &b) {
  return (a + b - (T)1) / b;
}

inline UINT32 GetBytesPerBlock(const DXGI_FORMAT &fmt) {
  switch (fmt) {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
      return 8;
      break;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return 16;
      break;
  }
  assert(0);
  return 0;
}
