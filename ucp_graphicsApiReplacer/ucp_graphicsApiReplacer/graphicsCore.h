#pragma once

// interface to use for graphic API implementations

namespace UCPGraphicsApiReplacer
{
  class GraphicsCore
  {
  public:
    GraphicsCore() {};
    virtual ~GraphicsCore() {};

    void setConf(GraphicsAPIReplacerConfig* conf)
    {
      confPtr = conf;
    };

    virtual bool preWindowCreationCall(HINSTANCE hInstance) = 0;  // is called before the actual crusader window is created
    virtual bool createWindow(HWND win) = 0;

    virtual void setOnlyTexSize(Size<int> texSize) = 0;

    virtual void adjustTexSizeAndViewport(Size<int> texSize, Size<int> viewSize, Size<double> scale) = 0;

    virtual Size<int> getTexStrongSize() = 0;

    virtual HRESULT renderNextScreen(unsigned short* backData) = 0;

    virtual void releaseContext(HWND hwnd) = 0;  // for a bit clean up

    static std::unique_ptr<GraphicsCore> GetGraphicsCore(GraphicsAPIReplacerConfig& conf);

  protected:

    // config:
    GraphicsAPIReplacerConfig* confPtr{ nullptr };
    PixelFormat pixFormat{ ARGB_1555 };
  };
}