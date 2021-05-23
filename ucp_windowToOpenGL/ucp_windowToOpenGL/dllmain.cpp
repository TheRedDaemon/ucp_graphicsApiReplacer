
#include "pch.h"

// lua
#include "lua.hpp"

#include "windowCore.h"
#include "fakeSurfaces.h"
#include "crusaderToOpenGL.h"


// static object
static UCPtoOpenGL::CrusaderToOpenGL ToOpenGL;

// lua functions to bind

HWND WINAPI CreateWindowCall(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y,
  int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
  return ToOpenGL.createWindow(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight,
    hWndParent, hMenu, hInstance, lpParam);
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


// lua module load
extern "C" __declspec(dllexport) int __cdecl luaopen_ucp_windowToOpenGL(lua_State * L)
{
  //lua_newtable(L); // push a new table on the stack
  //lua_pushinteger(L, &dummyFunction); // The value we want to set
  //lua_setfield(L, -2, "dummyFunction"); // Sets the value on the top of the stack to this key in the table at index -2 (the table is not on the top, but right under it). The value is popped off the stack.

  // The table is left on top of the stack, so it is now easy to tell lua we will return one value (the table).

  // for test, I will hardcode the detours

  // address for create window: 00467B22 -> can be replaced with call and a nop // extreme: 0x00467D52
  DWORD oldAddressProtection{ 0 };
  VirtualProtect(reinterpret_cast<DWORD*>(0x00467D52), 6, PAGE_EXECUTE_READWRITE, &oldAddressProtection);

  unsigned char* call = reinterpret_cast<unsigned char*>(0x00467D52);
  DWORD* func = reinterpret_cast<DWORD*>(0x00467D52 + 1);
  unsigned char* nop = reinterpret_cast<unsigned char*>(0x00467D52 + 5);

  *call = 0xE8;
  *func = reinterpret_cast<DWORD>(CreateWindowCall) - 0x00467D52 - 5;
  *nop = 0x90;
  
  VirtualProtect(reinterpret_cast<DWORD*>(0x00467B22), 6, oldAddressProtection, &oldAddressProtection);


  // there is another pretty similar call, one condition further // extreme: 0x0046FCB8
  VirtualProtect(reinterpret_cast<DWORD*>(0x0046FCB8), 5, PAGE_EXECUTE_READWRITE, &oldAddressProtection);

  call = reinterpret_cast<unsigned char*>(0x0046FCB8);
  func = reinterpret_cast<DWORD*>(0x0046FCB8 + 1);

  *call = 0xE8;
  *func = reinterpret_cast<DWORD>(DirectDrawCreateCall) - 0x0046FCB8 - 5;

  VirtualProtect(reinterpret_cast<DWORD*>(0x0046FCB8), 5, oldAddressProtection, &oldAddressProtection);


  // Crusader gets the size for some of its drawing RECTs via screen size, lets change that
  // extreme address for ONE Rect: 00468089
  // changing the jump address althougher for a test: 0059E1D0
  //  -> does not really help? result is that the click positions move to the display edge
  VirtualProtect(reinterpret_cast<DWORD*>(0x0059E1D0), 4, PAGE_EXECUTE_READWRITE, &oldAddressProtection);

  func = reinterpret_cast<DWORD*>(0x0059E1D0);

  // change the address that is moved in the register
  *func = reinterpret_cast<DWORD>(GetSystemMetricsCall);

  VirtualProtect(reinterpret_cast<DWORD*>(0x0059E1D0), 4, oldAddressProtection, &oldAddressProtection);


  // the main drawing RECT is set using USER.SetRect, lets do it, but remember the pointer, extreme call: 004B2D06
  // this is done before DirectDraw gets the set display mode order -> sets it to screen size?
  // some weird stuff happens during the window creation... why is this a different number in other cases...
  // maybe the combination of window and exlusiv mode already changes the resolution, the SetDisplayMode is just for DirectDraw?
  // other address, for general jump: 0059E200
  VirtualProtect(reinterpret_cast<DWORD*>(0x0059E200), 6, PAGE_EXECUTE_READWRITE, &oldAddressProtection);

  //call = reinterpret_cast<unsigned char*>(0x004B2D06);
  func = reinterpret_cast<DWORD*>(0x0059E200 /*+ 1*/);
  //nop = reinterpret_cast<unsigned char*>(0x004B2D06 + 5);

  //*call = 0xE8;
  *func = reinterpret_cast<DWORD>(SetRectCall) /*- 0x004B2D06 - 5*/;
  //*nop = 0x90;

  VirtualProtect(reinterpret_cast<DWORD*>(0x004B2D06), 6, oldAddressProtection, &oldAddressProtection);

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

