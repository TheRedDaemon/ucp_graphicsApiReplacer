
#ifndef WIN_PROC_HEADER
#define WIN_PROC_HEADER

// also needs windows.h, although the way of including can be handled by the user
#include "lua.hpp"

namespace WinProcHeader
{
  using FuncGetMainProc = WNDPROC(__stdcall*)();
  using FuncCallNextProc = LRESULT(__stdcall*)(int reservedCurrentPrio, HWND, UINT, WPARAM, LPARAM);
  using FuncRegisterProc = int(__stdcall*)(FuncCallNextProc funcToCall, int priority);

  inline constexpr char const* NAME_VERSION{ "0.1.0" };

  inline constexpr char const* NAME_MODULE{ "winProcHandler" };
  inline constexpr char const* NAME_GET_MAIN_PROC{ "_GetMainProc@0" };
  inline constexpr char const* NAME_CALL_NEXT_PROC{ "_CallNextProc@20" };
  inline constexpr char const* NAME_REGISTER_PROC{ "_RegisterProc@8" };

  // used at start for first call, can not carry function, returned by RegisterProc indicates placement failed
  inline constexpr int NO_VALID_PRIO{ INT_MIN };

  inline FuncGetMainProc GetMainProc{ nullptr };
  inline FuncCallNextProc CallNextProc{ nullptr };
  inline FuncRegisterProc RegisterProc{ nullptr };

  // returns true if the function variables of this header were successfully filled
  inline bool initModuleFunctions(lua_State* L)
  {
    if (lua_getglobal(L, "modules") != LUA_TTABLE)
    {
      lua_pop(L, 1);  // remove value
      return false;
    }

    if (lua_getfield(L, -1, NAME_MODULE) != LUA_TTABLE)
    {
      lua_pop(L, 2);  // remove table and value
      return false;
    }

    GetMainProc = (lua_getfield(L, -1, NAME_GET_MAIN_PROC) == LUA_TNUMBER) ? (FuncGetMainProc)lua_tointeger(L, -1) : nullptr;
    lua_pop(L, 1);  // remove value
    CallNextProc = (lua_getfield(L, -1, NAME_CALL_NEXT_PROC) == LUA_TNUMBER) ? (FuncCallNextProc)lua_tointeger(L, -1) : nullptr;
    lua_pop(L, 1);  // remove value
    RegisterProc = (lua_getfield(L, -1, NAME_REGISTER_PROC) == LUA_TNUMBER) ? (FuncRegisterProc)lua_tointeger(L, -1) : nullptr;
    lua_pop(L, 3);  // remove value and all tables

    return GetMainProc && CallNextProc && RegisterProc;
  }

  //Currently unused func using default windows load
  /* inline bool initModuleFunctions()
  {
    HMODULE winProcModule{ GetModuleHandleA(NAME_MODULE) };
    if (!winProcModule)
    {
      return false;
    }

    GetMainProc = (FuncGetMainProc)GetProcAddress(winProcModule, NAME_GET_MAIN_PROC);
    CallNextProc = (FuncCallNextProc)GetProcAddress(winProcModule, NAME_CALL_NEXT_PROC);
    RegisterProc = (FuncRegisterProc)GetProcAddress(winProcModule, NAME_REGISTER_PROC);

    return GetMainProc && CallNextProc && RegisterProc;
  }
  */
}

#endif //WIN_PROC_HEADER