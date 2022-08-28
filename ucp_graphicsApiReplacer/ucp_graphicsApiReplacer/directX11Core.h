/****************************************************************************************************/
//  Tutorial Source: https://antongerdelan.net/opengl/d3d11.html
/****************************************************************************************************/

#pragma once

#include "IUnknownWrapper.h"

// parent class
#include "graphicsCore.h"

#include <d3d11.h>       // D3D interface
#include <dxgi.h>        // DirectX driver interface
#include <d3dcompiler.h> // shader compiler

#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d11.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

namespace UCPGraphicsApiReplacer
{
  class DirectX11Core final : public GraphicsCore
  {
  public:
    DirectX11Core();
    ~DirectX11Core() override;

    bool preWindowCreationCall(HINSTANCE hInstance) override;
    bool createWindow(HWND win) override;

    // does nothing, only sets tex size
    void setOnlyTexSize(Size<int> texSize) override;

    void adjustTexSizeAndViewport(Size<int> texSize, Size<int> viewSize, Size<double> scale) override;

    Size<int> getTexStrongSize() override;

    HRESULT renderNextScreen(unsigned short* backData) override;

    void releaseContext(HWND hwnd) override;  // for a bit clean up

  private:

    struct CustomDebugMessage
    {
      D3D11_MESSAGE_CATEGORY Category;
      D3D11_MESSAGE_SEVERITY Severity;
      D3D11_MESSAGE_ID ID;
      std::string Description;
    };

    DXGI_FORMAT colorFormat{};
    bool debug{};

    IUnknownWrapper<ID3D11Device> devicePtr{};
    IUnknownWrapper<ID3D11DeviceContext> deviceContextPtr{};
    IUnknownWrapper<IDXGISwapChain> swapChainPtr{};
    IUnknownWrapper<ID3D11RenderTargetView> renderTargetViewPtr{};

    IUnknownWrapper<ID3D11InfoQueue> infoQueue{};

    // helper functions
    void writeOutFeatureLevelString(D3D_FEATURE_LEVEL level);

    // unlike OpenGL, DirectX11 has no callback feature for this messages, so they need to be polled
    std::unique_ptr<CustomDebugMessage> getDebugMessage(int index);
    void receiveDirectXDebugMessages();
  };
}