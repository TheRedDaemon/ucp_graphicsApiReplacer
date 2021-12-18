#pragma once

// includes that are always used with this
#include "fakeDDClasses.h"

namespace UCPtoOpenGL
{
  // forward declared
  struct SHCWindowOrMainStructFake;
  struct SHCBinkControlStructFake;
  class WindowCore;

  class CrusaderToOpenGL : public FakeDirectDraw
  {
    // using func defines
    using BinkDDSurfaceType = unsigned int(__stdcall *)(IDirectDrawSurface* surfacePtr);
    using SetSomeColors = void(__stdcall *)();

  public:
    CrusaderToOpenGL(ToOpenGLConfig& conf);
    virtual ~CrusaderToOpenGL();

    // that -> the stronghold object(whatever it is)
    void __thiscall createWindow(WNDPROC windowCallbackFunc, SHCWindowOrMainStructFake* that,
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
    bool windowActivated(bool active);

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
    ToOpenGLConfig& confRef;

    // intern:

    const std::unique_ptr<WindowCore> window;

    // contains data values
    struct
    {
      // flags
      bool            windowDone        { false };      // this might get removed, instead becoming a working or breaking situation
      bool            hasFocus          { true };       // should have focus at start
      bool            devourAfterFocus  { false };      // only window continue without render, after the foucs is regained, it requires a first click to get the input again
      bool            cursorClipped     { false };      // only for window mode

      bool            mouseDown[3]      { false, false, false };  // Left, middle, right

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