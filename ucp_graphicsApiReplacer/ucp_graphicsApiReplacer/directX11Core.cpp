/****************************************************************************************************/
//  Tutorial Source: https://antongerdelan.net/opengl/d3d11.html
/****************************************************************************************************/

#include "pch.h"

#include "directX11Core.h"

namespace UCPGraphicsApiReplacer
{
  void DirectX11Core::writeOutFeatureLevelString(D3D_FEATURE_LEVEL level)
  {
    std::string featureLevelMessage{ "[graphicsApiReplacer]: [DirectX]: Using DirectX feature level: " };
    switch (level)
    {
      case D3D_FEATURE_LEVEL_11_0:
        featureLevelMessage.append("DirectX11.0");
        break;
      case D3D_FEATURE_LEVEL_11_1:
        featureLevelMessage.append("DirectX11.1");
        break;
      default:
        featureLevelMessage.append("Unknown");
        break;
    }
    Log(LogLevel::LOG_INFO, featureLevelMessage.c_str());
  }

  // source: https://stackoverflow.com/a/57362700
  std::unique_ptr<DirectX11Core::CustomDebugMessage> DirectX11Core::getDebugMessage(int index)
  {
    static const CustomDebugMessage fallback{
      D3D11_MESSAGE_CATEGORY_MISCELLANEOUS,
      D3D11_MESSAGE_SEVERITY_WARNING,
      D3D11_MESSAGE_ID_UNKNOWN,
      "Failed to obtain debug message."
    };

    SIZE_T messageSize{ 0 };
    if (FAILED(infoQueue->GetMessage(index, nullptr, &messageSize)))  // get the size of the message
    {
      return std::make_unique<CustomDebugMessage>(fallback);
    };

    D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(messageSize); //allocate enough space
    if (FAILED(infoQueue->GetMessage(index, message, &messageSize)))   // get the actual message
    {
      free(message);
      return std::make_unique<CustomDebugMessage>(fallback);
    };

    CustomDebugMessage messageStruct {
      message->Category,
      message->Severity,
      message->ID,
      message->pDescription
    };
    std::unique_ptr<CustomDebugMessage> resultMessage{ std::make_unique<CustomDebugMessage>(std::move(messageStruct)) };
    free(message);
    return resultMessage;
  }

  // source: https://stackoverflow.com/a/57362700
  void DirectX11Core::receiveDirectXDebugMessages()
  {
    if (!debug)
    {
      return;
    }

    UINT64 messageCount{ infoQueue->GetNumStoredMessages() };
    for (UINT64 i = 0; i < messageCount; i++)
    {
      std::unique_ptr<CustomDebugMessage> message{ getDebugMessage(i) };

      LogLevel levelToUse{ LOG_NONE };
      switch (message->Severity)
      {
        case D3D11_MESSAGE_SEVERITY_MESSAGE:
          levelToUse = LOG_DEBUG;
          break;
        case D3D11_MESSAGE_SEVERITY_INFO:
          levelToUse = LOG_INFO;
          break;
        case D3D11_MESSAGE_SEVERITY_WARNING:
          levelToUse = LOG_WARNING;
          break;
        case D3D11_MESSAGE_SEVERITY_ERROR:
          levelToUse = LOG_ERROR;
          break;
        case D3D11_MESSAGE_SEVERITY_CORRUPTION:
          levelToUse = LOG_FATAL;
          break;
        default:
          Log(LOG_ERROR, "[graphicsApiReplacer]: [DirectX]: Received DirectX Debug Message without valid severity.");
          break;
      }

      if (levelToUse != LOG_NONE)
      {
        std::string& description{ message->Description };
        description.erase(description.begin(), std::find_if(description.begin(), description.end(),
          [](unsigned char ch)
          {
            return !std::isspace(ch);
          }
        ));

        // trim from end
        description.erase(std::find_if(description.rbegin(), description.rend(),
          [](unsigned char ch)
          {
            return !std::isspace(ch);
          }
        ).base(), description.end());

        Log(levelToUse, description.insert(0, "[graphicsApiReplacer]: [DirectX]: ").c_str());
      }
    }
    infoQueue->ClearStoredMessages();
  }

