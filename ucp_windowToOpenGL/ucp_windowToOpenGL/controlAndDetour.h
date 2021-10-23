#pragma once


struct lua_State; // forward dec

namespace UCPtoOpenGL
{
  // forward dec
  class CrusaderToOpenGL;
  struct ToOpenGLConfig;
  struct SHCWindowOrMainStructFake;

  namespace Control
  {
    // not sure if smart idea...
    extern ToOpenGLConfig Conf;
    extern CrusaderToOpenGL ToOpenGL;

    LRESULT CALLBACK WindowProcCallbackFake(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    // debug helper
    void ReplaceDWORD(DWORD destination, DWORD newDWORD);
  }

  namespace LuaFunc
  {
    // helper
    bool isInRange(int num, int min, int max);
    bool setIntField(lua_State* L, int luaStackNum, int* ptrToSet, int min, int max);
    bool setBoolField(lua_State* L, int luaStackNum, bool* ptrToSet);

    // to external
    int setConfigField(lua_State* L);
  }

  namespace DetourFunc
  {
    bool __fastcall CreateWindowComplete(SHCWindowOrMainStructFake* that, DWORD, LPSTR windowName, unsigned int unknown);
    bool NakedCreateWindowComplete();

    HRESULT WINAPI DirectDrawCreateCall(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter);
    int WINAPI GetSystemMetricsCall(int nIndex);
    BOOL WINAPI SetRectCall(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
    BOOL WINAPI SetWindowPosCall(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlag);
    BOOL WINAPI GetCursorPosCall(LPPOINT lpPoint);
    HWND WINAPI GetForegroundWindowCall();
    BOOL WINAPI UpdateWindowCall(HWND hWnd);
    BOOL WINAPI AdjustWindowRectCall(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
    LONG WINAPI SetWindowLongACall(HWND hWnd, int nIndex, LONG dwNewLong);
    void WINAPI DetouredWindowLongPtrReceive(char*, char*, DWORD* ptrToWindowLongPtr, DWORD, DWORD);
  }

  namespace FillAddress
  {
    extern WNDPROC WindowProcCallbackFunc; // will be filled by memory write
  }
}