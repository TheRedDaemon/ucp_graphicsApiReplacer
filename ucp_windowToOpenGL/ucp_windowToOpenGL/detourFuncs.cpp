
#include "pch.h"

#include "controlAndDetour.h"
#include "crusaderToOpenGL.h"

namespace UCPtoOpenGL
{
  namespace DetourFunc
  {
    // using fastCall hack to get this in global function
    // source: https://www.unknowncheats.me/forum/c-and-c-/154364-detourfunction-__thiscall.html
    // note: the second parameter is EDX and a dummy that should be ignored!
    bool __fastcall CreateWindowComplete(SHCWindowOrMainStructFake* that, DWORD, LPSTR windowName, unsigned int unknown)
    {
      return Control::ToOpenGL.createWindow(that, windowName, unknown, Control::WindowProcCallbackFake);
    }

    // callback function missing
    bool __declspec(naked) NakedCreateWindowComplete()
    {
      __asm {
        push    ecx   // push that pointer
        push    Control::WindowProcCallbackFake
        mov     ecx, Control::ToOpenGL // mov toOpenGL this pointer
        jmp     CrusaderToOpenGL::createWindow  // jump to actual func
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

    // the main drawing rect is set via a user call -> keep the pointer, change it on demand
    // TODO: Replacing the jump routes all code through here, maybe there are less hard methods?
    //  - depends on how other stuff gets handeld

    BOOL WINAPI SetRectCall(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
    {
      return Control::ToOpenGL.setFakeRect(lprc, xLeft, yTop, xRight, yBottom);
    };


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