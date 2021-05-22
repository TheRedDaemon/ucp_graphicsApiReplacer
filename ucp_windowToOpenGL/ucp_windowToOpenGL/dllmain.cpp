
#include "pch.h"

// lua
#include "lua.hpp"

#include "windowCore.h"
#include "fakeSurfaces.h"
#include "crusaderToOpenGL.h"


// static object
static UCPtoOpenGL::CrusaderToOpenGL ToOpenGL;

// lua functions

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


  // there is another pretty similar call, one condition further // extrme: 0x0046FCB8
  VirtualProtect(reinterpret_cast<DWORD*>(0x0046FCB8), 5, PAGE_EXECUTE_READWRITE, &oldAddressProtection);

  call = reinterpret_cast<unsigned char*>(0x0046FCB8);
  func = reinterpret_cast<DWORD*>(0x0046FCB8 + 1);

  *call = 0xE8;
  *func = reinterpret_cast<DWORD>(DirectDrawCreateCall) - 0x0046FCB8 - 5;

  VirtualProtect(reinterpret_cast<DWORD*>(0x0046FCB8), 5, oldAddressProtection, &oldAddressProtection);

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

