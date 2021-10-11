
#pragma once

namespace UCPtoOpenGL
{
  // holds possible render resolutions
  enum GameResolution
  {
    RES_800_X_600	      = 0x1         ,
    RES_1024_X_768	    = 0x2         ,
    RES_1280_X_720	    = 0x3         ,
    RES_1280_X_1024	    = 0x4         ,
    RES_1366_X_768	    = 0x5         ,
    RES_1440_X_900	    = 0x6         ,
    RES_1600_X_900	    = 0x7         ,
    RES_1600_X_1200	    = 0x8         ,
    RES_1680_X_1050	    = 0x9         ,
    RES_1920_X_1080	    = 0xa         ,
    RES_1920_X_1200	    = 0xb         ,
    RES_2560_X_1440	    = 0xc         ,
    RES_2560_X_1600	    = 0xd         ,
    RES_1360_X_768	    = 0xe         ,
    RES_1024_X_600	    = 0xf         ,
    RES_640_X_480	      = 0x14        ,
    RES_NONE	          = 0xffffffff  ,
  };

  // hardcoded const resolutions of the game, index is the GameResolution enum
  inline constexpr std::array<int[2], 17> RESOLUTIONS { {
    { 0     ,   0     },  // 0 has none
    { 800   ,   600   },
    { 1024  ,   768   },
    { 1280  ,   720   },
    { 1280  ,   1024  },
    { 1366  ,   768   },
    { 1440  ,   900   },
    { 1600  ,   900   },
    { 1600  ,   1200  },
    { 1680  ,   1050  },
    { 1920  ,   1080  },
    { 1920  ,   1200  },
    { 2560  ,   1440  },
    { 2560  ,   1600  },
    { 1360  ,   768   },
    { 1024  ,   600   },
    { 640   ,   480   },
  } };

  class CrusaderToOpenGL : public IDirectDraw
  {
  public:
    CrusaderToOpenGL() {};

    virtual ~CrusaderToOpenGL() {};

    // Inferface methods: Most are changed to do nothing but return DD_OK

    /*
      Parts of the interfaces were taken from the ddraw_logger: https://github.com/JeremyAnsel/ddraw_logger
      Copyright (c) 2014 Jérémy Ansel
      Licensed under the MIT license. See LICENSE.txt
    */

    /*** not used ***/

    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID FAR*)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD_(ULONG, AddRef) (THIS)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(Compact)(THIS)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR*)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR*)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR*)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(FlipToGDISurface)(THIS)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(GetDisplayMode)(THIS_ LPDDSURFACEDESC)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR*)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(GetScanLine)(THIS_ LPDWORD)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(Initialize)(THIS_ GUID FAR*)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(RestoreDisplayMode)(THIS)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD)
    {
      return DD_OK;	// called, but ignored
    }

    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE)
    {
      return DD_OK;	// not called by Crusader
    }

    STDMETHOD_(ULONG, Release) (THIS)
    {
      return DD_OK; // called, by now ignored, there is no interface to free
    }


    /*** modified ***/

    // uses windows function to get the display modes
    STDMETHOD(EnumDisplayModes)(THIS_ DWORD dw, LPDDSURFACEDESC lpsurf, LPVOID lpvoid, LPDDENUMMODESCALLBACK callback);

    // return as if no compatibility
    STDMETHOD(GetCaps)(THIS_ LPDDCAPS cap1, LPDDCAPS cap2);

    // needs handling
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR*, IUnknown FAR*);

    // needs handling -> place to get current resolution
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD, DWORD);

    /* copied structure end */

    void setConf(ToOpenGLConfig* conf)
    {
      confPtr = conf;
    };

    // that -> the stronghold object(whatever it is)
    bool createWindow(DWORD that, LPSTR windowName, unsigned int unknown, WNDPROC keyboardCallbackFunc);

    HRESULT createDirectDraw(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter);

    int getFakeSystemMetrics(int nIndex);

    BOOL setFakeRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);

    BOOL getWindowCursorPos(LPPOINT lpPoint);

    BOOL setWindowPosFake(HWND, HWND, int, int, int, int, UINT);

    BOOL WINAPI updateWindowFake(HWND hWnd);

    BOOL WINAPI adjustWindowRectFake(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);

    LONG WINAPI setWindowLongAFake(HWND hWnd, int nIndex, LONG dwNewLong);

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

    HWND getWindowHandle()
    {
      return winHandle;
    }

  private:

    // config:
    ToOpenGLConfig* confPtr{ nullptr };

    // intern:

    WindowCore window;

    // window stuff
    HWND winHandle{ 0 };	// the actual window -> stronghold should clean this up
    bool windowDone{ false };
    int gameWinSizeW{ 1280 };
    int gameWinSizeH{ 720 };
    int winOffsetW{ 0 };
    int winOffsetH{ 0 };
    int gameScreenSizeW{ 1280 };
    int gameScreenSizeH{ 720 };
    double winToTexMult{ 1.0 }; // used for sizes, ranging from 0 to width
    double winToGamePosX{ 1.0 };  // used for positions, since they range from 0 to width - 1 and need handling for both axes
    double winToGamePosY{ 1.0 };  // used for positions, since they range from 0 to width - 1 and need handling for both axes
    bool hasFocus{ true };  // should have focus at start
    bool devourAfterFocus{ false }; // only window continue without render, after the foucs is regained, it requires a first click to get the input again
    bool cursorClipped{ false };  // only for window mode

    bool possibleTexChange{ false };  // hint that the texture might have changed

    // set during drawing rect inits, removed during DirectDrawCreate
    // sets scroll borders, since they do not react to resolution changes
    bool rectInit{ false };
    bool resChanged{ false };
    int scrollMaxW{ 0 };  // width - 1, because scroll max
    int scrollMaxH{ 0 };
    int scrollDeadZone{ 0 }; // zone in render pixels outside of window where it still scrolls
    int scrollZoneWidth{ 0 };  // zone in render pixels inside the window where it already scrolls

    // fake interfaces
    FakeOffscreenMain offMain{ &window };
    FakeOffscreenMap offMap;
    FakeBackbuffer back{ &window };
    FakePrimary prim{ &window, &back };


    // functions:

    // config
    void setWindowStyleAndSize();

    void clipCursor();
  };
}