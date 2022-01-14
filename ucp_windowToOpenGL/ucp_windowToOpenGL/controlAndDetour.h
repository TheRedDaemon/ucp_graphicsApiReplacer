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

    LRESULT CALLBACK WindowProcHandlerFunc(int reservedCurrPrio, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // debug helper
    void ReplaceDWORD(DWORD destination, DWORD newDWORD);
  }

  namespace LuaFunc
  {
    // helper
    void getLoggingFunction(lua_State* L);

    bool isInRange(int num, int min, int max);
    bool setIntField(lua_State* L, int luaStackNum, int* ptrToSet, int min, int max);
    bool setBoolField(lua_State* L, int luaStackNum, bool* ptrToSet);

    // from external
    int setConfigField(lua_State* L);
  }

  namespace DetourFunc
  {
    void CreateWindowComplete(SHCWindowOrMainStructFake* that, HINSTANCE hInstance,
      LPSTR windowName, unsigned int cursorResource);   // naked
    void MainDrawInit(SHCWindowOrMainStructFake* that); // naked

    int WINAPI GetSystemMetricsCall(int nIndex);
    BOOL WINAPI GetCursorPosCall(LPPOINT lpPoint);
    HWND WINAPI GetForegroundWindowCall();
  }

  namespace FillAddress
  {
    extern DWORD WinSetRectObjBaseAddr;
    extern DWORD BinkControlObjAddr;
    extern DWORD SetSomeColorsAddr;
  }
}