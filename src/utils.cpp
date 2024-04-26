#include "utils.h"
#include "device-resources.h"

HRESULT CompileAndCreateShader(const std::wstring &path,
                               ID3D11DeviceChild **ppShader,
                               const std::vector<std::string> &defines,
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
  } else if (ext == L"cs") {
    entryPoint = "cs";
    platform = "cs_5_0";
  }

  // Setup flags
  UINT flags1 = 0;
#ifdef _DEBUG
  flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

  D3DInclude includeHandler;

  std::vector<D3D_SHADER_MACRO> shaderDefines;
  shaderDefines.resize(defines.size() + 1);
  for (int i = 0; i < defines.size(); i++) {
    shaderDefines[i].Name = defines[i].c_str();
    shaderDefines[i].Definition = "";
  }
  shaderDefines.back().Name = nullptr;
  shaderDefines.back().Definition = nullptr;

  // Try to compile
  ID3DBlob *pCode = nullptr;
  ID3DBlob *pErrMsg = nullptr;
  HRESULT result =
      D3DCompile(data.data(), data.size(), WCSToMBS(path).c_str(),
                 shaderDefines.data(), &includeHandler, entryPoint.c_str(),
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
      result = DX::DeviceResources::GetDevice()->CreateVertexShader(
          pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr,
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
    } else if (ext == L"cs") {
      ID3D11ComputeShader *pComputeShader = nullptr;
      result = m_pDevice->CreateComputeShader(pCode->GetBufferPointer(),
                                              pCode->GetBufferSize(), nullptr,
                                              &pComputeShader);
      if (SUCCEEDED(result)) {
        *ppShader = pComputeShader;
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
