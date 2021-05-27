
#include "pch.h"

#include "windowCore.h"
#include "fakeSurfaces.h"


namespace UCPtoOpenGL
{

  /** fake helper **/

  // Fun Fact: The menu border in the editor is broken at access, even in vanilla.

  void FakeSurface::FakeBlt(unsigned short* bltTo, int toX, int toY, int toWidth,
    unsigned short* bltFrom, int fromX, int fromY, int lenX, int lenY, int fromWidth)
  {
    // init
    bltFrom += fromY * fromWidth + fromX;
    bltTo += toY * toWidth + toX;

    // loop
    for (int yRun{ 0 }; yRun < lenY; yRun++)
    {
      for (int xRun{ 0 }; xRun < lenX; xRun++)
      {
        *bltTo = *bltFrom;  // all transparency seems to be handled by stronghold
        ++bltFrom;
        ++bltTo;
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
    DWORD, LPDDBLTFX)
  {
    // should not be reached by anything but fake
    FakeSurface* otherSurf{ (FakeSurface*)fromSurf };
    FakeBlt(bitData.get(), toRect->left, toRect->top, win->getTexStrongSizeW(), otherSurf->getBitmapPtr(), fromRect->left,
      fromRect->top, fromRect->right - fromRect->left, fromRect->bottom - fromRect->top, otherSurf->getSurfaceWidth());
    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeBackbuffer::BltFast(DWORD x, DWORD y, LPDIRECTDRAWSURFACE fromSurf,
    LPRECT fromRect, DWORD)
  {
    // should not be reached by anything but fake
    FakeSurface* otherSurf{ (FakeSurface*)fromSurf };
    FakeBlt(bitData.get(), x, y, win->getTexStrongSizeW(), otherSurf->getBitmapPtr(), fromRect->left,
      fromRect->top, fromRect->right - fromRect->left, fromRect->bottom - fromRect->top, otherSurf->getSurfaceWidth());
    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeBackbuffer::GetSurfaceDesc(LPDDSURFACEDESC descriptionPtr)
  {
    // let hope that this is enough

    DDSURFACEDESC& des{ *descriptionPtr };
    des.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;

    des.dwHeight = win->getTexStrongSizeH();
    des.dwWidth = win->getTexStrongSizeW();

    des.lPitch = 2 * des.dwWidth;  // lPitch is the number of bytes from the start of one line to the next
    des.lpSurface = getBitmapPtr();


    // Stronghold supports multiple pixel formats -> what the game would "prefer" still needs more research
    des.ddpfPixelFormat.dwSize = sizeof(des.ddpfPixelFormat);
    des.ddpfPixelFormat.dwFlags = DDPF_RGB;       // it is RGB
    des.ddpfPixelFormat.dwRGBBitCount = 16;       // Stronghold uses 16bit

    /*
    // this is the 565 one
    des.ddpfPixelFormat.dwRBitMask = 0xf800;          // 1111 1000 0000 0000
    des.ddpfPixelFormat.dwGBitMask = 0x7e0;           // 0000 0111 1110 0000
    des.ddpfPixelFormat.dwBBitMask = 0x1F;            // 0000 0000 0001 1111
    des.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0;      // 0000 0000 0000 0000
    */
    
    // this is ARGB 1555, I assume
    des.ddpfPixelFormat.dwRBitMask = 0x7c00;          // 0111 1100 0000 0000
    des.ddpfPixelFormat.dwGBitMask = 0x3e0;           // 0000 0011 1110 0000
    des.ddpfPixelFormat.dwBBitMask = 0x1F;            // 0000 0000 0001 1111
    des.ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;   // 1000 0000 0000 0000
    

    des.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_SYSTEMMEMORY;

    return DD_OK;
  }


  /** fake offscreen main **/

  STDMETHODIMP_(HRESULT __stdcall) FakeOffscreenMain::Lock(LPRECT, LPDDSURFACEDESC descriptionPtr, DWORD, HANDLE)
  {
    // let hope that this is enough

    DDSURFACEDESC& des{ *descriptionPtr };
    des.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_CAPS;

    des.dwHeight = win->getTexStrongSizeH();
    des.dwWidth = win->getTexStrongSizeW();

    des.lPitch = 2 * des.dwWidth; // lPitch is the number of bytes from the start of one line to the next
    des.lpSurface = getBitmapPtr();

    des.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeOffscreenMain::GetPixelFormat(LPDDPIXELFORMAT format)
  {
    DDPIXELFORMAT& pix{ *format };

    // Stronghold supports multiple pixel formats -> what the game would "prefer" still needs more research
    pix.dwFlags = DDPF_RGB;       // it is RGB
    pix.dwRGBBitCount = 16;       // Stronghold uses 16bit

    /*
    // this is the 565 one
    pix.dwRBitMask = 0xf800;          // 1111 1000 0000 0000
    pix.dwGBitMask = 0x7e0;           // 0000 0111 1110 0000
    pix.dwBBitMask = 0x1F;            // 0000 0000 0001 1111
    pix.dwRGBAlphaBitMask = 0x0;      // 0000 0000 0000 0000
    */
    
    // this is ARGB 1555, I assume
    pix.dwRBitMask = 0x7c00;          // 0111 1100 0000 0000
    pix.dwGBitMask = 0x3e0;           // 0000 0011 1110 0000
    pix.dwBBitMask = 0x1F;            // 0000 0000 0001 1111
    pix.dwRGBAlphaBitMask = 0x8000;   // 1000 0000 0000 0000
    

    return DD_OK;
  }


  /** fake offscreen map **/

  STDMETHODIMP_(HRESULT __stdcall) FakeOffscreenMap::Lock(LPRECT, LPDDSURFACEDESC descriptionPtr, DWORD, HANDLE)
  {
    // let hope that this is enough

    DDSURFACEDESC& des{ *descriptionPtr };
    des.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_CAPS;

    des.dwHeight = 2076;
    des.dwWidth = 4056;

    des.lPitch = 8112; // 4056 * 2; lPitch is the number of BYTES from the start of one line to the next
    des.lpSurface = getBitmapPtr();

    des.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    return DD_OK;
  }
}
