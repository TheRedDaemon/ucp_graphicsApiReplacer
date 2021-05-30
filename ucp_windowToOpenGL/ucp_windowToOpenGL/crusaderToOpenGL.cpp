
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
      gameSizeW = GetGameWidth(confPtr->window);
      gameSizeH = GetGameHeight(confPtr->window);

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
      // until then, this will return false, and I assume stronghold will close
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
      // use own if successful
      res = create(lpGUID, &realInterface, pUnkOuter);
      *lplpDD = this;
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
          scrollSizeW = mainDrawRect->right;
          scrollSizeH = mainDrawRect->bottom;
          resChanged = true;
        }
        else
        {
          scrollSizeW = xRight;
          scrollSizeH = yBottom;
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
        scrollSizeW = xRight;
        scrollSizeH = yBottom;
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

    bool success{ GetCursorPos(lpPoint) && ScreenToClient(winHandle, lpPoint) };
    if (success)
    {
      int texW{ window.getTexStrongSizeW() };
      int texH{ window.getTexStrongSizeH() };

      double cursorX{ (static_cast<double>(lpPoint->x) - winOffsetW) * winToTexMult };
      double cursorY{ (static_cast<double>(lpPoint->y) - winOffsetH) * winToTexMult };

      if (resChanged)
      {
        // need to adapt to tex/scroll differences after resolution change
        cursorX *= static_cast<double>(scrollSizeW) / texW;
        cursorY *= static_cast<double>(scrollSizeH) / texH;
      }

      // range limit test
      if (cursorX < -5.0 || cursorX > scrollSizeW + 5.0 || cursorY < -5.0 || cursorY > scrollSizeH + 5.0)
      {
        cursorX = static_cast<double>(scrollSizeW) / 2.0;
        cursorY = static_cast<double>(scrollSizeH) / 2.0;
      }
      else
      {
        // increased border test
        if (cursorX < 10.0)
        {
          cursorX = 0;
        }
        else if (cursorX > scrollSizeW - 10.0)
        {
          cursorX = scrollSizeW;
        }

        if (cursorY < 10.0)
        {
          cursorY = 0;
        }
        else if (cursorY > scrollSizeH - 10.0)
        {
          cursorY = scrollSizeH;
        }
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

  LPARAM CrusaderToOpenGL::transformMouseMovePos(LPARAM lParam)
  {
    if (!windowDone)
    {
      return lParam;
    }

    int texW{ window.getTexStrongSizeW() };
    int texH{ window.getTexStrongSizeH() };
    if (texW == gameSizeW && texH == gameSizeH)
    {
      return lParam;
    }

    POINTS mousePos{ MAKEPOINTS(lParam) };
    mousePos.x = static_cast<short>(lround((static_cast<double>(mousePos.x) - winOffsetW) * winToTexMult));
    mousePos.y = static_cast<short>(lround((static_cast<double>(mousePos.y) - winOffsetH) * winToTexMult));
    return MAKELPARAM(mousePos.x, mousePos.y);
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

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::SetDisplayMode(DWORD width, DWORD height, DWORD)
  {
    // in-theory, the earliest to adapt to screen changes where the tex heights are known is now the
    // setRectFake function on the second call per request

    // execute either if hint received or if the tex size is not initialized
    if (possibleTexChange || window.getTexStrongSizeW() == 0 || window.getTexStrongSizeH() == 0)
    {
      int wWin{ gameSizeW };
      int hWin{ gameSizeH };
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
      winToTexMult = winToTexH > winToTexW ? winToTexH : winToTexW;
      double winScaleW = winToTexW / winToTexMult;
      double winScaleH = winToTexH / winToTexMult;
      winOffsetW = lround((1.0 - winScaleW) * wWin / 2.0);
      winOffsetH = lround((1.0 - winScaleH) * hWin / 2.0);

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
    gameSizeW = GetGameWidth(confPtr->window);
    gameSizeH = GetGameHeight(confPtr->window);

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