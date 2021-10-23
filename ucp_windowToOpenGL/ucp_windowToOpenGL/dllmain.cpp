
#include "pch.h"

// lua
#include "lua.hpp"

#include "windowCore.h"
#include "fakeDDClasses.h"
#include "crusaderToOpenGL.h"

// for test
//#include <chrono>
//#include <thread>


namespace UCPtoOpenGL
{

  // static objects
  static CrusaderToOpenGL ToOpenGL;
  static ToOpenGLConfig conf;  // managed from here
  static WNDPROC WindowProcCallbackFunc{ 0x0 };  // 0x004B2C50, currently hardcoded, later received from lua, gynt gave me this address: 0x004B2AE0 (Crusader maybe?)

  // static debug helper

  static void ReplaceDWORD(DWORD destination, DWORD newDWORD)
  {
    DWORD* des{ reinterpret_cast<DWORD*>(destination) };

    DWORD oldAddressProtection;
    VirtualProtect(des, 4, PAGE_EXECUTE_READWRITE, &oldAddressProtection);
    *des = newDWORD;
    VirtualProtect(des, 4, oldAddressProtection, &oldAddressProtection);
  }


  // create own callbackProc -> everything has to pass through here for now
  static LRESULT CALLBACK WindowProcCallbackFake(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
  {
    // transform all mouse coords
    switch (uMsg)
    {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
      {
        if (!ToOpenGL.mouseDown() || !ToOpenGL.transformMouseMovePos(&lParam))
        {
          return 0; // devours message, since mouse in invalid area, or click should be discarded
        }
        break;
      }
      case WM_MOUSEMOVE:
      case WM_LBUTTONDBLCLK:
      case WM_LBUTTONUP:
      case WM_MBUTTONDBLCLK:
      case WM_MBUTTONUP:
      case WM_MOUSEHWHEEL:
      case WM_MOUSEHOVER:
      case WM_RBUTTONDBLCLK:
      case WM_RBUTTONUP:
      {
        if (!ToOpenGL.transformMouseMovePos(&lParam))
        {
          return 0; // devours message, since mouse in invalid area
        }
        break;
      }
      case WM_KILLFOCUS:
      {
        if (!ToOpenGL.windowLostFocus())
        {
          return 0;
        }
        break;
      }
      case WM_SETFOCUS:
      {
        if (!ToOpenGL.windowSetFocus())
        {
          return 0;
        }
        break;
      }
      case WM_ACTIVATEAPP:
      {
        if (!ToOpenGL.windowActivated(wParam))
        {
          return 0;
        }
        break;
      }
      /*
      case WM_DISPLAYCHANGE:
      case WM_SIZE: 
      {
        return 0; // prevent game from knowing, that the game size or the display + bit depth changed
      }
      */
      case WM_DESTROY:
        ToOpenGL.windowDestroyed();
        break;
      case WM_EXITSIZEMOVE:
        ToOpenGL.windowEditEnded(); // called if user stopped an interaction with a window (title bar, etc.)
        break;
      default:
        break;
    }

    return WindowProcCallbackFunc(hwnd, uMsg, wParam, lParam);
  }


  // lua functions to bind

  // using fastCall hack to get this in global function
  // source: https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
  // note: the second parameter is EDX and a dummy that should be ignored!
  static bool __fastcall CreateWindowComplete(SHCWindowOrMainStructFake* that, DWORD, LPSTR windowName, unsigned int unknown)
  {
    return ToOpenGL.createWindow(that, windowName, unknown, WindowProcCallbackFake);
  }

  // callback function missing
  static bool __declspec(naked) NakedCreateWindowComplete()
  {
    __asm {
      push    ecx   // push that pointer
      mov     ecx, ToOpenGL // mov toOpenGL this pointer
      jmp     CrusaderToOpenGL::createWindow  // jump to actual func
    }
  }



  static HRESULT WINAPI DirectDrawCreateCall(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
  {
    return ToOpenGL.createDirectDraw(lpGUID, lplpDD, pUnkOuter);
  }

  // stronghold gets the size of the screen sometimes
  // only handles 0 and 1, the first SetDisplayMode needs to happen before
  // TODO: Replacing the jump routes all code through here, maybe there are less hard methods?
  //  - depends on how other stuff gets handeld
  static int WINAPI GetSystemMetricsCall(int nIndex)
  {
    return ToOpenGL.getFakeSystemMetrics(nIndex);
  }

  // the main drawing rect is set via a user call -> keep the pointer, change it on demand
  // TODO: Replacing the jump routes all code through here, maybe there are less hard methods?
  //  - depends on how other stuff gets handeld

  static BOOL WINAPI SetRectCall(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
  {
    return ToOpenGL.setFakeRect(lprc, xLeft, yTop, xRight, yBottom);
  };


  // set window call -> simply deactivate
  static BOOL WINAPI SetWindowPosCall(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlag)
  {
    return ToOpenGL.setWindowPosFake(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlag);
  }

  // cursor -> to window (not to screen)
  static BOOL WINAPI GetCursorPosCall(LPPOINT lpPoint)
  {
    return ToOpenGL.getWindowCursorPos(lpPoint);
  }

  static HWND WINAPI GetForegroundWindowCall()
  {
    return ToOpenGL.GetForegroundWindowFake();
  }

  // further tests

  // what does it update?
  static BOOL WINAPI UpdateWindowCall(HWND hWnd)
  {
    return ToOpenGL.updateWindowFake(hWnd);
  }

  static BOOL WINAPI AdjustWindowRectCall(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
  {
    return ToOpenGL.adjustWindowRectFake(lpRect, dwStyle, bMenu);
  };


  static LONG WINAPI SetWindowLongACall(HWND hWnd, int nIndex, LONG dwNewLong)
  {
    return ToOpenGL.setWindowLongAFake(hWnd, nIndex, dwNewLong);
  }


  // first chars -> dll name, function name
  static void WINAPI DetouredWindowLongPtrReceive(char*, char*, DWORD* ptrToWindowLongPtr, DWORD, DWORD)
  {
    // e8 call -> but needs direct address
    *ptrToWindowLongPtr = reinterpret_cast<DWORD>(SetWindowLongACall);
  }


  // helper
  static bool isInRange(int num, int min, int max)
  {
    return num >= min && num <= max;
  }

  static bool setIntField(lua_State* L, int luaStackNum, int* ptrToSet, int min, int max)
  {
    if (!lua_isinteger(L, luaStackNum))
    {
      return false;
    }

    int value{ static_cast<int>(lua_tointeger(L, luaStackNum)) };
    if (!isInRange(value, min, max))
    {
      return false;
    }

    *ptrToSet = value;
    return true;
  }

  static bool setBoolField(lua_State* L, int luaStackNum, bool* ptrToSet)
  {
    if (!lua_isboolean(L, luaStackNum))
    {
      return false;
    }

    *ptrToSet = lua_toboolean(L, luaStackNum);
    return true;
  }

  // https://www.lua.org/manual/5.1/manual.html#lua_CFunction
  // static set field table
  static int setConfigField(lua_State* L)
  {
    int n = lua_gettop(L);    /* number of arguments */
    if (n != 3)
    {
      luaL_error(L, "ToOpenGL-Config: Invalid number of args.");
    }

    // func structure: setConfigField(char* option, char* field, ? value)
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2))
    {
      luaL_error(L, "ToOpenGL-Config: Field identifiers wrong.");
    }

    std::string option{ lua_tostring(L, 1) };
    std::string field{ lua_tostring(L, 2) };

    bool success{ false };
    bool fieldUnknown{ false };
    if (option == "window")
    {
      if (field == "type")
      {
        success = setIntField(L, 3, (int*)&conf.window.type, 0, 3);
      }
      else if (field == "width")
      {
        success = setIntField(L, 3, &conf.window.width, 0, 20000); // using ridiculous max 
      }
      else if (field == "height")
      {
        success = setIntField(L, 3, &conf.window.height, 0, 20000); // using ridiculous max
      }
      else if (field == "pos")
      {
        success = setIntField(L, 3, (int*)&conf.window.pos, 0, 4);
      }
      else if (field == "continueOutOfFocus")
      {
        success = setIntField(L, 3, (int*)&conf.window.continueOutOfFocus, 0, 2);
      }
      else
      {
        fieldUnknown = true;
      }
    }
    else if (option == "graphic")
    {
      if (field == "filterLinear")
      {
        success = setBoolField(L, 3, &conf.graphic.filterLinear);
      }
      else if (field == "vsync")
      {
        success = setBoolField(L, 3, &conf.graphic.vsync);
      }
      else if (field == "waitWithGLFinish")
      {
        success = setBoolField(L, 3, &conf.graphic.waitWithGLFinish);
      }
      else if (field == "pixFormat")
      {
        success = setIntField(L, 3, (int*)&conf.graphic.pixFormat, 0x555, 0x555) ? true : setIntField(L, 3, (int*)&conf.graphic.pixFormat, 0x565, 0x565);
      }
      else if (field == "debug")
      {
        success = setIntField(L, 3, (int*)&conf.graphic.debug, 0, 2);
      }
      else
      {
        fieldUnknown = true;
      }
    }
    else if (option == "control")
    {
      if (field == "clipCursor")
      {
        success = setBoolField(L, 3, &conf.control.clipCursor);
      }
      else if (field == "scrollActive")
      {
        success = setBoolField(L, 3, &conf.control.scrollActive);
      }
      else if (field == "margin")
      {
        success = setIntField(L, 3, &conf.control.margin, 0, 1000); // using ridiculous max
      }
      else if (field == "padding")
      {
        success = setIntField(L, 3, &conf.control.padding, 0, 1000); // using ridiculous max
      }
      else
      {
        fieldUnknown = true;
      }
    }
    else
    {
      luaL_error(L, ("ToOpenGL-Config: Invalid option: " + option).c_str());
    }

    if (fieldUnknown)
    {
      luaL_error(L, ("ToOpenGL-Config: Invalid field name for option '" + option + "': " + field).c_str());
    }

    if (!success)
    {
      luaL_error(L, ("ToOpenGL-Config: Invalid value for field '" + field + "' of option '" + option + "'.").c_str());
    }

    return 0;  /* number of results */
  }


  // lua module load
  extern "C" __declspec(dllexport) int __cdecl luaopen_ucp_windowToOpenGL(lua_State * L)
  {

    //std::this_thread::sleep_for(std::chrono::seconds(20)); // 20 seconds to attach

    lua_newtable(L); // push a new table on the stack

    // config function
    lua_pushcfunction(L, setConfigField);
    lua_setfield(L, -2, "setConfigField");

    // needs CALL to the value
    lua_pushinteger(L, (DWORD)CreateWindowComplete);
    lua_setfield(L, -2, "funcAddress_CreateWindow");

    //lua_pushinteger(L, (DWORD)NakedCreateWindowComplete);
    //lua_setfield(L, -2, "funcAddress_CreateWindow");

    // simple replace
    lua_pushinteger(L, (DWORD)DirectDrawCreateCall);
    lua_setfield(L, -2, "funcAddress_DirectDrawCreate");

    // simple replace
    lua_pushinteger(L, (DWORD)GetSystemMetricsCall);
    lua_setfield(L, -2, "funcAddress_GetSystemMetrics");

    // simple replace
    lua_pushinteger(L, (DWORD)SetRectCall);
    lua_setfield(L, -2, "funcAddress_SetRect");

    // simple replace
    lua_pushinteger(L, (DWORD)SetWindowPosCall);
    lua_setfield(L, -2, "funcAddress_SetWindowPos");

    // simple replace
    lua_pushinteger(L, (DWORD)GetCursorPosCall);
    lua_setfield(L, -2, "funcAddress_GetCursorPos");

    // simple replace
    lua_pushinteger(L, (DWORD)UpdateWindowCall);
    lua_setfield(L, -2, "funcAddress_UpdateWindow");

    // simple replace
    lua_pushinteger(L, (DWORD)AdjustWindowRectCall);
    lua_setfield(L, -2, "funcAddress_AdjustWindowRect");

    // simple replace
    lua_pushinteger(L, (DWORD)GetForegroundWindowCall);
    lua_setfield(L, -2, "funcAddress_GetForegroundWindow");

    // already a call
    lua_pushinteger(L, (DWORD)DetouredWindowLongPtrReceive);
    lua_setfield(L, -2, "funcAddress_DetouredWindowLongPtrReceive");

    // need to write window callback func to the returned address
    lua_pushinteger(L, (DWORD)&WindowProcCallbackFunc);
    lua_setfield(L, -2, "address_FillWithWindowProcCallback");
   



    /** The following structures where created at the start. Maybe some of the comments might prove useful one day. **/


    /* CreateWindowComplete */

    // extreme window creation function: 0x00470189
    // -> it is a thisCall
    // needed callback: 0x004B2C50 
    
    /*
    DWORD oldAddressProtection{ 0 };
    VirtualProtect(reinterpret_cast<DWORD*>(0x00470189), 5, PAGE_EXECUTE_READWRITE, &oldAddressProtection);

    unsigned char* call = reinterpret_cast<unsigned char*>(0x00470189);
    DWORD* func = reinterpret_cast<DWORD*>(0x00470189 + 1);

    *call = 0xE8;
    *func = reinterpret_cast<DWORD>(CreateWindowComplete) - 0x00470189 - 5;

    VirtualProtect(reinterpret_cast<DWORD*>(0x00470189), 5, oldAddressProtection, &oldAddressProtection);
    */


    /* DirectDrawCreateCall */

    // there is another pretty similar call, one condition further // extreme: 0x0046FCB8
    // extreme jump address: 0x0059E010

    // ReplaceDWORD(0x0059E010, (DWORD)DirectDrawCreateCall);


    /* GetSystemMetricsCall */

    // Crusader gets the size for some of its drawing RECTs via screen size, lets change that
    // extreme address for ONE Rect: 00468089
    // changing the jump address althougher for a test: 0059E1D0
    //  -> does not really help? result is that the click positions move to the display edge
    
    //ReplaceDWORD(0x0059E1D0, (DWORD)GetSystemMetricsCall);


    /* SetRectCall */

    // the main drawing RECT is set using USER.SetRect, lets do it, but remember the pointer, extreme call: 004B2D06
    // this is done before DirectDraw gets the set display mode order -> sets it to screen size?
    // some weird stuff happens during the window creation... why is this a different number in other cases...
    // maybe the combination of window and exlusiv mode already changes the resolution, the SetDisplayMode is just for DirectDraw?
    // other address, for general jump: 0059E200
    
    //ReplaceDWORD(0x0059E200, (DWORD)SetRectCall);


    /* SetWindowPosCall */

    // try to get more control over window:
    
    //ReplaceDWORD(0x0059E1F8, (DWORD)SetWindowPosCall);


    /* GetCursorPosCall */

    // map cursor
   
    //ReplaceDWORD(0x0059E1E8, (DWORD)GetCursorPosCall);


    /* UpdateWindowCall */

    // window update call
    
    //ReplaceDWORD(0x0059E1F4, (DWORD)UpdateWindowCall);


    /* AdjustWindowRectCall */

    // adjust client rect

    //ReplaceDWORD(0x0059E1FC, (DWORD)AdjustWindowRectCall);


    /* DetouredWindowLongPtrReceive */

    // set windowLong load func
    
    //ReplaceDWORD(0x0057CCCA + 1, reinterpret_cast<DWORD>(DetouredWindowLongPtrReceive) - 0x0057CCCA - 5);


    /* DetouredWindowLongPtrReceive */

    // game tests if it is in the foreground

    // ReplaceDWORD(0x0059E20C, (DWORD)GetForegroundWindowCall);

    ToOpenGL.setConf(&conf);

    return 1;
  }

  // entry point
  BOOL APIENTRY DllMain(HMODULE,
    DWORD  ul_reason_for_call,
    LPVOID
  )
  {
    switch (ul_reason_for_call)
    {
      case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
  }
}

/* ISSUES:
    - 1366x768 problem:
      - at 0x0046905E: 0xC0000005: Access denied while writting to position 0x1709D000
      - Try to find reason

      - 00468ea0 in normal Crusader:
        - function seems to move something on one of the surfaces
        - idea:
          - might be used to transform (or maybe stretch?) something on the surface
          - the surface size is fixed, so the issue should be on something else...
            - maybe the breaking res can actually not be renderend, so Crusader needs to tranform something
            and since I aggressivly set the drawing rects, it breaks...?
*/

/* NOTE:
    - There might be a chance that bink video will be of interest in the future.
    - for example:
        - RADEXPFUNC s32 RADEXPLINK BinkDDSurfaceType(void PTR4* lpDDS) (0x0046FF61) -> gets flag(part)
        - RADEXPFUNC s32 RADEXPLINK BinkCopyToBuffer(HBINK bnk,void* dest,s32 destpitch,u32 destheight,u32 destx,u32 desty,u32 flags); (0x004091D6) -> blt to surface
    - structure ref (maybe): https://github.com/ioquake/jedi-outcast/blob/master/code/win32/bink.h

    - older bink: https://github.com/miohtama/aliens-vs-predator/blob/master/source/AvP_vc/3dc/win95/bink.h

    - for mss: https://github.com/VSES/SourceEngine2007/blob/master/src_main/common/Miles/MSS.H

    - Crusader (likely as part of development?) seems to have (leftovers?) of console output.
      - The biggest hint should be the printf(?) structures gynt found.
      - But they also import functions like: (could however just be lowlevel functions of the engine)
        - 309  GetConsoleOutputCP
        - 921  WriteConsoleA
        - 307  GetConsoleMode
        - 290  GetConsoleCP
      - Could also be that they were included for tests with code parts, and no real "debug" system in-game ever existed.
        - 272  GetCommandLineA -> maybe they have start parameter?
        - 569  IsDebuggerPresent -> they might have a debuging system?
      - gynt found a debugging menu. This might be related.

    - There are multiple functions that likely effect the window (excluding already detoured) (jump addresses):
      -   13  BeginPaint          -> 0x0059E1A8
      -  200  EndPaint            -> 0x0059E1AC
      -  554  ReleaseDC           -> 0x0059E1E0
      -  268  GetDC               -> 0x0059E1E4
      -  598  SetFocus            -> 0x0059E1F0
      -  700  UpdateWindow        -> 0x0059E1F4
      -  643  SetWindowPos        -> 0x0059E1F8
      -    1  AdjustWindowRect    -> 0x0059E1FC -> This might very well also be a possible point to change stuff, instead of other positions
      -   64  ClientToScreen      -> 0x         -> registered no call in normal game
      -  255  GetClientRect       -> 0x         -> registered no call in normal game
      -  279  GetForegroundWindow -> 0x0059E20C

      - (Crusader) 0x00b95778 -> ptr to SetWindowLongA -> is memory position
        - dynamic resolved with with a function here: (Crusader) 0x0057c89e (is a E8 call):
          - func: undefined _ResolveThunk@20("user32.dll","SetWindowLongA", &PTR_SetWindowLongAThunk_00b95778, DAT_024280b4, FID_conflict:_GodotFailFindResourceW@12)
            - third is important ptr to SetWindowLongA ptr
            - ghira assumes void return
          - extreme address: 0x0057CCCA

    - Others likely effect the mouse control:
      -  267  GetCursorPos        -> 0x0059E1E8
      -> they get their mouse cursor differently... -> maybe messages -> message hook?
      - general mouse input function seems to be here: 0x00468320
      - but here the whole thing reacts to clicks: 0x00468250
      - ok, it is the general message proc:
        -> I will detour the window proc message for now:
          -> this might get handy for later, but I assume a more general interface for this would be needed
          -> other might want to do stuff with it

    - it might also be possible to remove the create window hook and use the received window
      - changing class values is possible after all: SetClassLongPtrA

    - Borderless fullscreen vs fullscreen:
      - I am now confused if the difference is basically: fullscreen set screen resolution to own res, borderless fullscreen not
*/

/*
  Borderless Fullscreen:
    - Windows Game Bar
      - "Borderless" fullscreen:
        - not working, recording broken
        - screenshot shortcut works
          - altough it starts to make the screen brighter and brighter
        - first recording fails after one second
          - then the gamebar can be openend once + one normal recording is possible
        -> default settings (outside of window type)
          - currently no idea, maybe sokmething related to the focus switch?
      -> apperantly this is related to OpenGL:
        - https://docs.microsoft.com/en-us/gaming/game-bar/known-issues
        - the driver renders the game likely without DWM, I assume this is what they need to use...
          - maybe it works after the failed recording attempt, becasue it becomes connected to the DWM for a moment 
        - this could be the reason why VSync is needed in fullscreen...

    - Steam:
      - Overlay works

  Borderless Window:
    - Steam
      - no problems

    - Gamebar
      - no problems

  Window:
    - Steam
      - no problems

    - Gamebar
      - Screenshots to not take titlebar into account
*/