  bool DirectX11Core::initSystem()
  {
    const char shaderCode[]{ R"(
      cbuffer DRAWING_HELPER : register(b0) {
          float reverseTransform;
          int3  vSomeVectorThatMayBeNeededByASpecificShader;
      };

      SamplerState SimpleSampler
      {
          Filter = MIN_MAG_MIP_POINT ; // this needs to be settable
          AddressU = Wrap;
          AddressV = Wrap;
      };

      /* vertex attributes go here to input to the vertex shader */
      struct VertexShaderIn {
          float3 positionLocal : POS;
          float2 tex : TEX;
      };

      /* outputs from vertex shader go here. can be interpolated to pixel shader */
      struct VertexShaderOut {
          float4 positionClip : SV_POSITION; // required output of VS
          float2 tex : TEXCOORD0;
      };

      VertexShaderOut vertexShaderMain(VertexShaderIn input) {
        VertexShaderOut output;
        output.positionClip = float4(input.positionLocal, 1.0);
        output.tex = input.tex;
        return output;
      }

      Texture2D gameTexture : register(t0);
      SamplerState simpleSampler : register(s0);

      float4 pixelShaderMain(VertexShaderOut input) : SV_TARGET {
        return gameTexture.Sample(simpleSampler, input.tex); // must return an RGBA colour
      }
    )" };

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | (debug ? D3DCOMPILE_DEBUG : 0);
    IUnknownWrapper<ID3DBlob> vertexShaderBlobPtr{}, pixelShaderBlobPtr{}, errorBlobPtr{};

    // COMPILE VERTEX SHADER
    if (FAILED(D3DCompile(
      shaderCode,
      sizeof(shaderCode),
      "Vertex Shader",
      nullptr,
      D3D_COMPILE_STANDARD_FILE_INCLUDE,
      "vertexShaderMain",
      "vs_5_0",
      flags,
      0,
      vertexShaderBlobPtr.expose(),
      errorBlobPtr.expose()
    )))
    {
      std::string shaderError{ "[graphicsApiReplacer] : [DirectX] : Vertex Shader Error: " };
      shaderError.append(errorBlobPtr ? (char*)errorBlobPtr->GetBufferPointer() : "Unknown Error");
      Log(LOG_ERROR, shaderError.c_str());
      return false;
    }

    // COMPILE PIXEL SHADER
    if (FAILED(D3DCompile(
      shaderCode,
      sizeof(shaderCode),
      "Pixel Shader",
      nullptr,
      D3D_COMPILE_STANDARD_FILE_INCLUDE,
      "pixelShaderMain",
      "ps_5_0",
      flags,
      0,
      pixelShaderBlobPtr.expose(),
      errorBlobPtr.expose()
    )))
    {
      std::string shaderError{ "[graphicsApiReplacer] : [DirectX] : Pixel Shader Error: " };
      shaderError.append(errorBlobPtr ? (char*)errorBlobPtr->GetBufferPointer() : "Unknown Error");
      Log(LOG_ERROR, shaderError.c_str());
      return false;
    }

    if (FAILED(devicePtr->CreateVertexShader(
      vertexShaderBlobPtr->GetBufferPointer(),
      vertexShaderBlobPtr->GetBufferSize(),
      NULL,
      vertexShaderPtr.expose()
    )))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Failed to create vertex shader. ");
      return false;
    }
    
