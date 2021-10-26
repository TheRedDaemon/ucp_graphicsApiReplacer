
#include "pch.h"


#include "windowCore.h"
#include "shcRelatedStructures.h"
#include "crusaderToOpenGL.h"
#include "configUtil.h"

// included explicitly, since it is one of the by WIN32_LEAN_AND_MEAN removed headers
// needed to additionally link with Winmm.lib
#include <timeapi.h>


namespace UCPtoOpenGL
{
  CrusaderToOpenGL::CrusaderToOpenGL(ToOpenGLConfig& conf) : confRef{ conf }, window{ std::make_unique<WindowCore>() } {};
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
    return confRef.graphic.pixFormat;
  }


  // DirectDraw

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::EnumDisplayModes(DWORD, LPDDSURFACEDESC,
    LPVOID, LPDDENUMMODESCALLBACK)
  {
    // ignore callback, just fill struct:
    for (size_t i = 1; i < RESOLUTIONS.size() - 1; i++) // 0 and the last are not available
    {
      // this res has an issue with rendering the endscreen, the elements are too close, you can not leave the screen
      // I would assume the menu positions are created for RES_1024_X_768
      // the lose screen func as two flags set for 800x600, maybe this would need to be set for this also?
      // anyway, this res is functionally broken and I am not sure if this is an issue of my code
      // func in crusader: 0x004d55d0, flag based on res in that func: 0x004d5629
      if (i == RES_1024_X_600)
      {
        continue; // -> disabled
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
    d.gameTexSize = { (int)width, (int)height };
    d.scrollRange = { (int)width - 1, (int)height - 1 };

    //create new bit maps
    int pixNum{ d.gameTexSize.w * d.gameTexSize.h };
    back.createBitData(pixNum);
    offMain.createBitData(pixNum);

    // change scale
    // I choose the easy route, algorithm: https://math.stackexchange.com/a/1620375
      
    // compute needed base values
    Size<double> winToTex{ static_cast<double>(d.gameTexSize.w) / d.windowSize.w, static_cast<double>(d.gameTexSize.h) / d.windowSize.h };
    double winToTexMult = winToTex.h > winToTex.w ? winToTex.h : winToTex.w;
    Size<double> winScale{ winToTex.w / winToTexMult, winToTex.h / winToTexMult };
      
    // offset in window pixels
    d.windowOffset = { lround((1.0 - winScale.w) * d.windowSize.w / 2.0), lround((1.0 - winScale.h) * d.windowSize.h / 2.0) };

    // size of the game in the window
    d.gameWindowRange = { d.windowSize.w - d.windowOffset.w * 2 - 1, d.windowSize.h - d.windowOffset.h * 2 - 1 };

    // for positions, I need to use the relation between the range ( 0 -> width - 1 ) of the window and the game
    d.winToGamePos = { static_cast<double>(d.scrollRange.w) / d.gameWindowRange.w, static_cast<double>(d.scrollRange.h) / d.gameWindowRange.h };

    window->adjustTexSizeAndViewport(d.gameTexSize, d.windowSize, winScale);


    // set clip for cursor
    if (confRef.control.clipCursor && confRef.window.type != TYPE_WINDOW)
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




  // detoured calls

  // complete
  void __thiscall CrusaderToOpenGL::createWindow(WNDPROC windowCallbackFunc, SHCWindowOrMainStructFake* that,
    HINSTANCE hInstance, LPSTR windowName, unsigned int cursorResource)
  {
    // removed original shc window init, if interested, check older git versions

    // keep ref
    shcWinStrucPtr = that;
    shcWinStrucPtr->gameHInstance = hInstance;
    shcWinStrucPtr->gameWindowHandle = NULL;

    // trying to get WGL functions, if this fails, return false, Crusader crashes
    if (!window->loadWGLFunctions(hInstance))
    {
      return;
    }

    LPCSTR className{ "FFwinClass" };

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
      return;
    }

    RECT winRect{ GetWindowRect(confRef.window) };
    d.windowSize = { GetGameWidth(confRef.window), GetGameHeight(confRef.window) };

    shcWinStrucPtr->gameWindowHandle = CreateWindowExA(
      GetExtendedWindowStyle(confRef.window.type),
      className,
      windowName,
      GetWindowStyle(confRef.window.type),
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
    if (handle == NULL)
    {
      return;
    }

    window->setConf(&confRef);
    d.windowDone = handle && window->createWindow(handle);

    HRESULT res{ CoInitialize(NULL) };  // ignoring res, like SHC
    shcWinStrucPtr->windowCreationTime = timeGetTime();

    // no other action should be needed -> missing actions from orig are repeated during DirectDraw Setup anyway
  }


  void CrusaderToOpenGL::drawInit(DWORD winSetRectObjBaseAddr, SHCWindowOrMainStructFake* that)
  {
    // at this point, create window should be there, but still
    if (shcWinStrucPtr != that)
    {
      shcWinStrucPtr = that;
    }
    SHCWindowOrMainStructFake& mainStruct{ *shcWinStrucPtr };
    Size<int>& texS{ d.gameTexSize };
    texS = { getFakeSystemMetrics(SM_CXSCREEN), getFakeSystemMetrics(SM_CYSCREEN) };

    // set getDeviceCaps values
    mainStruct.colorDepth = 0x10;  // simply to 16
    mainStruct.screenWidthInPixels = texS.w; // to current res -> defaults to 1280x720
    mainStruct.screenHeightInPixels = texS.h;
    mainStruct.runGameAsExclusiveFullscreen = 1;

    // set setWindowStyleAndRect values:
    mainStruct.clientOnScreenCoords = { 0, 0, texS.w, texS.h };

    // these values might not be used, set them anyway for now
    int* resRect{ (int*)(winSetRectObjBaseAddr + 0x30) };
    *resRect = 0;
    *(resRect + 1) = texS.w;
    *(resRect + 2) = 0;
    *(resRect + 3) = texS.h;
    mainStruct.gameInWindowPosX = 0;
    mainStruct.gameInWindowPosY = 0;
    mainStruct.gameOnScreenPosX = 0;
    mainStruct.gameOnScreenPosX = 0;

    // DirectDrawCreate
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
    // if no infomration was received, the return will be the RES_1280_X_720
    if (nIndex == SM_CXSCREEN || nIndex == SM_CYSCREEN)
    {
      GameResolution currRes{ GameResolution::RES_1280_X_720 };
      if (shcWinStrucPtr && shcWinStrucPtr->currentGameResolution != GameResolution::RES_NONE)
      {
        currRes = shcWinStrucPtr->currentGameResolution;
      }

      return nIndex == SM_CXSCREEN ? RESOLUTIONS[currRes][0] : RESOLUTIONS[currRes][1];
    }

    return GetSystemMetrics(nIndex);
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
    if (!confRef.control.scrollActive || (confRef.window.continueOutOfFocus == NOFOCUS_CONTINUE && !d.hasFocus))
    {
      *lpPoint = { d.scrollRange.w / 2, d.scrollRange.h / 2 };
      return true;
    }

    bool success{ GetCursorPos(lpPoint) && ScreenToClient(shcWinStrucPtr->gameWindowHandle, lpPoint) };
    if (success)
    {
      Size<int>& texS{ d.gameTexSize };
      Size<int>& scrollM{ d.scrollRange };

      Size<int> intCursor{ lpPoint->x - d.windowOffset.x, lpPoint->y - d.windowOffset.y };

      // using screen pixels to define ranges
      // margin disabled if the cursor is clipped or no window mode
      int margin{ confRef.control.clipCursor || confRef.window.type == TYPE_BORDERLESS_FULLSCREEN ||
        confRef.window.type == TYPE_FULLSCREEN ? 0 : confRef.control.margin };
      
      if (intCursor.x < -margin || intCursor.x > d.gameWindowRange.w + margin ||
        intCursor.y < -margin || intCursor.y > d.gameWindowRange.h + margin)
      {
        *lpPoint = { scrollM.w / 2, scrollM.h / 2 };
        return true;
      }

      // transform to game
      Size<double> cursor{ static_cast<double>(intCursor.x) * d.winToGamePos.x, static_cast<double>(intCursor.y) * d.winToGamePos.y };

      // increased border -> checking here to allow easier overwrite
      int padding{ confRef.control.padding };
      if (intCursor.x < padding)
      {
        cursor.x = 0.0;
      }
      else if (intCursor.x > d.gameWindowRange.w - padding)
      {
        cursor.x = texS.w - 1.0;
      }

      if (intCursor.y < padding)
      {
        cursor.y = 0.0;
      }
      else if (intCursor.y > d.gameWindowRange.h - padding)
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
    return (confRef.window.continueOutOfFocus == NOFOCUS_RENDER) ? shcWinStrucPtr->gameWindowHandle : GetForegroundWindow();
  }

  bool CrusaderToOpenGL::transformMouseMovePos(LPARAM* ptrlParam)
  {
    if (!d.windowDone)
    {
      return true;
    }

    // discard if game has no focus and continues without it being rendered
    if (confRef.window.continueOutOfFocus == NOFOCUS_CONTINUE && !d.hasFocus)
    {
      return false;
    }

    POINTS mousePos{ MAKEPOINTS(*ptrlParam) };

    // mouse position in game window
    Size<int> gameWinMousePos{ mousePos.x - d.windowOffset.x, mousePos.y - d.windowOffset.y };

    // discard if outside screen
    if (gameWinMousePos.x < 0 || gameWinMousePos.x > d.gameWindowRange.w ||
      gameWinMousePos.y < 0 || gameWinMousePos.y > d.gameWindowRange.h)
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
    d.hasFocus = false;

    // found no other way to proper minimize
    if (confRef.window.type == TYPE_BORDERLESS_FULLSCREEN || confRef.window.type == TYPE_FULLSCREEN)
    {
      ShowWindow(shcWinStrucPtr->gameWindowHandle, SW_MINIMIZE);
    }

    if (confRef.control.clipCursor)
    {
      ClipCursor(NULL); // free cursor
      d.cursorClipped = false;
    }

    return !(confRef.window.continueOutOfFocus); // if zero (NOFOCUS_PAUSE), continue
  }

  bool CrusaderToOpenGL::windowSetFocus()
  {
    if (!d.windowDone)
    {
      return true;
    }
    d.hasFocus = true;

    if (confRef.window.continueOutOfFocus == NOFOCUS_CONTINUE)
    {
      d.devourAfterFocus = true;
    }

    // because of interaction with window border needs other handling
    if (confRef.window.type != TYPE_WINDOW && confRef.control.clipCursor)
    {
      clipCursor();
    }

    return !(confRef.window.continueOutOfFocus); // if zero (NOFOCUS_PAUSE), continue
  }

  bool CrusaderToOpenGL::windowActivated(bool active) // nothing currently
  {
    if (active && shcWinStrucPtr) // setting to process input, to ignore window again request
    {
      shcWinStrucPtr->isNotProcessingInputEvents = 0;
      SetFocus(shcWinStrucPtr->gameWindowHandle);
    }

    if (!confRef.window.continueOutOfFocus)
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
    if (confRef.control.clipCursor)
    {
      ClipCursor(NULL);
      d.cursorClipped = false;
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

    if (confRef.window.type == TYPE_WINDOW && confRef.control.clipCursor)
    {
      if (!d.cursorClipped && d.hasFocus)
      {
        clipCursor();
      }
    }

    if (confRef.window.continueOutOfFocus == NOFOCUS_CONTINUE && d.devourAfterFocus)
    {
      d.devourAfterFocus = false;
      ret = ret && false;
    }

    return ret;
  }


  /** internal helper functions **/
  
  // NOTE: currently unused
  void CrusaderToOpenGL::setWindowStyleAndSize()
  {
    RECT newWinRect{ GetWindowRect(confRef.window) };
    d.windowSize = { GetGameWidth(confRef.window), GetGameHeight(confRef.window) };

    // this would set a new style and adjust the window
    // however, screenshots still do not work
    DWORD newStyle{ GetWindowStyle(confRef.window.type) };
    DWORD newExStyle{ GetExtendedWindowStyle(confRef.window.type) };

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
    d.cursorClipped = true;
  }
}