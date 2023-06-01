
#ifndef WIN_PROC_HEADER
#define WIN_PROC_HEADER

// also needs windows.h, although the way of including can be handled by the user
#include "lua.hpp"

#include "ucp3.h"

namespace WinProcHeader
{
  using FuncGetMainProc = WNDPROC(__stdcall*)();
  using FuncCallNextProc = LRESULT(__stdcall*)(int reservedCurrentPrio, HWND, UINT, WPARAM, LPARAM);
  using FuncRegisterProc = int(__stdcall*)(FuncCallNextProc funcToCall, int priority);

  inline constexpr char const* NAME_VERSION{ "0.2.0" };
  inline constexpr char const* NAME_MODULE{ "winProcHandler" };
  inline constexpr char const* NAME_LIBRARY{ "winProcHandler.dll" };

  inline constexpr char const* NAME_GET_MAIN_PROC{ "_GetMainProc@0" };
  inline constexpr char const* NAME_CALL_NEXT_PROC{ "_CallNextProc@20" };
  inline constexpr char const* NAME_REGISTER_PROC{ "_RegisterProc@8" };

  // used at start for first call, can not carry function, returned by RegisterProc indicates placement failed
  inline constexpr int NO_VALID_PRIO{ INT_MIN };

  inline FuncGetMainProc GetMainProc{ nullptr };
  inline FuncCallNextProc CallNextProc{ nullptr };
  inline FuncRegisterProc RegisterProc{ nullptr };

  inline bool initModuleFunctions()
  {
    GetMainProc = (FuncGetMainProc) ucp_getProcAddressFromLibraryInModule(NAME_MODULE, NAME_LIBRARY, NAME_GET_MAIN_PROC);
    CallNextProc = (FuncCallNextProc) ucp_getProcAddressFromLibraryInModule(NAME_MODULE, NAME_LIBRARY, NAME_CALL_NEXT_PROC);
    RegisterProc = (FuncRegisterProc) ucp_getProcAddressFromLibraryInModule(NAME_MODULE, NAME_LIBRARY, NAME_REGISTER_PROC);

    return GetMainProc && CallNextProc && RegisterProc;
  }
}

#endif //WIN_PROC_HEADER