    if (FAILED(devicePtr->CreatePixelShader(
      pixelShaderBlobPtr->GetBufferPointer(),
      pixelShaderBlobPtr->GetBufferSize(),
      NULL,
      pixelShaderPtr.expose()
    )))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Failed to create pixel shader. ");
      return false;
    }

    // INPUT LAYOUT
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
      { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      /*
      { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      */
    };
    if (FAILED(devicePtr->CreateInputLayout(
      inputElementDesc,
      ARRAYSIZE(inputElementDesc),
      vertexShaderBlobPtr->GetBufferPointer(),
      vertexShaderBlobPtr->GetBufferSize(),
      inputLayoutPtr.expose()
    )))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Failed to create input layout. ");
      return false;
    }

    // data
    float vertexDataArray[]{
      -1.0f, -1.0f,   // pos
      0.0f, 1.0f,     // tex

      1.0f, -1.0f,
      1.0f, 1.0f,

      -1.0f, 1.0f,
      0.0f, 0.0f,
      
      1.0f, 1.0f,
      1.0f, 0.0f,
    };
    UINT vertexStride{ 4 * sizeof(float) };
    UINT vertexOffset{ 0 };
    UINT vertexCount{ 4 };

    int indexDataArray[]{ 0, 2, 1, 2, 3, 1 };

    float textureCoordsArray[]{
      -1.0f, -1.0f,
      1.0f, -1.0f,
      -1.0f, 1.0f,
      1.0f, 1.0f,
    };

    struct
    {
      float reverseTransform{ 1.0 / 0xFF };
      int colorTransformConstant[3]{ 0, 0, 0b0000000000011111 };
    } constantValueStruct;
    if (colorFormat == RGB_565) {
      constantValueStruct.colorTransformConstant[0] = 0b1111100000000000;
      constantValueStruct.colorTransformConstant[1] = 0b0000011111100000;
    }
    else {
      constantValueStruct.colorTransformConstant[0] = 0b0111110000000000;
      constantValueStruct.colorTransformConstant[1] = 0b0000001111100000;
    }

    // create Vertex buffer
    D3D11_BUFFER_DESC vertexBuffDescr{};
    vertexBuffDescr.ByteWidth = sizeof(vertexDataArray);
    vertexBuffDescr.Usage = D3D11_USAGE_DEFAULT;
    vertexBuffDescr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexSrData{ 0 };
    vertexSrData.pSysMem = vertexDataArray;
    if (FAILED(devicePtr->CreateBuffer(
      &vertexBuffDescr,
      &vertexSrData,
      vertexBufferPtr.expose()
    )))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Unable to create vertex buffer. ");
      return false;
    }

    // create index buffer
    D3D11_BUFFER_DESC indexBuffDescr{};
    indexBuffDescr.ByteWidth = sizeof(indexDataArray);
    indexBuffDescr.Usage = D3D11_USAGE_IMMUTABLE;
    indexBuffDescr.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexSrData{ 0 };
    indexSrData.pSysMem = indexDataArray;
    if (FAILED(devicePtr->CreateBuffer(
      &indexBuffDescr,
      &indexSrData,
      indexBufferPtr.expose()
    )))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Unable to create index buffer. ");
      return false;
    }

    // create constant pixel transform buffer
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = sizeof(constantValueStruct);
    cbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA constData{};
    constData.pSysMem = &constantValueStruct;

    // Create the buffer.
    if (FAILED(devicePtr->CreateBuffer(
      &cbDesc,
      &constData,
      constantPixelTransformBufferPtr.expose()
    )))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Unable to create constant color transform buffer. ");
      return false;
    }

    // create sampler state
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = confPtr->graphic.filterLinear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    if (FAILED(devicePtr->CreateSamplerState(&samplerDesc, samplerStatePtr.expose())))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Unable to create sampler state. ");
      return false;
    }

    // set input assembler
    deviceContextPtr->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContextPtr->IASetInputLayout(inputLayoutPtr.get());
    deviceContextPtr->IASetVertexBuffers(
      0,
      1,
      vertexBufferPtr.expose(),
      &vertexStride,
      &vertexOffset);
    deviceContextPtr->IASetIndexBuffer(indexBufferPtr.get(), DXGI_FORMAT_R32_UINT, 0);

    // set shader
    deviceContextPtr->VSSetShader(vertexShaderPtr.get(), NULL, 0);
    deviceContextPtr->PSSetShader(pixelShaderPtr.get(), NULL, 0);

    deviceContextPtr->PSSetConstantBuffers(0, 1, constantPixelTransformBufferPtr.expose());
    deviceContextPtr->PSSetSamplers(0, 1, samplerStatePtr.expose());

    receiveDirectXDebugMessages();
    return true;
  }


  void UCPGraphicsApiReplacer::DirectX11Core::createRenderAndGameTexture()
  {
    if (gameTexturePtr)
    {
      gameTexturePtr->Release();
      *gameTexturePtr.expose() = nullptr;
    }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = strongTexSize.w;
    desc.Height = strongTexSize.h;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = colorFormat;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (FAILED(devicePtr->CreateTexture2D(&desc, NULL, gameTexturePtr.expose())))
    {
      receiveDirectXDebugMessages();
      Log(LOG_FATAL, "[graphicsApiReplacer] : [DirectX] : Unable to create game texture resource. "); // needs to crash
    }

    if (gameTextureViewPtr)
    {
      gameTextureViewPtr->Release();
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format = colorFormat;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MipLevels = 1;
    if (FAILED(devicePtr->CreateShaderResourceView(gameTexturePtr.get(), &SRVDesc, gameTextureViewPtr.expose())))
    {
      receiveDirectXDebugMessages();
      Log(LOG_FATAL, "[graphicsApiReplacer] : [DirectX] : Unable to create game texture view. "); // needs to crash
    }

    deviceContextPtr->PSSetShaderResources(0, 1, gameTextureViewPtr.expose());
  }



  DirectX11Core::DirectX11Core() {};
  DirectX11Core::~DirectX11Core() {};

  bool DirectX11Core::preWindowCreationCall(HINSTANCE hInstance)
  {
    return true;  // unused
  }

  bool DirectX11Core::createWindow(HWND win)
  {
    if (!confPtr)
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [DirectX]: Did not receive configuration.");
      return false;
    }

    HRESULT hResult{}; // used for results

    colorFormat = confPtr->graphic.pixFormat == RGB_565 ? DXGI_FORMAT_B5G6R5_UNORM : DXGI_FORMAT_B5G5R5A1_UNORM;
    debug = confPtr->graphic.debug != DEBUG_OFF;
    vSync = confPtr->graphic.vsync;

    // creates a window swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDescr{};

    // as fast as possible
    swapChainDescr.BufferDesc.RefreshRate.Numerator = 0;  
    swapChainDescr.BufferDesc.RefreshRate.Denominator = 1;

    // using a normal format and setting it in render target
    swapChainDescr.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    // no multi-sampling
    swapChainDescr.SampleDesc.Count = 1;
    swapChainDescr.SampleDesc.Quality = 0;

    // one output buffer with two backbuffers for flip sequential
    swapChainDescr.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescr.BufferCount = 2;
    swapChainDescr.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    // maybe allowing variable framerates... needs tests
    swapChainDescr.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // create as window -> normal fullscreen in obsolete today
    swapChainDescr.OutputWindow = win;
    swapChainDescr.Windowed = true;

    // order of feature levels it tries to create
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    D3D_FEATURE_LEVEL resultingFeatureLevel;
    hResult = D3D11CreateDeviceAndSwapChain(
      NULL,
      D3D_DRIVER_TYPE_HARDWARE,
      NULL,
      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED | (debug ? D3D11_CREATE_DEVICE_DEBUG : 0),
      featureLevels,
      ARRAYSIZE(featureLevels),
      D3D11_SDK_VERSION,
      &swapChainDescr,
      swapChainPtr.expose(),
      devicePtr.expose(),
      &resultingFeatureLevel,
      deviceContextPtr.expose());

    if (FAILED(hResult) || !swapChainPtr || !devicePtr || !deviceContextPtr)
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [DirectX]: Failed to create device and swap chain.");
      return false;
    }
    writeOutFeatureLevelString(resultingFeatureLevel);

    if (debug)
    {
      // source: https://walbourn.github.io/dxgi-debug-device/

      IUnknownWrapper<ID3D11Debug> d3dDebug;
      if (debug && FAILED(devicePtr->QueryInterface(d3dDebug.expose())))
      {
        Log(LOG_WARNING, "[graphicsApiReplacer]: [DirectX]: Unable to get debugging interface. Debugging disabled.");
        debug = false;
      }

      if (debug && FAILED(d3dDebug->QueryInterface(infoQueue.expose())))
      {
        Log(LOG_WARNING, "[graphicsApiReplacer]: [DirectX]: Unable to receive debug info queue. Debugging disabled.");
        debug = false;
      }

      D3D11_MESSAGE_ID hide[] =
      {
          D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS, // apparently something really common
          // TODO: Add more message IDs here as needed
      };
      D3D11_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.NumIDs = _countof(hide);
      filter.DenyList.pIDList = hide;
      if (debug && FAILED(infoQueue->AddStorageFilterEntries(&filter)))
      {
        Log(LOG_WARNING, "[graphicsApiReplacer]: [DirectX]: Unable to set debug storage filter. Debugging disabled.");
        debug = false;
      }

      receiveDirectXDebugMessages();
    }

    // get render target
    IUnknownWrapper<ID3D11Texture2D> framebuffer{};
    if (FAILED(swapChainPtr->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)framebuffer.expose())))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer]: [DirectX]: Unable to obtain frame buffer.");
      return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC targetViewDesc{
      CD3D11_RENDER_TARGET_VIEW_DESC(framebuffer.get(), D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM)
    };
    if (FAILED(devicePtr->CreateRenderTargetView(framebuffer.get(), &targetViewDesc, renderTargetViewPtr.expose())))
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer]: [DirectX]: Unable to create render target.");
      return false;
    }

    if (!initSystem())
    {
      return false;
    }

    return true;
  }

  // source: https://stackoverflow.com/a/64808444
  HRESULT DirectX11Core::renderNextScreen(unsigned short* backData)
  {
    // update texture, source: https://docs.microsoft.com/en-us/windows/win32/direct3d11/how-to--use-dynamic-resources
    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    //  Disable GPU access to the vertex buffer data.
    if (SUCCEEDED(deviceContextPtr->Map(gameTexturePtr.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
    {
      //  Update the vertex buffer here.
      size_t bytesOfOneLine{ static_cast<size_t>(strongTexSize.w * 2) };
      unsigned short* sourceRunPtr{ backData };
      unsigned char* destRunPtr{ (unsigned char*)mappedResource.pData };
      for (size_t i{ 0 }; i < strongTexSize.h; i++)
      {
        memcpy(destRunPtr, sourceRunPtr, bytesOfOneLine);
        sourceRunPtr += strongTexSize.w;
        destRunPtr += mappedResource.RowPitch;
      }
      //  Re-enable GPU access to the vertex buffer data.
      deviceContextPtr->Unmap(gameTexturePtr.get(), 0);
    }
    else
    {
      receiveDirectXDebugMessages();
      Log(LOG_ERROR, "[graphicsApiReplacer]: [DirectX]: Unable to write to game texture.");
    }


    // Add this before each rendering
    deviceContextPtr->OMSetRenderTargets(1, renderTargetViewPtr.expose(), 0 /*spZView.Get()*/); // no stencil, and actually no array, I just get the ptr to the ptr

    // clear
    FLOAT blankColor[] { 0.0, 0.0, 0.0, 1.0 };
    deviceContextPtr->ClearRenderTargetView(renderTargetViewPtr.get(), blankColor);
    //deviceContextPtr->ClearDepthStencilView(spZView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // no stencil
    
    // drawing...
    deviceContextPtr->DrawIndexed(6, 0, 0);

    // swap
    HRESULT swapRes{ (vSync) ? swapChainPtr->Present(1, 0) : swapChainPtr->Present(0, DXGI_PRESENT_ALLOW_TEARING) };
    receiveDirectXDebugMessages();
    return swapRes;
  }

  void DirectX11Core::setOnlyTexSize(Size<int> texSize)
  {
    strongTexSize = texSize;
  }

  void DirectX11Core::adjustTexSizeAndViewport(Size<int> texSize, Size<int> viewSize, Size<double> scale)
  {
    D3D11_VIEWPORT viewport{
      0.0f,
      0.0f,
      (FLOAT)viewSize.w,
      (FLOAT)viewSize.h,
      0.0f,
      1.0f };
    deviceContextPtr->RSSetViewports(1, &viewport);
    strongTexSize = texSize;

    float sW{ static_cast<float>(scale.w) };
    float sH{ static_cast<float>(scale.h) };
    FLOAT newBuffer[]{
      -1.0f * sW, -1.0f * sH, // pos
      0.0f, 1.0f,             // tex
      
      1.0f * sW, -1.0f * sH,
      1.0f, 1.0f,

      -1.0f * sW, 1.0f * sH,
      0.0f, 0.0f,

      1.0f * sW, 1.0f * sH,
      1.0f, 0.0f,
    };
    deviceContextPtr->UpdateSubresource(vertexBufferPtr.get(), 0, nullptr, newBuffer, 0, 0);

    createRenderAndGameTexture();
    receiveDirectXDebugMessages();
  }

  void DirectX11Core::releaseContext(HWND hwnd)
  {
    // does nothing, the interfaces should release
  }
}