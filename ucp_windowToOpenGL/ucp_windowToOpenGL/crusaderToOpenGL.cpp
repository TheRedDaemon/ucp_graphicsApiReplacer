
#include "pch.h"


#include "windowCore.h"

#include "shcRelatedStructures.h"
#include "crusaderToOpenGL.h"


#include "configUtil.h"


namespace UCPtoOpenGL
{
  CrusaderToOpenGL::CrusaderToOpenGL(ToOpenGLConfig* conf) : confPtr{ conf }, window{ std::make_unique<WindowCore>() } {};
  CrusaderToOpenGL::~CrusaderToOpenGL() {};


  /** Override **/


  // impl virtual

  HRESULT CrusaderToOpenGL::renderNextFrame(unsigned short* sourcePtr)
  {
    return window->renderNextScreen(sourcePtr);
  }
  int CrusaderToOpenGL::getRenderTexWidth()
  {
    return d.gameTexSize.w;
  }
  int CrusaderToOpenGL::getRenderTexHeight()
  {
    return d.gameTexSize.h;
  }
  PixelFormat CrusaderToOpenGL::getPixelFormat()
  {
    return confPtr->graphic.pixFormat;
  }


  // DirectDraw

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::EnumDisplayModes(DWORD, LPDDSURFACEDESC,
    LPVOID, LPDDENUMMODESCALLBACK)
  {
    // ignore callback, just fill struct:
    for (size_t i = 1; i < RESOLUTIONS.size() - 1; i++) // 0 and the last are not available
    {
      // this res causes a crash, because it is handled differently
      if (i == RES_1366_X_768)
      {
        continue; // -> disabled at the moment
      }

      shcWinStrucPtr->resolutionSupported[i] = 1;
    }

    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::GetCaps(THIS_ LPDDCAPS cap1, LPDDCAPS)
  {
    //return realInterface->GetCaps(cap1, cap2);

    // the original returned only three values: DDCAPS_NOHARDWARE, DDCAPS2_CANRENDERWINDOWED, DDCAPS2_CERTIFIED
    // the compatibility mode returns a whole set of values, but the system here still works
    // I assume this is the result of Crusader having a software renderer

    // using the non-compatibility return now, if some effects are missing in this case... well, maybe it will surface some day
    // ignore cap2, this was NULL in tests
    cap1->dwCaps = DDCAPS_NOHARDWARE;
    cap1->dwCaps2 = DDCAPS2_CANRENDERWINDOWED | DDCAPS2_CERTIFIED;
    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::SetDisplayMode(DWORD width, DWORD height, DWORD)
  {
    // in-theory, the earliest to adapt to screen changes where the tex heights are known is now the
    // setRectFake function on the second call per request

    // execute either if hint received or if the tex size is not initialized
    Size<int>& texS{ d.gameTexSize };
    if (possibleTexChange || texS.w == 0 || texS.h == 0)
    {
      Size<int>& winS{ d.windowSize };
      Size<int>& winOff{ d.windowOffset };
      double& mul{ d.winToTexMult };
      Size<int>& screenS{ d.gameScreenSize };
      Size<double>& winToGame{ d.winToGamePos };

      texS = { (int)width, (int)height };

      //create new bit maps
      int pixNum{ texS.w * texS.h };
      back.createBitData(pixNum);
      offMain.createBitData(pixNum);

      // change scale
      // I choose the easy route, algorithm: https://math.stackexchange.com/a/1620375
      
      // compute needed base values
      Size<double> winToTex{ static_cast<double>(texS.w) / winS.w, static_cast<double>(texS.h) / winS.h };
      mul = winToTex.h > winToTex.w ? winToTex.h : winToTex.w;
      Size<double> winScale{ winToTex.w / mul, winToTex.h / mul };
      
      // offset in window pixels
      winOff = { lround((1.0 - winScale.w) * winS.w / 2.0), lround((1.0 - winScale.h) * winS.h / 2.0) };

      // size of the game in the window
      screenS = { winS.w - winOff.w * 2, winS.h - winOff.h * 2 };

      // for positions, I need to use the relation between the size of the game in the window, and the tex (the pixel size of the game)
      winToGame = { (static_cast<double>(texS.w) - 1.0) / (screenS.w - 1.0), (static_cast<double>(texS.h) - 1.0) / (screenS.h - 1.0) };

      window->adjustTexSizeAndViewport(texS, winS, winScale);

      possibleTexChange = false;
    }

    // set clip for cursor -> should be needed for resolution change
    if (confPtr->control.clipCursor && (confPtr->window.type != TYPE_WINDOW || resChanged))
    {
      clipCursor();
    }

    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::CreateSurface(LPDDSURFACEDESC des, LPDIRECTDRAWSURFACE* retSurfPtr, IUnknown*)
  {
    if (des->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
      *retSurfPtr = &prim;
      return DD_OK;
    }

    // offscreen surfaces (backbuffer is gathered different)
    if (des->dwHeight == 2076 && des->dwWidth == 4056)	// lets hope this resolution will never be supported
    {
      *retSurfPtr = &offMap;
    }
    else
    {
      *retSurfPtr = &offMain;
    }

    return DD_OK;
  }




  // lua bound calls

  bool CrusaderToOpenGL::createWindow(SHCWindowOrMainStructFake* that, LPSTR windowName, unsigned int cursorResource, WNDPROC windowCallbackFunc)
  {
    // keep ref
    shcWinStrucPtr = that;

    if (!confPtr) // no config, so everything normal
    {
      // recreated original window func
      HINSTANCE hInstance{ shcWinStrucPtr->gameHInstance };
      LPCSTR className{ "FFwinClass" };

      WNDCLASSA wndClass;
      wndClass.hInstance = hInstance;
      wndClass.lpfnWndProc = windowCallbackFunc;
      wndClass.style = NULL;	// this needs changes later
      wndClass.cbClsExtra = NULL;
      wndClass.cbWndExtra = NULL;
      wndClass.hIcon = LoadIconA(hInstance, (LPCSTR)(cursorResource & 0xFFFF));	// this is apparently the cursor resource?
      wndClass.hCursor = NULL;
      wndClass.hbrBackground = NULL;
      wndClass.lpszMenuName = NULL;
      wndClass.lpszClassName = className;

      ATOM classAtom{ RegisterClassA(&wndClass) };
      if (classAtom == NULL)
      {
        return false;
      }

      shcWinStrucPtr->gameWindowHandle = CreateWindowExA(
        NULL,
        className,
        windowName,
        WS_POPUP | WS_VISIBLE, // WS_POPUP is apperantly an indicator that the window is only short li
        0,
        0,
        GetSystemMetrics(0),	// at this point, no hint exist as to how big it should be
        GetSystemMetrics(1),
        NULL,
        NULL,
        hInstance,
        NULL
      );

      return shcWinStrucPtr->gameWindowHandle != NULL;
    }
    else
    {
      // try to change stuff
      HINSTANCE hInstance{ shcWinStrucPtr->gameHInstance };
      LPCSTR className{ "FFwinClass" };


      // trying to get WGL functions, if this fails, return false, Crusader crashes
      if (!window->loadWGLFunctions(hInstance))
      {
        return false;
      }


      WNDCLASSA wndClass;
      wndClass.hInstance = hInstance;
      wndClass.lpfnWndProc = windowCallbackFunc;
      wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;	// CS_OWNDC apperantly needed to allow a constant device context CS_HREDRAW | CS_VREDRAW
      wndClass.cbClsExtra = NULL;
      wndClass.cbWndExtra = NULL;
      wndClass.hIcon = LoadIconA(hInstance, (LPCSTR)(cursorResource & 0xFFFF));	// this is apparently the cursor resource?
      wndClass.hCursor = NULL;
      wndClass.hbrBackground = NULL;
      wndClass.lpszMenuName = NULL;
      wndClass.lpszClassName = className;

      ATOM classAtom{ RegisterClassA(&wndClass) };
      if (classAtom == NULL)
      {
        return false;
      }

      RECT winRect{ GetWindowRect(confPtr->window) };
      d.windowSize = { GetGameWidth(confPtr->window), GetGameHeight(confPtr->window) };

      shcWinStrucPtr->gameWindowHandle = CreateWindowExA(
        GetExtendedWindowStyle(confPtr->window.type),
        className,
        windowName,
        GetWindowStyle(confPtr->window.type),
        winRect.left,
        winRect.top,
        winRect.right - winRect.left,
        winRect.bottom - winRect.top,
        NULL,
        NULL,
        hInstance,
        NULL
      );

      HWND handle{ shcWinStrucPtr->gameWindowHandle };

      // this is an issue -> if something here fails, then the whole thing might be busted...
      // TODO: is there a possible way around? -> close everything until then, then recreate with original Stronghold?
      // until then, this will return false, and I assume stronghold will close (or crash, not tested here yet)
      window->setConf(confPtr);
      d.windowDone = handle && window->createWindow(handle);

      // failing at this part can will cause issues
      return handle != NULL;
    }
  }

  HRESULT CrusaderToOpenGL::createDirectDraw(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
  {
    // get library and func (just once)
    static decltype(DirectDrawCreate)* create{ nullptr };
    if (create == nullptr)
    {
      HMODULE ddraw{ GetModuleHandleA("ddraw.dll") };
      if (ddraw == NULL) return DDERR_GENERIC;	// lets hope the game just crashes normally... need better handling... but what?
      create = (decltype(DirectDrawCreate)*)GetProcAddress(ddraw, "DirectDrawCreate");
    }

    HRESULT res;
    if (d.windowDone)
    {
      *lplpDD = this; // in this case, no interface is created at all
      res = DD_OK;
    }
    else
    {
      res = create(lpGUID, lplpDD, pUnkOuter);
    }

    return res;
  }

  int CrusaderToOpenGL::getFakeSystemMetrics(int nIndex)
  {
    // The fake window size can only be set after a refocus. Before that, it likely sets the choosen render size
    // before or around the "SetDisplayMode" method.
    // TODO: Find out how to change this. It also effects other parts. -> Using real screen size as return for now.
    if (d.windowDone)
    {
      switch (nIndex)
      {
      case SM_CXSCREEN:
      {
        int texWidth{ d.gameTexSize.w };
        if (texWidth > 0)
        {
          return texWidth;
        }
        break;
      }
      case SM_CYSCREEN:
      {
        int texHeight{ d.gameTexSize.h };
        if (texHeight > 0)
        {
          return texHeight;
        }
        break;
      }
      default:
        break;
      }
    }
    return GetSystemMetrics(nIndex);
  }


  BOOL CrusaderToOpenGL::setFakeRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
  {
    // There seem to be three Rects (at least around the screen init):
    // -> The main Rect
    // -> a second rect for the initialisation (only during init, resolution)
    // -> a third used during refocus
    // this structure adapts to the first

    // trying to get as early as possible, issue -> scroll, does not seem to get set
    // by resolution change through this structure
    // -> change scroll method altogether
    if (confPtr)  // this part will likely create issues should the window creation fail
    {
      static LPRECT mainDrawRect{ nullptr };
      static bool startUpDone{ false };
      if (!mainDrawRect)
      {
        mainDrawRect = lprc;
      }
      else if (mainDrawRect->right != xRight || mainDrawRect->bottom != yBottom)
      {
        if (startUpDone)
        {
          d.scrollMax = { mainDrawRect->right - 1, mainDrawRect->bottom - 1 };
          resChanged = true;
        }
        else
        {
          d.scrollMax = { xRight - 1, yBottom - 1 };
          startUpDone = true;
        }

        mainDrawRect->right = xRight;
        mainDrawRect->bottom = yBottom;
        d.gameTexSize = { xRight, yBottom };
        possibleTexChange = true;
        rectInit = true;
      }
      else if (!rectInit)
      {
        d.scrollMax = { xRight - 1, yBottom - 1 };
        rectInit = true;
        startUpDone = true; // for the case that same size -> if this is reached, not additional handling of start needed
      }
    }

    return SetRect(lprc, xLeft, yTop, xRight, yBottom);
  }


  // this adjusts scrolling only -> maybe use custom code later, this stuff here is kinda horrible
  BOOL CrusaderToOpenGL::getWindowCursorPos(LPPOINT lpPoint)
  {
    if (!d.windowDone)
    {
      return GetCursorPos(lpPoint);
    }

    // deactivate by setting return pos to scroll middle
    // also if no focus and window stops
    if (!confPtr->control.scrollActive || (confPtr->window.continueOutOfFocus == NOFOCUS_CONTINUE && !hasFocus))
    {
      *lpPoint = { d.scrollMax.w / 2, d.scrollMax.h / 2 };
      return true;
    }

    bool success{ GetCursorPos(lpPoint) && ScreenToClient(shcWinStrucPtr->gameWindowHandle, lpPoint) };
    if (success)
    {
      Size<int>& texS{ d.gameTexSize };
      Size<int>& scrollM{ d.scrollMax };

      Size<int> intCursor{ lpPoint->x - d.windowOffset.x, lpPoint->y - d.windowOffset.y };

      // using screen pixels to define ranges
      // margin disabled if the cursor is clipped or no window mode
      int margin{ confPtr->control.clipCursor || confPtr->window.type == TYPE_BORDERLESS_FULLSCREEN ||
        confPtr->window.type == TYPE_FULLSCREEN ? 0 : confPtr->control.margin };
      
      if (intCursor.x < -margin || intCursor.x > d.gameScreenSize.w - 1 + margin ||
        intCursor.y < -margin || intCursor.y > d.gameScreenSize.h - 1 + margin)
      {
        *lpPoint = { scrollM.w / 2, scrollM.h / 2 };
        return true;
      }

      // transform to game
      Size<double> cursor{ static_cast<double>(intCursor.x) * d.winToGamePos.x, static_cast<double>(intCursor.y) * d.winToGamePos.y };

      // if the game never stops, a tab switch will not reset the cursor control
      // this can cause issues if the initial resolution is rather high or vice versa
      if (resChanged /* || confPtr->window.continueOutOfFocus*/)
      {
        cursor = { scrollM.w * cursor.x / (texS.w - 1.0), scrollM.h * cursor.y / (texS.h - 1.0) };
      }

      // increased border -> checking here to allow easier overwrite
      int padding{ confPtr->control.padding };
      if (intCursor.x < padding)
      {
        cursor.x = 0.0;
      }
      else if (intCursor.x > d.gameScreenSize.w - 1 - padding)
      {
        cursor.x = texS.w - 1.0;
      }

      if (intCursor.y < padding)
      {
        cursor.y = 0.0;
      }
      else if (intCursor.y > d.gameScreenSize.h - 1 - padding)
      {
        cursor.y = texS.h - 1.0;
      }

      *lpPoint = { lround(cursor.x), lround(cursor.y) };
    }

    return success;
  }

  BOOL CrusaderToOpenGL::setWindowPosFake(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlag)
  {
    return d.windowDone ? true : SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlag);
  }

  BOOL WINAPI CrusaderToOpenGL::updateWindowFake(HWND hWnd)
  {
    return d.windowDone ? true : UpdateWindow(hWnd);
  }

  BOOL WINAPI CrusaderToOpenGL::adjustWindowRectFake(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
  {
    return d.windowDone ? true : AdjustWindowRect(lpRect, dwStyle, bMenu);
  };

  LONG WINAPI CrusaderToOpenGL::setWindowLongAFake(HWND hWnd, int nIndex, LONG dwNewLong)
  {
    return d.windowDone ? true : SetWindowLongA(hWnd, nIndex, dwNewLong); // 0 could mean error
  }

  HWND WINAPI CrusaderToOpenGL::GetForegroundWindowFake()
  {
    return (confPtr->window.continueOutOfFocus == NOFOCUS_RENDER) ? shcWinStrucPtr->gameWindowHandle : GetForegroundWindow();
  }

  bool CrusaderToOpenGL::transformMouseMovePos(LPARAM* ptrlParam)
  {
    if (!d.windowDone)
    {
      return true;
    }

    // discard if game has no focus and continues without it being rendered
    if (confPtr->window.continueOutOfFocus == NOFOCUS_CONTINUE && !hasFocus)
    {
      return false;
    }

    POINTS mousePos{ MAKEPOINTS(*ptrlParam) };

    // mouse position in game window
    Size<int> gameWinMousePos{ mousePos.x - d.windowOffset.x, mousePos.y - d.windowOffset.y };

    // discard if outside screen
    if (gameWinMousePos.x < 0 || gameWinMousePos.x > d.gameScreenSize.w - 1 ||
      gameWinMousePos.y < 0 || gameWinMousePos.y > d.gameScreenSize.h - 1)
    {
      return false;
    }

    // no tranform needed if crusader screen size equal to game window
    if (d.gameTexSize.w == d.windowSize.w && d.gameTexSize.h == d.windowSize.h)
    {
      return true;
    }

    // transform and return
    mousePos = { static_cast<short>(lround(gameWinMousePos.x * d.winToGamePos.x)), static_cast<short>(lround(gameWinMousePos.y * d.winToGamePos.y)) };
    *ptrlParam = MAKELPARAM(mousePos.x, mousePos.y);
    return true;
  }

  bool CrusaderToOpenGL::windowLostFocus()
  {
    if (!d.windowDone)
    {
      return true;
    }

    // found no other way to proper minimize
    if (confPtr->window.type == TYPE_BORDERLESS_FULLSCREEN || confPtr->window.type == TYPE_FULLSCREEN)
    {
      ShowWindow(shcWinStrucPtr->gameWindowHandle, SW_MINIMIZE);
    }

    hasFocus = false;
    rectInit = false;

    if (confPtr->control.clipCursor)
    {
      ClipCursor(NULL); // free cursor
      cursorClipped = false;
    }

    return !(confPtr->window.continueOutOfFocus); // if zero (NOFOCUS_PAUSE), continue
  }

  bool CrusaderToOpenGL::windowSetFocus()
  {
    if (!d.windowDone)
    {
      return true;
    }

    hasFocus = true;
    resChanged = false;

    if (confPtr->window.continueOutOfFocus == NOFOCUS_CONTINUE)
    {
      devourAfterFocus = true;
    }

    // because of interaction with window border needs other handling
    if (confPtr->window.type != TYPE_WINDOW && confPtr->control.clipCursor)
    {
      clipCursor();
    }

    return !(confPtr->window.continueOutOfFocus); // if zero (NOFOCUS_PAUSE), continue
  }

  bool CrusaderToOpenGL::windowActivated(bool active) // nothing currently
  {
    if (!confPtr->window.continueOutOfFocus)
    {
      return true;
    }

    // for continue running config
    static bool onceActivated{ false };
    if (onceActivated)
    {
      return false;
    }
    onceActivated = true;
    return true;
  }

  void CrusaderToOpenGL::windowDestroyed()
  {
    // a final free action
    if (confPtr->control.clipCursor)
    {
      ClipCursor(NULL);
      cursorClipped = false;
    }

    // windows spoke: https://docs.microsoft.com/en-us/windows/win32/opengl/deleting-a-rendering-context
    window->releaseContext(shcWinStrucPtr->gameWindowHandle);
  }

  void CrusaderToOpenGL::windowEditEnded()  // currently unused
  {
  }


  bool CrusaderToOpenGL::mouseDown()
  {
    bool ret{ true };

    if (confPtr->window.type == TYPE_WINDOW && confPtr->control.clipCursor)
    {
      if (!cursorClipped && hasFocus)
      {
        clipCursor();
      }
    }

    if (confPtr->window.continueOutOfFocus == NOFOCUS_CONTINUE && devourAfterFocus)
    {
      devourAfterFocus = false;
      ret = ret && false;
    }

    return ret;
  }


  /** internal helper functions **/
  
  // NOTE: currently unused
  void CrusaderToOpenGL::setWindowStyleAndSize()
  {
    RECT newWinRect{ GetWindowRect(confPtr->window) };
    d.windowSize = { GetGameWidth(confPtr->window), GetGameHeight(confPtr->window) };

    // this would set a new style and adjust the window
    // however, screenshots still do not work
    DWORD newStyle{ GetWindowStyle(confPtr->window.type) };
    DWORD newExStyle{ GetExtendedWindowStyle(confPtr->window.type) };

    HWND handle{ shcWinStrucPtr->gameWindowHandle };
    SetWindowLongPtr(handle, GWL_STYLE, newStyle);
    SetWindowLongPtr(handle, GWL_EXSTYLE, newExStyle);
    SetWindowPos(handle, HWND_TOP, newWinRect.left, newWinRect.top, newWinRect.right - newWinRect.left,
      newWinRect.bottom - newWinRect.top, SWP_SHOWWINDOW | SWP_FRAMECHANGED);

    // other possible stuff:
    //GetWindowRect(winHandle, &newWinRect);
    //MoveWindow(winHandle, newWinRect.left, newWinRect.top, newWinRect.right - newWinRect.left, newWinRect.bottom - newWinRect.top, true);
  }


  void CrusaderToOpenGL::clipCursor()
  {
    HWND handle{ shcWinStrucPtr->gameWindowHandle };
    RECT winPos{};
    GetClientRect(handle, &winPos);
    MapWindowPoints(handle, NULL, (LPPOINT)&winPos, 2);
    winPos.left += d.windowOffset.w;
    winPos.right -= d.windowOffset.w;
    winPos.top += d.windowOffset.h;
    winPos.bottom -= d.windowOffset.h;
    ClipCursor(&winPos); // clip cursor
    cursorClipped = true;
  }
}