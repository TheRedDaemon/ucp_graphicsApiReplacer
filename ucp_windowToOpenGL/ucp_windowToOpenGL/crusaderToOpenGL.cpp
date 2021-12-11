
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
      Log(LOG_FATAL, "[graphicsApiReplacer]: Preinitialization of graphics API failed. Aborting game.");
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
      Log(LOG_FATAL, "[graphicsApiReplacer]: Failed to register window class. Aborting game.");
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
      Log(LOG_FATAL, "[graphicsApiReplacer]: Failed to create window. Aborting game.");
      return;
    }

    window->setConf(&confRef);
    if (!window->createWindow(handle))
    {
      Log(LOG_FATAL, "[graphicsApiReplacer]: Failed to initialize graphics API. Aborting game.");
      return;
    }
    d.windowDone = true;

    HRESULT res{ CoInitialize(NULL) };  // ignoring res, like SHC
    shcWinStrucPtr->windowCreationTime = timeGetTime();

    // set default res here if not given
    if (shcWinStrucPtr->currentGameResolution == GameResolution::RES_NONE)
    {
      shcWinStrucPtr->currentGameResolution = GameResolution::RES_1280_X_720;
    }

    // set supported res
    for (size_t i = 1; i < RESOLUTIONS.size() - 1; i++) // 0 and the last are not available
    {
      // RES_1024_X_600 has an issue with rendering the endscreen, the elements are too close, you can not leave the screen
      // menues seem to not work for this res; one func in crusader: 0x004d55d0, flag based on res in that func: 0x004d5629

      // RES_2560_X_1600 has an issue with activating HUD-less mode on 2560x1600 while zoomed out, it crashes the game
      // the address points in the map rendering, although it seems it crashed first for the hight map (extreme: 0x004E9B78, normal: 0x004e97e8 (maybe))
      if (i == RES_1024_X_600 || i == RES_2560_X_1600)
      {
        continue; // -> disabled, because broken
      }
      shcWinStrucPtr->resolutionSupported[i] = 1;
    }

    Log(LOG_INFO, "[graphicsApiReplacer]: Window created.");
  }


  void CrusaderToOpenGL::drawInit(SetSomeColors colorFunc, SHCBinkControlStructFake* binkStruct,
    DWORD winSetRectObjBaseAddr, SHCWindowOrMainStructFake* that)
  {
    // at this point, create window should be there, but still
    if (shcWinStrucPtr != that)
    {
      shcWinStrucPtr = that;
    }
    SHCWindowOrMainStructFake& mainStruct{ *shcWinStrucPtr };

    /* Set up crusader to openGL + values */

    d.gameTexSize = { getFakeSystemMetrics(SM_CXSCREEN), getFakeSystemMetrics(SM_CYSCREEN) };
    d.scrollRange = { d.gameTexSize.w - 1, d.gameTexSize.h - 1 };

    // change scale, I choose the easy route, algorithm: https://math.stackexchange.com/a/1620375
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

    //create new bit maps and set up tex and viewport
    int pixNum{ d.gameTexSize.w * d.gameTexSize.h };
    back.createBitData(pixNum);
    offMain.createBitData(pixNum);
    window->adjustTexSizeAndViewport(d.gameTexSize, d.windowSize, winScale);

    // set clip for cursor
    if (confRef.control.clipCursor)
    {
      clipCursor();
    }

    /* set Crusader values */

    /* set getDeviceCaps values: */
    mainStruct.colorDepth = 0x10;  // simply to 16
    mainStruct.screenWidthInPixels = d.gameTexSize.w; // to current res -> defaults to 1280x720
    mainStruct.screenHeightInPixels = d.gameTexSize.h;
    mainStruct.runGameAsExclusiveFullscreen = 1;

    /*  set setWindowStyleAndRect values: */
    mainStruct.clientOnScreenCoords = { 0, 0, d.gameTexSize.w, d.gameTexSize.h };

    // these values might not be used, set them anyway for now
    int* resRect{ (int*)(winSetRectObjBaseAddr + 0x30) };
    *resRect = 0;
    *(resRect + 1) = d.gameTexSize.w;
    *(resRect + 2) = 0;
    *(resRect + 3) = d.gameTexSize.h;
    mainStruct.gameInWindowPosX = 0;
    mainStruct.gameInWindowPosY = 0;
    mainStruct.gameOnScreenPosX = 0;
    mainStruct.gameOnScreenPosY = 0;

    // interface
    mainStruct.ddInterfacePtr = this;
    mainStruct.NOT_selfBufferOrWindowMode = 1;
    mainStruct.unknown_6[1] = 1;  // function unknown, something with the cursor

    // surfaces:
    mainStruct.ddPrimarySurfacePtr = &prim;
    mainStruct.ddBackbufferSurfacePtr = &back;
    mainStruct.ddOffscreenSurfacePtr_Game = &offMain;
    mainStruct.ddOffscreenSurfacePtr_Map = &offMap;

    // offscreen desc, lock should fill them
    ZeroMemory(&mainStruct.ddSurfDescForBink_Game, sizeof(DDSURFACEDESC));
    offMain.Lock(NULL, &mainStruct.ddSurfDescForBink_Game, NULL, NULL);
    ZeroMemory(&mainStruct.ddSurfDescForBink_Map, sizeof(DDSURFACEDESC));
    offMap.Lock(NULL, &mainStruct.ddSurfDescForBink_Map, NULL, NULL);

    // set surface ptr
    mainStruct.surfacePtr_Game = offMain.getBitmapPtr();
    mainStruct.surfacePtr_Map = offMap.getBitmapPtr();

    // get and set bink flag
    static BinkDDSurfaceType binkSurfType{ nullptr };
    if (binkSurfType == nullptr)
    {
      HMODULE bink{ GetModuleHandleA("binkw32.dll") };  // no check, needs to be there
      if (bink)
      {
        binkSurfType = (BinkDDSurfaceType)GetProcAddress(bink, "_BinkDDSurfaceType@4");
      }
      if (!binkSurfType)
      {
        Log(LOG_FATAL, "[graphicsApiReplacer]: Failed to receive Bink Surface Type function. Aborting game.");
      }
    }
    // 0x8 in case of ARGB1555, 0xa for RGB565 (0x1 would be RGB 24bit, 0xc would be RGB664)
    binkStruct->gameSurfaceType = binkSurfType(&offMain);
    binkStruct->mapSurfaceType = binkSurfType(&offMap);

    // set some colors
    mainStruct.colorBitMode = confRef.graphic.pixFormat;
    colorFunc();

    Log(LOG_INFO, "[graphicsApiReplacer]: Drawing initialized.");
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