/****************************************************************************************************/
//  Tutorial Source: https://antongerdelan.net/opengl/d3d11.html
/****************************************************************************************************/

#include "pch.h"

#include "directX11Core.h"

namespace UCPGraphicsApiReplacer
{
  void UCPGraphicsApiReplacer::DirectX11Core::writeOutFeatureLevelString(D3D_FEATURE_LEVEL level)
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
  std::unique_ptr<DirectX11Core::CustomDebugMessage> UCPGraphicsApiReplacer::DirectX11Core::getDebugMessage(int index)
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

    std::unique_ptr<CustomDebugMessage> resultMessage{ std::make_unique<CustomDebugMessage>(
      message->Category, message->Severity, message->ID, message->pDescription)};
    free(message);
    return resultMessage;
  }

  // source: https://stackoverflow.com/a/57362700
  void UCPGraphicsApiReplacer::DirectX11Core::receiveDirectXDebugMessages()
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
      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED | (debug == DEBUG_OFF ? 0 : D3D11_CREATE_DEVICE_DEBUG),
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
      if (SUCCEEDED(devicePtr->QueryInterface(d3dDebug.expose())))
      {
        if (SUCCEEDED(d3dDebug->QueryInterface(infoQueue.expose())))
        {
          D3D11_MESSAGE_ID hide[] =
          {
              D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS, // apparently something really common
              // TODO: Add more message IDs here as needed
          };
          D3D11_INFO_QUEUE_FILTER filter = {};
          filter.DenyList.NumIDs = _countof(hide);
          filter.DenyList.pIDList = hide;
          infoQueue->AddStorageFilterEntries(&filter);
        }
      }

      receiveDirectXDebugMessages();
    }


    return false;
  }
}