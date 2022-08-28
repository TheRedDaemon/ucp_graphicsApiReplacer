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
      /* vertex attributes go here to input to the vertex shader */
      struct vertexShaderIn {
          float3 positionLocal : POS;
      };

      /* outputs from vertex shader go here. can be interpolated to pixel shader */
      struct vertexShaderOut {
          float4 positionClip : SV_POSITION; // required output of VS
      };

      vertexShaderOut vertexShaderMain(vertexShaderIn input) {
        vertexShaderOut output = (vertexShaderOut)0; // zero the memory first
        output.position_clip = float4(input.position_local, 1.0);
        return output;
      }

      float4 pixelShaderMain(vertexShaderOut input) : SV_TARGET {
        return float4( 1.0, 0.0, 1.0, 1.0 ); // must return an RGBA colour
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
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Failed to create pixel shader. ");
      return false;
    }

    // INPUT LAYOUT
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
      { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      /*
      { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Failed to create input layout. ");
      return false;
    }

    // create Vertex buffer
    float vertexDataArray[] = {
      -1.0f, -1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,
      -1.0f, 1.0f, 0.0f,
      1.0f, 1.0f, 0.0f,
    };
    UINT vertexStride{ 3 * sizeof(float) };
    UINT vertexOffset{ 0 };
    UINT vertexCount{ 4 };

    D3D11_BUFFER_DESC vertexBuffDescr{};
    vertexBuffDescr.ByteWidth = sizeof(vertexDataArray);
    vertexBuffDescr.Usage = D3D11_USAGE_DEFAULT;
    vertexBuffDescr.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA srData{ 0 };
    srData.pSysMem = vertexDataArray;
    if (FAILED(devicePtr->CreateBuffer(
      &vertexBuffDescr,
      &srData,
      vertexBufferPtr.expose()
    )))
    {
      Log(LOG_ERROR, "[graphicsApiReplacer] : [DirectX] : Unable to create vertex buffer. ");
      return false;
    }

    // set input assembler
    deviceContextPtr->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
    deviceContextPtr->IASetInputLayout(inputLayoutPtr.get());
    deviceContextPtr->IASetVertexBuffers(
      0,
      1,
      vertexBufferPtr.expose(),
      &vertexStride,
      &vertexOffset);

    // set shader
    deviceContextPtr->VSSetShader(vertexShaderPtr.get(), NULL, 0);
    deviceContextPtr->PSSetShader(pixelShaderPtr.get(), NULL, 0);

    receiveDirectXDebugMessages();
    return true;
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

    // min OS is with the format settings Win8
    swapChainDescr.BufferDesc.Format = colorFormat;

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
      Log(LOG_ERROR, "[graphicsApiReplacer]: [DirectX]: Unable to obtain frame buffer.");
      return false;
    }

    if (FAILED(devicePtr->CreateRenderTargetView(framebuffer.get(), 0, renderTargetViewPtr.expose())))
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [DirectX]: Unable to create render target.");
      return false;
    }

    if (!initSystem())
    {
      return false;
    }

    return false;
  }

  // source: https://stackoverflow.com/a/64808444
  HRESULT DirectX11Core::renderNextScreen(unsigned short* backData)
  {
    // Add this before each rendering
    deviceContextPtr->OMSetRenderTargets(1, renderTargetViewPtr.expose(), 0 /*spZView.Get()*/); // no stencil, and actually no array, I just get the ptr to the ptr

    // clear
    FLOAT blankColor[] { 0.5, 0.5, 0.5, 1.0 };
    deviceContextPtr->ClearRenderTargetView(renderTargetViewPtr.get(), blankColor);
    //deviceContextPtr->ClearDepthStencilView(spZView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // no stencil
    
    // drawing...
    deviceContextPtr->Draw(4, 0);

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

    float sW{ static_cast<float>(scale.w) };
    float sH{ static_cast<float>(scale.h) };
    FLOAT newPos[]{
      -1.0f * sW, -1.0f * sH,
      1.0f * sW, -1.0f * sH,
      -1.0f * sW, 1.0f * sH,
      1.0f * sW, 1.0f * sH
    };

    deviceContextPtr->UpdateSubresource(vertexBufferPtr.get(), 0, nullptr, newPos, 0, 0);
  }

  void DirectX11Core::releaseContext(HWND hwnd)
  {
    // does nothing, the interfaces should release
  }
}