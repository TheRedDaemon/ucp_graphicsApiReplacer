
#include "pch.h"

// lua
#include "lua.hpp"

#include "windowCore.h"
#include "fakeSurfaces.h"
#include "crusaderToOpenGL.h"


#include <chrono>
#include <thread>


// static objects
static UCPtoOpenGL::CrusaderToOpenGL ToOpenGL;
static WNDPROC KeyboardCallbackFunc{ (WNDPROC)0x004B2C50 };  // currently hardcoded, later received from lua, gynt gave me this address: 0x004B2AE0 (Crusader maybe?)

// static deug helper

static void ReplaceDWORD(DWORD destination, DWORD newDWORD)
{
  DWORD* des{ reinterpret_cast<DWORD*>(destination) };
  
  DWORD oldAddressProtection;
  VirtualProtect(des, 4, PAGE_EXECUTE_READWRITE, &oldAddressProtection);
  *des = newDWORD;
  VirtualProtect(des, 4, oldAddressProtection, &oldAddressProtection);
}


// lua functions to bind

// using fastCall hack to get this in global function
// source: https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
// note: the second parameter is EDX and a dummy that should be ignored!
bool __fastcall CreateWindowComplete(void* that, DWORD, LPSTR windowName, unsigned int unknown)
{
  return ToOpenGL.createWindow((DWORD)that, windowName, unknown, KeyboardCallbackFunc);
}

HRESULT WINAPI DirectDrawCreateCall(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
{
  return ToOpenGL.createDirectDraw(lpGUID, lplpDD, pUnkOuter);
}

// stronghold gets the size of the screen sometimes times
// only handles 0 and 1, the first SetDisplayMode needs to happen before
// TODO: Replacing the jump routes all code through here, maybe there are less hard methods?
//  - depends on how other stuff gets handeld
int WINAPI GetSystemMetricsCall(int nIndex)
{
  return ToOpenGL.getFakeSystemMetrics(nIndex);
}

// the main drawing rect is set via a user call -> keep the pointer, change it on demand
// TODO: Replacing the jump routes all code through here, maybe there are less hard methods?
//  - depends on how other stuff gets handeld
BOOL WINAPI SetRectCall(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
{
  return ToOpenGL.setFakeRect(lprc, xLeft, yTop, xRight, yBottom);
};

// set window call -> simply deactivate
BOOL WINAPI SetWindowPosCall(HWND, HWND, int, int, int, int, UINT)
{
  return true;
}

// cursor -> to window (not to screen)
BOOL WINAPI GetCursorPosCall(LPPOINT lpPoint)
{
  return ToOpenGL.getWindowCursorPos(lpPoint);
}


// lua module load
extern "C" __declspec(dllexport) int __cdecl luaopen_ucp_windowToOpenGL(lua_State * L)
{
  //lua_newtable(L); // push a new table on the stack
  //lua_pushinteger(L, &dummyFunction); // The value we want to set
  //lua_setfield(L, -2, "dummyFunction"); // Sets the value on the top of the stack to this key in the table at index -2 (the table is not on the top, but right under it). The value is popped off the stack.

  // The table is left on top of the stack, so it is now easy to tell lua we will return one value (the table).

  // for test, I will hardcode the detours
  DWORD oldAddressProtection{ 0 };

  // extreme window creation function: 0x00470189
  // -> it is a thisCall
  // needed callback: 004b2ae0 
  VirtualProtect(reinterpret_cast<DWORD*>(0x00470189), 5, PAGE_EXECUTE_READWRITE, &oldAddressProtection);

  unsigned char* call = reinterpret_cast<unsigned char*>(0x00470189);
  DWORD* func = reinterpret_cast<DWORD*>(0x00470189 + 1);

  *call = 0xE8;
  *func = reinterpret_cast<DWORD>(CreateWindowComplete) - 0x00470189 - 5;

  VirtualProtect(reinterpret_cast<DWORD*>(0x00470189), 5, oldAddressProtection, &oldAddressProtection);


  // there is another pretty similar call, one condition further // extreme: 0x0046FCB8
  // extreme jump address: 0x0059E010
  ReplaceDWORD(0x0059E010, (DWORD)DirectDrawCreateCall);

  // Crusader gets the size for some of its drawing RECTs via screen size, lets change that
  // extreme address for ONE Rect: 00468089
  // changing the jump address althougher for a test: 0059E1D0
  //  -> does not really help? result is that the click positions move to the display edge
  ReplaceDWORD(0x0059E1D0, (DWORD)GetSystemMetricsCall);

  // the main drawing RECT is set using USER.SetRect, lets do it, but remember the pointer, extreme call: 004B2D06
  // this is done before DirectDraw gets the set display mode order -> sets it to screen size?
  // some weird stuff happens during the window creation... why is this a different number in other cases...
  // maybe the combination of window and exlusiv mode already changes the resolution, the SetDisplayMode is just for DirectDraw?
  // other address, for general jump: 0059E200
  ReplaceDWORD(0x0059E200, (DWORD)SetRectCall);


  // try to get more control over window:
  ReplaceDWORD(0x0059E1F8, (DWORD)SetWindowPosCall);


  // map cursor
  ReplaceDWORD(0x0059E1E8, (DWORD)GetCursorPosCall);

  //std::this_thread::sleep_for(std::chrono::seconds(10)); // 20 seconds to attach

  return 1;
}

// entry point
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
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

/* NOTE:
    - There might be a chance that bink video will be of interest in the future.
    - for example:
        - RADEXPFUNC s32 RADEXPLINK BinkDDSurfaceType(void PTR4* lpDDS) (0x0046FF61) -> gets flag(part)
        - RADEXPFUNC s32 RADEXPLINK BinkCopyToBuffer(HBINK bnk,void* dest,s32 destpitch,u32 destheight,u32 destx,u32 desty,u32 flags); (0x004091D6) -> blt to surface
    - structure ref (maybe): https://github.com/ioquake/jedi-outcast/blob/master/code/win32/bink.h

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

    - There are multiple functions that likely effect the window (excluding already detoured) (jump addresses):
      -   13  BeginPaint          -> 0x0059E1A8
      -  200  EndPaint            -> 0x0059E1AC
      -  554  ReleaseDC           -> 0x0059E1E0
      -  268  GetDC               -> 0x0059E1E4
      -  598  SetFocus            -> 0x0059E1F0
      -  700  UpdateWindow        -> 0x0059E1F4
      -  643  SetWindowPos        -> 0x0059E1F8
      -    1  AdjustWindowRect    -> 0x0059E1FC
      -   64  ClientToScreen      -> 0x         -> registered no call in normal game
      -  255  GetClientRect       -> 0x         -> registered no call in normal game
      -  279  GetForegroundWindow -> 0x0059E20C

    - Others likely effect the mouse control:
      -  267  GetCursorPos        -> 0x0059E1E8
      -> they get their mouse cursor differently...
*/