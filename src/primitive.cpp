#include "primitive.h"

HRESULT Primitive::Init(Point3f&& pos)
{
  static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
       D3D11_INPUT_PER_VERTEX_DATA, 0} };

  static const Vertex Vertices[] = { {{ -1, 0, -1}, {0, 1, 0}},
                                         {{1, 0, -1}, {0, 1, 0}},
                                         {{1, 0, 1}, {0, 1, 0}},
                                         {{ -1, 0, 1}, {0, 1, 0}} 
  };

  static const UINT16 Indices[] = { 0, 1, 2, 0, 2, 3 };

  HRESULT result = S_OK;

  // Create vertex buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)sizeof(Vertices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = Vertices;
    data.SysMemPitch = (UINT)sizeof(Vertices);
    data.SysMemSlicePitch = 0;

    result = m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pVertexBuffer, "RectVertexBuffer");
    }
  }

  // Create index buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)sizeof(Indices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = Indices;
    data.SysMemPitch = (UINT)sizeof(Indices);
    data.SysMemSlicePitch = 0;

    result = m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pIndexBuffer, "RectIndexBuffer");
    }
  }

  ID3DBlob* pRectVertexShaderCode = nullptr;
  if (SUCCEEDED(result)) {
    result = CompileAndCreateShader(L"../shaders/Surface.vs",
      (ID3D11DeviceChild**)&m_pVertexShader,
      {}, &pRectVertexShaderCode);
  }
  if (SUCCEEDED(result)) {
    result = CompileAndCreateShader(L"../shaders/Surface.ps",
      (ID3D11DeviceChild**)&m_pPixelShader,
      { "USE_LIGHTS" }, {});
  }

  if (SUCCEEDED(result)) {
    result = m_pDevice->CreateInputLayout(
      InputDesc, 2, pRectVertexShaderCode->GetBufferPointer(),
      pRectVertexShaderCode->GetBufferSize(), &m_pInputLayout);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pInputLayout, "RectInputLayout");
    }
  }

  SAFE_RELEASE(pRectVertexShaderCode);

  // Create geometry buffer
  if (SUCCEEDED(result)) {
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(GeomBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    GeomBuffer geomBuffer;
    geomBuffer.m =
      DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z) * DirectX::XMMatrixScaling(10, 10, 10);
    auto m = DirectX::XMMatrixInverse(nullptr, geomBuffer.m);
    m = DirectX::XMMatrixTranspose(m);
    geomBuffer.norm = m;
    geomBuffer.color = Point4f{ 0.0f, 0.25f, 0.75f, 0.5f };

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &geomBuffer;
    data.SysMemPitch = sizeof(geomBuffer);
    data.SysMemSlicePitch = 0;

    result = m_pDevice->CreateBuffer(&desc, &data, &m_pGeomBuffer);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pGeomBuffer, "RectGeomBuffer");
    }
  }

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
    result = m_pDevice->CreateBlendState(&desc, &m_pBlendState);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pBlendState, "TransBlendState");
    }
  }

  // Create reverse transparent depth state
  if (SUCCEEDED(result)) {
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_GREATER;
    desc.StencilEnable = FALSE;

    result = m_pDevice->CreateDepthStencilState(&desc, &m_pDepthState);
    if (SUCCEEDED(result)) {
      result = SetResourceName(m_pDepthState, "TransDepthState");
    }
  }

  return result;
}

void Primitive::Render(){
  ID3D11SamplerState *samplers[] = {m_pSampler};
  m_pDeviceContext->PSSetSamplers(0, 1, samplers);

  ID3D11ShaderResourceView *resources[] = {m_pCubemapView};
  m_pDeviceContext->PSSetShaderResources(0, 1, resources);

  m_pDeviceContext->OMSetDepthStencilState(m_pDepthState, 0);

  m_pDeviceContext->OMSetBlendState(m_pBlendState, nullptr, 0xFFFFFFFF);

  m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT,
                                     0);
  ID3D11Buffer *vertexBuffers[] = {m_pVertexBuffer};
  UINT strides[] = {24};
  UINT offsets[] = {0};
  ID3D11Buffer *cbuffers[] = {m_pSceneBuffer, m_pGeomBuffer};
  m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
  m_pDeviceContext->IASetInputLayout(m_pInputLayout);
  m_pDeviceContext->IASetPrimitiveTopology(
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
  m_pDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
  m_pDeviceContext->PSSetConstantBuffers(0, 2, cbuffers);
  m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

  m_pDeviceContext->DrawIndexed(6, 0, 0);
}

class D3DInclude : public ID3DInclude {
  STDMETHOD(Open)
  (THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData,
   LPCVOID *ppData, UINT *pBytes) {
    FILE *pFile = nullptr;
    fopen_s(&pFile, pFileName, "rb");
    assert(pFile != nullptr);
    if (pFile == nullptr) {
      return E_FAIL;
    }

    fseek(pFile, 0, SEEK_END);
    long long size = _ftelli64(pFile);
    fseek(pFile, 0, SEEK_SET);

    VOID *pData = malloc(size);
    if (pData == nullptr) {
      fclose(pFile);
      return E_FAIL;
    }

    size_t rd = fread(pData, 1, size, pFile);
    assert(rd == (size_t)size);

    if (rd != (size_t)size) {
      fclose(pFile);
      free(pData);
      return E_FAIL;
    }

    *ppData = pData;
    *pBytes = (UINT)size;

    return S_OK;
  }
  STDMETHOD(Close)(THIS_ LPCVOID pData) {
    free(const_cast<void *>(pData));
    return S_OK;
  }
};

HRESULT Primitive::CompileAndCreateShader(
    const std::wstring &path, ID3D11DeviceChild **ppShader,
    const std::vector<std::string> &defines, ID3DBlob **ppCode) {
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
