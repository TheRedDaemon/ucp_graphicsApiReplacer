
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
        // remove return address (eax should be free to modify
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
        // remove return address (eax should be free to modify
        mov     eax, dword ptr[esp]
        add     esp, 4

        // push
        push    ecx   // shc struct
        push    dword ptr[FillAddress::WinSetRectObjBaseAddr]
        push    eax   // set ret address again

        mov     ecx, offset Control::ToOpenGL // mov toOpenGL this pointer

        jmp     CrusaderToOpenGL::drawInit  // jump to actual func
      }
    }


    HRESULT WINAPI DirectDrawCreateCall(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
    {
      return Control::ToOpenGL.createDirectDraw(lpGUID, lplpDD, pUnkOuter);
    }

    // stronghold gets the size of the screen sometimes
    // only handles 0 and 1, the first SetDisplayMode needs to happen before
    // TODO: Replacing the jump routes all code through here, maybe there are less hard methods?
    //  - depends on how other stuff gets handeld
    int WINAPI GetSystemMetricsCall(int nIndex)
    {
      return Control::ToOpenGL.getFakeSystemMetrics(nIndex);
    }


    // set window call -> simply deactivate
    BOOL WINAPI SetWindowPosCall(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlag)
    {
      return Control::ToOpenGL.setWindowPosFake(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlag);
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

    // further tests

    // what does it update?
    BOOL WINAPI UpdateWindowCall(HWND hWnd)
    {
      return Control::ToOpenGL.updateWindowFake(hWnd);
    }

    BOOL WINAPI AdjustWindowRectCall(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
    {
      return Control::ToOpenGL.adjustWindowRectFake(lpRect, dwStyle, bMenu);
    };


    LONG WINAPI SetWindowLongACall(HWND hWnd, int nIndex, LONG dwNewLong)
    {
      return Control::ToOpenGL.setWindowLongAFake(hWnd, nIndex, dwNewLong);
    }


    // first chars -> dll name, function name
    void WINAPI DetouredWindowLongPtrReceive(char*, char*, DWORD* ptrToWindowLongPtr, DWORD, DWORD)
    {
      // e8 call -> but needs direct address
      *ptrToWindowLongPtr = reinterpret_cast<DWORD>(SetWindowLongACall);
    }
  }
}