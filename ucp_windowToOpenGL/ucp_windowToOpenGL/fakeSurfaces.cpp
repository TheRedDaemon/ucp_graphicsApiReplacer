
#include "pch.h"

#include "windowCore.h"
#include "fakeSurfaces.h"

// for tests
//#include <intrin.h>

namespace UCPtoOpenGL
{

  /** fake helper **/

  void FakeSurface::FakeBlt(unsigned short* bltTo, int toX, int toY, int toWidth,
    unsigned short* bltFrom, int fromX, int fromY, int lenX, int lenY, int fromWidth)
  {
    // assumption:
    //  - the rectangles are ok
    //  - coords are between the pixels: 0->1600 means running from 0 to 1599

    // init
    bltFrom += fromY * fromWidth + fromX;
    bltTo += toY * toWidth + toX;
    for (int yRun{ 0 }; yRun < lenY; yRun++)
    {
      int maxX{ lenX + (int)bltTo };
      for (bltTo; (int)bltTo < maxX; bltTo++)
      {
        *bltTo = *bltFrom;  // all transparency seems to be handled by stronghold

        // test
        //if (*bltTo == NULL)
        //{
        //  *bltTo = 0x0000;
        //}

        ++bltFrom;
      }

      bltFrom += fromWidth - lenX;
      bltTo += toWidth - lenX;
    }
  }


  /** fake primary **/

  STDMETHODIMP_(HRESULT __stdcall) FakePrimary::Flip(LPDIRECTDRAWSURFACE, DWORD)
  {
    return win->renderNextScreen(back->getBitmapPtr());
  }

  STDMETHODIMP_(HRESULT __stdcall) FakePrimary::GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE* ptrToBackbuffer)
  {
    *ptrToBackbuffer = back;  // return fake backbuffer
    return DD_OK;
  }


  /** fake backbuffer **/

  STDMETHODIMP_(HRESULT __stdcall) FakeBackbuffer::Blt(LPRECT toRect, LPDIRECTDRAWSURFACE fromSurf, LPRECT fromRect,
    DWORD effectFlags, LPDDBLTFX)
  {
    if (effectFlags != DDBLT_WAIT)
    {
      int test = 1;
    }

    if (fromRect->right > win->getTexStrongSizeW())
    {
      int test = 1;
    }

    // debug -> for some reason I receive my resolution -> maybe the window is not able to contain the size?
    // could also be that the screen resolution would change, but it can not...?
    // TODO: find out how it constructs the rects -> if it pulls for all blts, this could be a problem

    // still issues -> the rect is still constantly set to screen size
    int lenX{ fromRect->right - fromRect->left };
    int lenY{ fromRect->bottom - fromRect->top };
    lenX = win->getTexStrongSizeW() < lenX ? win->getTexStrongSizeW() : lenX;
    lenY = win->getTexStrongSizeH() < lenY ? win->getTexStrongSizeH() : lenY;

    // debug
    //if (lenX != win->getTexStrongSizeW() || lenY != win->getTexStrongSizeH())
    //{
     // return DD_OK;
    //}


    // should not be reached by anything but fake
    FakeSurface* otherSurf{ (FakeSurface*)fromSurf };
    FakeBlt(bitData.get(), toRect->left, toRect->top, win->getTexStrongSizeW(), otherSurf->getBitmapPtr(), fromRect->left,
      fromRect->top, lenX, lenY, otherSurf->getSurfaceWidth());

    //int add = (int)_ReturnAddress();
    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeBackbuffer::BltFast(DWORD x, DWORD y, LPDIRECTDRAWSURFACE fromSurf,
    LPRECT fromRect, DWORD bltAction)
  {
    if (bltAction != DDBLTFAST_WAIT)
    {
      int test = 1;
    }

    // debug -> for some reason I receive my resolution -> maybe the window is not able to contain the size?
    // could also be that the screen resolution would change, but it can not...?
    // TODO: find out how it constructs the rects -> if it pulls for all blts, this could be a problem

    // still issues -> the rect is still constantly set to screen size
    int lenX{ fromRect->right - fromRect->left };
    int lenY{ fromRect->bottom - fromRect->top };
    lenX = win->getTexStrongSizeW() < lenX ? win->getTexStrongSizeW() : lenX;
    lenY = win->getTexStrongSizeH() < lenY ? win->getTexStrongSizeH() : lenY;

    // debug
    //if (lenX != win->getTexStrongSizeW() || lenY != win->getTexStrongSizeH())
    //{
    //  return DD_OK;
    //}

    // should not be reached by anything but fake
    FakeSurface* otherSurf{ (FakeSurface*)fromSurf };
    FakeBlt(bitData.get(), x, y, win->getTexStrongSizeW(), otherSurf->getBitmapPtr(), fromRect->left,
      fromRect->top, lenX, lenY, otherSurf->getSurfaceWidth());

    //int add = (int)_ReturnAddress();
    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeBackbuffer::GetSurfaceDesc(LPDDSURFACEDESC descriptionPtr)
  {
    // let hope that this is enough

    DDSURFACEDESC& des{ *descriptionPtr };
    des.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;

    des.dwHeight = win->getTexStrongSizeH();
    des.dwWidth = win->getTexStrongSizeW();

    des.lPitch = 0;
    des.lpSurface = getBitmapPtr();

    des.ddpfPixelFormat.dwSize = sizeof(des.ddpfPixelFormat);
    des.ddpfPixelFormat.dwFlags = DDPF_RGB;  // it is RGB
    des.ddpfPixelFormat.dwRGBBitCount = 16; // Stronghold uses 16bit

    des.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_SYSTEMMEMORY;

    return DD_OK;
  }


  /** fake offscreen main **/

  STDMETHODIMP_(HRESULT __stdcall) FakeOffscreenMain::Lock(LPRECT, LPDDSURFACEDESC descriptionPtr, DWORD, HANDLE)
  {
    // let hope that this is enough

    DDSURFACEDESC& des{ *descriptionPtr };
    des.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;

    des.dwHeight = win->getTexStrongSizeH();
    des.dwWidth = win->getTexStrongSizeW();

    des.lPitch = 0;
    des.lpSurface = getBitmapPtr();

    des.ddpfPixelFormat.dwSize = sizeof(des.ddpfPixelFormat);
    des.ddpfPixelFormat.dwFlags = DDPF_RGB;  // it is RGB
    des.ddpfPixelFormat.dwRGBBitCount = 16; // Stronghold uses 16bit

    des.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    return DD_OK;
  }


  /** fake offscreen map **/

  STDMETHODIMP_(HRESULT __stdcall) FakeOffscreenMap::Lock(LPRECT, LPDDSURFACEDESC descriptionPtr, DWORD, HANDLE)
  {
    // let hope that this is enough

    DDSURFACEDESC& des{ *descriptionPtr };
    des.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;

    des.dwHeight = 2076;
    des.dwWidth = 4056;

    des.lPitch = 0;
    des.lpSurface = getBitmapPtr();

    des.ddpfPixelFormat.dwSize = sizeof(des.ddpfPixelFormat);
    des.ddpfPixelFormat.dwFlags = DDPF_RGB;  // it is RGB
    des.ddpfPixelFormat.dwRGBBitCount = 16; // Stronghold uses 16bit

    des.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    return DD_OK;
  }
}


// NOTES:
//  - switch to game after loading causes issues: -> of they create another window there, it might be better to use this instead
//  - if they close the window there -> would be ok to use other window
