#pragma once

// includes that are always used with this
#include "fakeDDClasses.h"

namespace UCPGraphicsApiReplacer
{
  // forward declared
  struct SHCWindowOrMainStructFake;
  struct SHCBinkControlStructFake;
  class GraphicsCore;


  class CrusaderGraphicsApiReplacer : public FakeDirectDraw
  {
    // using func defines
    using BinkDDSurfaceType = unsigned int(__stdcall *)(IDirectDrawSurface* surfacePtr);
    using SetSomeColors = void(__stdcall *)();


  public:
    CrusaderGraphicsApiReplacer(GraphicsAPIReplacerConfig& conf);
    virtual ~CrusaderGraphicsApiReplacer();

    // that -> the stronghold object(whatever it is)
    void __thiscall createWindow(SHCWindowOrMainStructFake* that,
    HINSTANCE hInstance, LPSTR windowName, unsigned int cursorResource);

    void __thiscall drawInit(SetSomeColors colorFunc, SHCBinkControlStructFake* binkStruct,
      DWORD winSetRectObjBaseAddr, SHCWindowOrMainStructFake* that);

    int getFakeSystemMetrics(int nIndex);

    BOOL getWindowCursorPos(LPPOINT lpPoint);

    HWND WINAPI GetForegroundWindowFake();

    // returns 'true' if the mouse action should get transported, 'false' if it should get discarded
    bool transformMouseMovePos(LPARAM* ptrlParam);

    // false, if the message should be devoured
    bool windowLostFocus();

    // false, if the message should be devoured
    bool windowSetFocus();

    // false, if the message should be devoured
    bool windowActivated(bool* active);

    void windowDestroyed();

    void windowEditEnded();

    // send if down in the client
    bool mouseDown();

    // clip it to prevent "hold -> leave window -> release" issues
    void mouseClipOnHold(UINT wmMsg);


  private:  // functions

     /*** need impl ***/
    virtual HRESULT renderNextFrame(unsigned short* sourcePtr) override;
    virtual int getRenderTexWidth() override;
    virtual int getRenderTexHeight() override;
    virtual PixelFormat getPixelFormat() override;

    // config
    void setWindowStyleAndSize();
    void clipCursor();

  private:  // variables

    // config:
    GraphicsAPIReplacerConfig& confRef;

    // intern:

    std::unique_ptr<GraphicsCore> graphicsCore;

    // contains data values
    struct
    {
      // flags
      bool            hasFocus          { true };       // should have focus at start
      bool            devourAfterFocus  { false };      // only window continue without render, after the focus is regained, it requires a first click to get the input again
      bool            cursorClipped     { false };      // only for window mode

      bool            mouseDown[3]      { false, false, false };  // left, middle, right

      // sizes
      Size<int>       windowSize        { 1280, 720 };  // ignores border etc.
      Size<int>       windowOffset      { 0, 0 };
      Size<int>       gameWindowRange   { 1279, 719 };  // window size - 1, because range
      Size<int>       gameTexSize       { 0, 0 };
      Size<int>       scrollRange       { 1279, 719 };  // game size - 1, because scroll max
      Size<double>    winToGamePos      { 1.0, 1.0 };   // used for positions, since they range from 0 to width - 1 and need handling for both axes
    } d;  // data

    SHCWindowOrMainStructFake* shcWinStrucPtr{ nullptr };
  };
}