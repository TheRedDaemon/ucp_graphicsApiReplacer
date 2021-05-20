
#include "pch.h"

// lua
#include "lua.hpp"

#include "windowCore.h"
#include "crusaderToOpenGL.h"

// static object
static CrusaderToOpenGL ToOpenGL;

// lua functions

HWND WINAPI CreateWindowCall(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y,
  int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
  return ToOpenGL.createWindow(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight,
    hWndParent, hMenu, hInstance, lpParam);
}

HRESULT WINAPI DirectDrawCreate(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
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

