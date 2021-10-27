
#include "pch.h"

#include "controlAndDetour.h"
#include "crusaderToOpenGL.h"

namespace UCPtoOpenGL
{
  namespace DetourFunc
  {
    void __declspec(naked) CreateWindowComplete(SHCWindowOrMainStructFake* that, HINSTANCE hInstance,
      LPSTR windowName, unsigned int cursorResource)
    {
      __asm
      {
        // remove return address (eax should be free to modify)
        mov     eax, dword ptr [esp]
        add     esp, 4
        
        // push
        push    ecx   // shc struct
        push    dword ptr[Control::WindowProcCallbackFake]
        push    eax   // set ret address again

        mov     ecx, offset Control::ToOpenGL // mov toOpenGL this pointer
        
        jmp     CrusaderToOpenGL::createWindow  // jump to actual func
      }
    }

    void __declspec(naked) MainDrawInit(SHCWindowOrMainStructFake* that)
    {
      __asm 
      {
        // remove return address (eax should be free to modify)
        mov     eax, dword ptr[esp]
        add     esp, 4

        // push
        push    ecx   // shc struct
        push    dword ptr[FillAddress::WinSetRectObjBaseAddr]
        push    dword ptr[FillAddress::BinkControlObjAddr]
        push    dword ptr[FillAddress::SetSomeColorsAddr]
        push    eax   // set ret address again

        mov     ecx, offset Control::ToOpenGL // mov toOpenGL this pointer

        jmp     CrusaderToOpenGL::drawInit  // jump to actual func
      }
    }

    // stronghold gets the size of the screen sometimes
    // only handles 0 and 1, the first SetDisplayMode needs to happen before
    // TODO: Replacing the jump routes all code through here, maybe there are less hard methods?
    //  - depends on how other stuff gets handeld
    int WINAPI GetSystemMetricsCall(int nIndex)
    {
      return Control::ToOpenGL.getFakeSystemMetrics(nIndex);
    }

    // cursor -> to window (not to screen)
    BOOL WINAPI GetCursorPosCall(LPPOINT lpPoint)
    {
      return Control::ToOpenGL.getWindowCursorPos(lpPoint);
    }

    HWND WINAPI GetForegroundWindowCall()
    {
      return Control::ToOpenGL.GetForegroundWindowFake();
    }
  }
}