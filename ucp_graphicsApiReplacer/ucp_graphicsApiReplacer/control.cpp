
#include "pch.h"

// lua
#include "lua.hpp"

#include "controlAndDetour.h"
#include "crusaderGraphicsApiReplacer.h"

// winProcHandler
#include <winProcHandler.h>

namespace UCPGraphicsApiReplacer
{

  namespace FillAddress
  {
    DWORD WinSetRectObjBaseAddr{ 0x0 };
    DWORD BinkControlObjAddr{ 0x0 };
    DWORD SetSomeColorsAddr{ 0x0 };
  }


  namespace Control
  {
    GraphicsAPIReplacerConfig Conf;
    CrusaderGraphicsApiReplacer ToOpenGL{ Conf };


    LRESULT CALLBACK WindowProcHandlerFunc(int reservedCurrPrio, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
          ToOpenGL.mouseClipOnHold(uMsg);
          break;
        }
        case WM_RBUTTONUP:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        {
          if (!ToOpenGL.transformMouseMovePos(&lParam))
          {
            return 0; // devours message, since mouse in invalid area
          }
          ToOpenGL.mouseClipOnHold(uMsg);
          break;
        }
        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_MOUSEHWHEEL:
        case WM_MOUSEHOVER:
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
          if (!ToOpenGL.windowActivated((bool*)&wParam))
          {
            return 0;
          }
          break;
        }
        case WM_DISPLAYCHANGE:
        case WM_SIZE:
          return 0;   // prevent game from knowing, that the size or display + bit depth changed
        /*
        case WM_PAINT:  // paint does kinda strange stuff, like requesting a begin and end paint after one another
          break;        // but since it sets values I do not have access to, I leave it at the moment
                        // I am wondering if the reason for the not as smooth ALT-TAB to the menu is here
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

      return WinProcHeader::CallNextProc(reservedCurrPrio, hwnd, uMsg, wParam, lParam);
    }


    // debug helper
    void ReplaceDWORD(DWORD destination, DWORD newDWORD)
    {
      DWORD* des{ reinterpret_cast<DWORD*>(destination) };

      DWORD oldAddressProtection;
      VirtualProtect(des, 4, PAGE_EXECUTE_READWRITE, &oldAddressProtection);
      *des = newDWORD;
      VirtualProtect(des, 4, oldAddressProtection, &oldAddressProtection);
    }
  }



  // also using for lua config
  namespace LuaFunc
  {
    static lua_State* luaState{ nullptr };
    static int luaLogFuncIndex{ 0 };


    // helper
    void getLoggingFunction(lua_State* L)
    {
      luaState = L; // keep state
      lua_getglobal(L, "log");  // get log
      if (!lua_isfunction(L, -1))
      {
        lua_pop(L, 1);
        return;
      }
      // stores index to function, also pops value, currently not removed for lifetime of program
      luaLogFuncIndex = luaL_ref(L, LUA_REGISTRYINDEX);
    }


    bool isInRange(int num, int min, int max)
    {
      return num >= min && num <= max;
    }


    bool setIntField(lua_State* L, int luaStackNum, int* ptrToSet, int min, int max)
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


    bool setBoolField(lua_State* L, int luaStackNum, bool* ptrToSet)
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
    int setConfigField(lua_State* L)
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
          success = setIntField(L, 3, (int*)&Control::Conf.window.type, 0, 3);
        }
        else if (field == "width")
        {
          success = setIntField(L, 3, &Control::Conf.window.width, 0, 20000); // using ridiculous max 
        }
        else if (field == "height")
        {
          success = setIntField(L, 3, &Control::Conf.window.height, 0, 20000); // using ridiculous max
        }
        else if (field == "pos")
        {
          success = setIntField(L, 3, (int*)&Control::Conf.window.pos, 0, 4);
        }
        else if (field == "continueOutOfFocus")
        {
          success = setIntField(L, 3, (int*)&Control::Conf.window.continueOutOfFocus, 0, 2);
        }
        else
        {
          fieldUnknown = true;
        }
      }
      else if (option == "graphic")
      {
        if (field == "api")
        {
          success = setIntField(L, 3, (int*)&Control::Conf.graphic.graphicsApi, 0, 1);
        }
        else if (field == "filterLinear")
        {
          success = setBoolField(L, 3, &Control::Conf.graphic.filterLinear);
        }
        else if (field == "vsync")
        {
          success = setBoolField(L, 3, &Control::Conf.graphic.vsync);
        }
        else if (field == "waitWithGLFinish")
        {
          success = setBoolField(L, 3, &Control::Conf.graphic.waitWithGLFinish);
        }
        else if (field == "pixFormat")
        {
          success = setIntField(L, 3, (int*)&Control::Conf.graphic.pixFormat, 0x555, 0x555) ? true : 
            setIntField(L, 3, (int*)&Control::Conf.graphic.pixFormat, 0x565, 0x565);
        }
        else if (field == "debug")
        {
          success = setIntField(L, 3, (int*)&Control::Conf.graphic.debug, 0, 2);
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
          success = setBoolField(L, 3, &Control::Conf.control.clipCursor);
        }
        else if (field == "scrollActive")
        {
          success = setBoolField(L, 3, &Control::Conf.control.scrollActive);
        }
        else if (field == "margin")
        {
          success = setIntField(L, 3, &Control::Conf.control.margin, 0, 1000); // using ridiculous max
        }
        else if (field == "padding")
        {
          success = setIntField(L, 3, &Control::Conf.control.padding, 0, 1000); // using ridiculous max
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
  }


  // define log function in right scope (did not figure out other way)
  void Log(LogLevel level, const char* message)
  {
    if (LuaFunc::luaLogFuncIndex) // only if there, otherwise silent
    {
      lua_State* L{ LuaFunc::luaState };
      lua_rawgeti(L, LUA_REGISTRYINDEX, LuaFunc::luaLogFuncIndex);
      lua_pushinteger(L, level);
      lua_pushstring(L, message);
      lua_call(L, 2, 0);
    }
  }
}