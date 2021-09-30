
#include "pch.h"

#include <string>

#include "windowCore.h"
#include "fakeSurfaces.h"
#include "crusaderToOpenGL.h"

#include "configUtil.h"


namespace UCPtoOpenGL
{

  // lua calls

  bool CrusaderToOpenGL::createWindow(DWORD that, LPSTR windowName, unsigned int unknown, WNDPROC windowCallbackFunc)
  {
    if (!confPtr) // no config, so everything normal
    {
      // recreated original window func
      HINSTANCE hInstance{ *(HINSTANCE*)(that + 0xA8) };
      LPCSTR className{ "FFwinClass" };

      WNDCLASSA wndClass;
      wndClass.hInstance = hInstance;
      wndClass.lpfnWndProc = windowCallbackFunc;
      wndClass.style = NULL;	// this needs changes later
      wndClass.cbClsExtra = NULL;
      wndClass.cbWndExtra = NULL;
      wndClass.hIcon = LoadIconA(hInstance, (LPCSTR)(unknown & 0xFFFF));	// weird... is this a downcast, so unknown would be an address?
      wndClass.hCursor = NULL;
      wndClass.hbrBackground = NULL;
      wndClass.lpszMenuName = NULL;
      wndClass.lpszClassName = className;

      ATOM classAtom{ RegisterClassA(&wndClass) };
      if (classAtom == NULL)
      {
        return false;
      }

      winHandle = CreateWindowExA(
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

      *(DWORD*)(that + 0xAC) = (DWORD)winHandle;
      return winHandle != NULL;
    }
    else
    {
      // try to change stuff
      HINSTANCE hInstance{ *(HINSTANCE*)(that + 0xA8) };
      LPCSTR className{ "FFwinClass" };


      // trying to get WGL functions, if this fails, return false, Crusader crashes
      if (!window.loadWGLFunctions(hInstance))
      {
        return false;
      }


      WNDCLASSA wndClass;
      wndClass.hInstance = hInstance;
      wndClass.lpfnWndProc = windowCallbackFunc;
      wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;	// CS_OWNDC apperantly needed to allow a constant device context CS_HREDRAW | CS_VREDRAW
      wndClass.cbClsExtra = NULL;
      wndClass.cbWndExtra = NULL;
      wndClass.hIcon = LoadIconA(hInstance, (LPCSTR)(unknown & 0xFFFF));	// weird... is this a downcast, so unknown would be a address?
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
      gameWinSizeW = GetGameWidth(confPtr->window);
      gameWinSizeH = GetGameHeight(confPtr->window);

      winHandle = CreateWindowExA(
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

      // this is an issue -> if something here fails, then the whole thing might be busted...
      // TODO: is there a possible way around? -> close everything until then, then recreate with original Stronghold?
      // until then, this will return false, and I assume stronghold will close (or crash, not tested here yet)
      window.setConf(confPtr);
      windowDone = winHandle && window.createWindow(winHandle);

      // give pixel format to
      offMain.setPixelFormat(confPtr->graphic.pixFormat);
      back.setPixelFormat(confPtr->graphic.pixFormat);

      // failing at this part can will cause issues

      // end -> do not touch
      *(DWORD*)(that + 0xAC) = (DWORD)winHandle;
      return winHandle != NULL;
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
    if (windowDone)
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
    if (windowDone)
    {
      switch (nIndex)
      {
      case SM_CXSCREEN:
      {
        int texWidth{ window.getTexStrongSizeW() };
        if (texWidth > 0)
        {
          return texWidth;
        }
        break;
      }
      case SM_CYSCREEN:
      {
        int texHeight{ window.getTexStrongSizeH() };
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
          scrollMaxW = mainDrawRect->right - 1;
          scrollMaxH = mainDrawRect->bottom - 1;
          resChanged = true;
        }
        else
        {
          scrollMaxW = xRight - 1;;
          scrollMaxH = yBottom - 1;;
          startUpDone = true;
        }

        mainDrawRect->right = xRight;
        mainDrawRect->bottom = yBottom;
        window.setOnlyTexSize(xRight, yBottom);
        possibleTexChange = true;
        rectInit = true;
      }
      else if (!rectInit)
      {
        scrollMaxW = xRight - 1;
        scrollMaxH = yBottom - 1;
        rectInit = true;
        startUpDone = true; // for the case that same size -> if this is reached, not additional handling of start needed
      }
    }

    return SetRect(lprc, xLeft, yTop, xRight, yBottom);
  }


  // this adjusts scrolling only -> maybe use custom code later, this stuff here is kinda horrible
  BOOL CrusaderToOpenGL::getWindowCursorPos(LPPOINT lpPoint)
  {
    if (!windowDone)
    {
      return GetCursorPos(lpPoint);
    }

    // deactivate by setting return pos to scroll middle
    if (!confPtr->control.scrollActive)
    {
      lpPoint->x = scrollMaxW / 2;
      lpPoint->y = scrollMaxH / 2;
      return true;
    }

    bool success{ GetCursorPos(lpPoint) && ScreenToClient(winHandle, lpPoint) };
    if (success)
    {
      int intCursorX{ lpPoint->x - winOffsetW };
      int intCursorY{ lpPoint->y - winOffsetH };

      // using screen pixels to define ranges
      // margin disabled if the cursor is clipped or no window mode
      int margin{ confPtr->control.clipCursor || confPtr->window.type == TYPE_BORDERLESS_FULLSCREEN ||
        confPtr->window.type == TYPE_FULLSCREEN ? 0 : confPtr->control.margin };
      if (intCursorX < -margin || intCursorX > gameScreenSizeW - 1 + margin || intCursorY < -margin || intCursorY > gameScreenSizeH - 1 + margin)
      {
        lpPoint->x = scrollMaxW / 2;
        lpPoint->y = scrollMaxH / 2;
        return true;
      }

      // transform to game
      double cursorX{ static_cast<double>(intCursorX) * winToGamePosX };
      double cursorY{ static_cast<double>(intCursorY) * winToGamePosY };

      if (resChanged)
      {
        cursorX = scrollMaxW * cursorX / (window.getTexStrongSizeW() - 1.0);
        cursorY = scrollMaxH * cursorY / (window.getTexStrongSizeH() - 1.0);
      }

      // increased border -> checking here to allow easier overwrite
      int padding{ confPtr->control.padding };
      if (intCursorX < padding)
      {
        cursorX = 0.0;
      }
      else if (intCursorX > gameScreenSizeW - 1 - padding)
      {
        cursorX = window.getTexStrongSizeW() - 1.0;
      }

      if (intCursorY < padding)
      {
        cursorY = 0.0;
      }
      else if (intCursorY > gameScreenSizeH - 1 - padding)
      {
        cursorY = window.getTexStrongSizeH() - 1.0;
      }

      lpPoint->x = lround(cursorX);
      lpPoint->y = lround(cursorY);
    }

    return success;
  }

  BOOL CrusaderToOpenGL::setWindowPosFake(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlag)
  {
    return windowDone ? true : SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlag);
  }

  BOOL WINAPI CrusaderToOpenGL::updateWindowFake(HWND hWnd)
  {
    return windowDone ? true : UpdateWindow(hWnd);
  }

  BOOL WINAPI CrusaderToOpenGL::adjustWindowRectFake(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
  {
    return windowDone ? true : AdjustWindowRect(lpRect, dwStyle, bMenu);
  };

  LONG WINAPI CrusaderToOpenGL::setWindowLongAFake(HWND hWnd, int nIndex, LONG dwNewLong)
  {
    return windowDone ? true : SetWindowLongA(hWnd, nIndex, dwNewLong); // 0 could mean error
  }

  bool CrusaderToOpenGL::transformMouseMovePos(LPARAM* ptrlParam)
  {
    if (!windowDone)
    {
      return true;
    }

    POINTS mousePos{ MAKEPOINTS(*ptrlParam) };

    // mouse position in game window
    int gameWinMousePosX{ mousePos.x - winOffsetW }; 
    int gameWinMousePosY{ mousePos.y - winOffsetH };

    // discard if outside screen
    if (gameWinMousePosX < 0 || gameWinMousePosX > gameScreenSizeW - 1 ||
      gameWinMousePosY < 0 || gameWinMousePosY > gameScreenSizeH - 1)
    {
      return false;
    }

    int texW{ window.getTexStrongSizeW() };
    int texH{ window.getTexStrongSizeH() };

    // no tranform needed if crusader screen size equal to game window
    if (texW == gameWinSizeW && texH == gameWinSizeH)
    {
      return true;
    }

    // transform and return
    mousePos.x = static_cast<short>(lround(gameWinMousePosX * winToGamePosX)); // just hoping that it does not end at texW/texH + 1
    mousePos.y = static_cast<short>(lround(gameWinMousePosY * winToGamePosY));
    *ptrlParam = MAKELPARAM(mousePos.x, mousePos.y);
    return true;
  }

  void CrusaderToOpenGL::windowLostFocus()
  {
    if (!windowDone)
    {
      return;
    }

    // found no other way to proper minimize
    if (confPtr->window.type == TYPE_BORDERLESS_FULLSCREEN || confPtr->window.type == TYPE_FULLSCREEN)
    {
      ShowWindow(winHandle, SW_MINIMIZE);
    }

    hasFocus = false;
    rectInit = false;

    if (confPtr->control.clipCursor)
    {
      ClipCursor(NULL); // free cursor
      cursorClipped = false;
    }
  }

  void CrusaderToOpenGL::windowSetFocus()
  {
    if (!windowDone)
    {
      return;
    }

    hasFocus = true;
    resChanged = false;

    // because of interaction with window border needs other handling
    if (confPtr->window.type != TYPE_WINDOW && confPtr->control.clipCursor)
    {
      clipCursor();
    }
  }

  void CrusaderToOpenGL::windowActivated(bool active) // nothing currently
  {
    if (active)
    {
    }
    else
    {
    }
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
    window.releaseContext(winHandle);
  }

  void CrusaderToOpenGL::windowEditEnded()  // currently unused
  {
  }


  void CrusaderToOpenGL::mouseDown()
  {
    if (confPtr->window.type != TYPE_WINDOW || !confPtr->control.clipCursor)
    {
      return;
    }

    if (!cursorClipped && hasFocus)
    {
      clipCursor();
    }
  }


  // DirectDraw

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::EnumDisplayModes(DWORD, LPDDSURFACEDESC,
    LPVOID lpvoid, LPDDENUMMODESCALLBACK callback)
  {
    // an other method could be to get every possible mode from crusader and send them back, but
    // here will do it for now

    DWORD modeNum{ 0 };
    DEVMODEA displaySettings{};
    displaySettings.dmSize = sizeof(DEVMODEA);
    DDSURFACEDESC des{};
    
    DWORD neededFlags{ DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY };
    while (EnumDisplaySettingsExA(NULL, modeNum, &displaySettings, EDS_RAWMODE))
    {
      // check if all required flags set and if the mode is 16 bit, because crusader is 16 bit anyway
      if ((displaySettings.dmFields & neededFlags) == neededFlags && displaySettings.dmBitsPerPel == 16
        && displaySettings.dmDisplayFixedOutput == 0) // only need one version of a resolution
      {
        // clean structure
        ZeroMemory(&des, sizeof(des));
        des.dwSize = sizeof(des);

        // set content
        des.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_REFRESHRATE | DDSD_PIXELFORMAT;
        
        /* this would add 2560 instead of 1080
        if (displaySettings.dmPelsHeight == 1080)
        {
          displaySettings.dmPelsHeight = 1440;
          displaySettings.dmPelsWidth = 2560;
        }
        */

        des.dwHeight = displaySettings.dmPelsHeight;
        des.dwWidth = displaySettings.dmPelsWidth;
        des.lPitch = 2 * des.dwWidth;  // lPitch is the number of bytes from the start of one line to the next
        des.dwRefreshRate = displaySettings.dmDisplayFrequency;

        FakeSurface::fillPixelFormat(&des.ddpfPixelFormat, confPtr->graphic.pixFormat);

        callback(&des, lpvoid); // send to callback
      }

      // reset data and use next mode
      ZeroMemory(&displaySettings, sizeof(DEVMODEA));
      displaySettings.dmSize = sizeof(DEVMODEA);
      ++modeNum;
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
    if (possibleTexChange || window.getTexStrongSizeW() == 0 || window.getTexStrongSizeH() == 0)
    {
      int wWin{ gameWinSizeW };
      int hWin{ gameWinSizeH };
      int wTex{ (int)width };
      int hTex{ (int)height };

      //create new bit maps
      int pixNum{ wTex * hTex };
      back.createBitData(pixNum);
      offMain.createBitData(pixNum);

      // change scale
      // I choose the easy route, algorithm: https://math.stackexchange.com/a/1620375
      double winToTexW = static_cast<double>(wTex) / wWin;
      double winToTexH = static_cast<double>(hTex) / hWin;
      
      bool vertScale{ winToTexH > winToTexW };
      winToTexMult = vertScale ? winToTexH : winToTexW;
      double winScaleW = winToTexW / winToTexMult;
      double winScaleH = winToTexH / winToTexMult;
      winOffsetW = lround((1.0 - winScaleW) * wWin / 2.0);
      winOffsetH = lround((1.0 - winScaleH) * hWin / 2.0);

      gameScreenSizeW = gameWinSizeW - winOffsetW * 2;
      gameScreenSizeH = gameWinSizeH - winOffsetH * 2;
      // for positions, I need to use the relation between the size of the game in the window, and the tex (the pixel size of the game)
      winToGamePosX = (static_cast<double>(wTex) - 1.0) / (gameScreenSizeW - 1.0);
      winToGamePosY = (static_cast<double>(hTex) - 1.0) / (gameScreenSizeH - 1.0);

      window.adjustTexSizeAndViewport(wTex, hTex, wWin, hWin, winScaleW, winScaleH);

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



  /** internal helper functions **/
  
  // NOTE: currently unused
  void CrusaderToOpenGL::setWindowStyleAndSize()
  {
    RECT newWinRect{ GetWindowRect(confPtr->window) };
    gameWinSizeW = GetGameWidth(confPtr->window);
    gameWinSizeH = GetGameHeight(confPtr->window);

    // this would set a new style and adjust the window
    // however, screenshots stil do not work
    DWORD newStyle{ GetWindowStyle(confPtr->window.type) };
    DWORD newExStyle{ GetExtendedWindowStyle(confPtr->window.type) };
    SetWindowLongPtr(winHandle, GWL_STYLE, newStyle);
    SetWindowLongPtr(winHandle, GWL_EXSTYLE, newExStyle);

    SetWindowPos(winHandle, HWND_TOP, newWinRect.left, newWinRect.top, newWinRect.right - newWinRect.left,
      newWinRect.bottom - newWinRect.top, SWP_SHOWWINDOW | SWP_FRAMECHANGED);

    // other possible stuff:
    //GetWindowRect(winHandle, &newWinRect);
    //MoveWindow(winHandle, newWinRect.left, newWinRect.top, newWinRect.right - newWinRect.left, newWinRect.bottom - newWinRect.top, true);
  }


  void CrusaderToOpenGL::clipCursor()
  {
    RECT winPos{};
    GetClientRect(winHandle, &winPos);
    MapWindowPoints(winHandle, NULL, (LPPOINT)&winPos, 2);
    winPos.left += winOffsetW;
    winPos.right -= winOffsetW;
    winPos.top += winOffsetH;
    winPos.bottom -= winOffsetH;
    ClipCursor(&winPos); // clip cursor
    cursorClipped = true;
  }
}