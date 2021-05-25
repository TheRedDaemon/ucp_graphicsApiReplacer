
#include "pch.h"

#include <string>

#include "windowCore.h"
#include "fakeSurfaces.h"
#include "crusaderToOpenGL.h"


namespace UCPtoOpenGL
{

  // lua calls

  bool CrusaderToOpenGL::createWindow(DWORD that, LPSTR windowName, unsigned int unknown, WNDPROC keyboardCallbackFunc)
  {
    /* recreated original window func
    HINSTANCE hInstance{ *(HINSTANCE*)(that + 0xA8) };
    LPCSTR className{ "FFwinClass" };

    WNDCLASSA wndClass;
    wndClass.hInstance = hInstance;
    wndClass.lpfnWndProc = keyboardCallbackFunc;
    wndClass.style = NULL;	// this needs changes later
    wndClass.cbClsExtra = NULL;
    wndClass.cbWndExtra = NULL;
    wndClass.hIcon = LoadIconA(hInstance, (LPCSTR)(unknown & 0xFFFF));	// weird... is this a downcast, so unknown would be a address?
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = NULL;
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = className;

    ATOM classAtom{ RegisterClassA(&wndClass) };
    if (classAtom == NULL)
    {
      return false;
    }

    HWND win{
      CreateWindowExA(
        NULL,
        className,
        windowName,
        WS_POPUP | WS_VISIBLE, // WS_POPUP is apperantly an indicator that the window is only short li
        0,
        0,
        GetSystemMetrics(0),	// at this point, no hint exist as to how big it should be
        GetSystemMetrics(1),
        NULL,
        NULL,
        hInstance,
        NULL
      )
    };
    */

    // try to change stuff
    HINSTANCE hInstance{ *(HINSTANCE*)(that + 0xA8) };
    LPCSTR className{ "FFwinClass" };

    WNDCLASSA wndClass;
    wndClass.hInstance = hInstance;
    wndClass.lpfnWndProc = keyboardCallbackFunc;
    wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;	// CS_OWNDC apperantly needed to allow a constant device context CS_HREDRAW | CS_VREDRAW
    wndClass.cbClsExtra = NULL;
    wndClass.cbWndExtra = NULL;
    wndClass.hIcon = LoadIconA(hInstance, (LPCSTR)(unknown & 0xFFFF));	// weird... is this a downcast, so unknown would be a address?
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = NULL;
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = className;

    ATOM classAtom{ RegisterClassA(&wndClass) };
    if (classAtom == NULL)
    {
      return false;
    }

    HWND win{
      CreateWindowExA(
        NULL,
        className,
        windowName,
        WS_OVERLAPPED | WS_VISIBLE, // WS_POPUP is apperantly an indicator that the window is only short lift... changed it to overlap
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        GetSystemMetrics(0),	// at this point, no hint exist as to how big it should be
        GetSystemMetrics(1),
        NULL,
        NULL,
        hInstance,
        NULL
      )
    };

    // this is an isseu -> if something here fails, then the whole thing might be busted...
    // TODO: is there a possible way around? -> close everything until then, then recreate with original Stronghold?
    // until then, this will return false, and I assume stronghold will close
    windowDone = win && window.createWindow(win);


    // end -> do not touch
    *(DWORD*)(that + 0xAC) = (DWORD)win;
    return win != NULL;
  }

  HRESULT CrusaderToOpenGL::createDirectDraw(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
  {
    // get library and func (just once)
    static decltype(DirectDrawCreate)* create{ nullptr };
    if (create == nullptr)
    {
      HMODULE ddraw{ GetModuleHandleA("ddraw.dll") };
      if (ddraw == NULL) return DDERR_GENERIC;	// lets hope the game just crashes normally... need better handling... but what?
      create = (decltype(DirectDrawCreate)*)GetProcAddress(ddraw, "DirectDrawCreate");
    }

    HRESULT res;
    if (windowDone)
    {
      // use own if successful
      res = create(lpGUID, &realInterface, pUnkOuter);
      *lplpDD = this;
    }
    else
    {
      res = create(lpGUID, lplpDD, pUnkOuter);
    }

    return res;
  }

  int CrusaderToOpenGL::getFakeSystemMetrics(int nIndex)
  {
    // does not garantuee that the numbers are OK
    // likely not fitting for many situation, not all stuff will need render size...
    if (windowDone)
    {
      switch (nIndex)
      {
      case SM_CXSCREEN:
      {
        int texWidth{ window.getTexStrongSizeW() };
        if (texWidth > 0)
        {
          return texWidth;
        }
        break;
      }
      case SM_CYSCREEN:
      {
        int texHeight{ window.getTexStrongSizeH() };
        if (texHeight > 0)
        {
          return texHeight;
        }
        break;
      }
      default:
        break;
      }
    }
    return GetSystemMetrics(nIndex);
  }

  BOOL CrusaderToOpenGL::setFakeRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
  {
    // test:
    //xRight = 1024;
    //yBottom = 768;

    if (mainDrawingRect == lprc)
    {
      yBottom = window.getTexStrongSizeH();
      xRight = window.getTexStrongSizeW();
    }

    return SetRect(lprc, xLeft, yTop, xRight, yBottom);
  }


  // DirectDraw

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::SetDisplayMode(DWORD w, DWORD h, DWORD)
  {
    //create new bit maps
    back.createBitData(w * h);
    offMain.createBitData(w * h);

    window.setTexStrongSize(w, h);

    if (mainDrawingRect)
    {
      RECT& rec{ *mainDrawingRect };
      rec.left = 0;
      rec.right = w;
      rec.top = 0;
      rec.bottom = h;
    }

    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::CreateSurface(LPDDSURFACEDESC des, LPDIRECTDRAWSURFACE* retSurfPtr, IUnknown*)
  {
    if (des->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
      *retSurfPtr = &prim;
      return DD_OK;
    }

    // offscreen surfaces (backbuffer is gathered different)
    if (des->dwHeight == 2076 && des->dwWidth == 4056)	// lets hope this resolution will never be supported
    {
      *retSurfPtr = &offMap;
    }
    else
    {
      *retSurfPtr = &offMain;
    }
    
    return DD_OK;
  }
}
