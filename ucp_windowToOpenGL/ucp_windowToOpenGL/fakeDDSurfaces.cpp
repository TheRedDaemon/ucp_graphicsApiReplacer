
#include "pch.h"

#include "fakeDDClasses.h"


namespace UCPtoOpenGL
{
  /** FakeSurface **/


  // to reduce writing the set structure
  void FakeDirectDraw::FakeSurface::fillPixelFormat(LPDDPIXELFORMAT pixPtr, PixelFormat pixFormat)
  {
    DDPIXELFORMAT& pix{ *pixPtr };
    ZeroMemory(&pix, sizeof(pix)); // Zero structure

    pix.dwSize = sizeof(pix);
    pix.dwFlags = DDPF_RGB;       // it is RGB
    pix.dwRGBBitCount = 16;       // Stronghold uses 16bit

    switch (pixFormat)
    {
      case RGB_565:
      {
        // this is the 565 one
        pix.dwRBitMask = 0xf800;          // 1111 1000 0000 0000
        pix.dwGBitMask = 0x7e0;           // 0000 0111 1110 0000
        pix.dwBBitMask = 0x1F;            // 0000 0000 0001 1111
        pix.dwRGBAlphaBitMask = 0x0;      // 0000 0000 0000 0000
        break;
      }
      case ARGB_1555:
      default:
      {
        // this is ARGB 1555, I assume
        pix.dwRBitMask = 0x7c00;          // 0111 1100 0000 0000
        pix.dwGBitMask = 0x3e0;           // 0000 0011 1110 0000
        pix.dwBBitMask = 0x1F;            // 0000 0000 0001 1111
        pix.dwRGBAlphaBitMask = 0x8000;   // 1000 0000 0000 0000
        break;
      }
    }
  }

  // Fun Fact: The menu border in the editor is broken at access, even in vanilla.

  void FakeDirectDraw::FakeSurface::FakeBlt(unsigned short* bltTo, int toX, int toY, int toWidth,
    unsigned short* bltFrom, int fromX, int fromY, int lenX, int lenY, int fromWidth)
  {
    // init
    bltFrom += fromY * fromWidth + fromX;
    bltTo += toY * toWidth + toX;

    // loop
    int cpyLen{ lenX * 2 };
    for (int yRun{ 0 }; yRun < lenY; yRun++)
    {
      std::memcpy(bltTo, bltFrom, cpyLen); // all transparency seems to be handled by stronghold
      bltFrom += fromWidth;
      bltTo += toWidth;
    }
  }



  /** FakePrimary **/

  STDMETHODIMP_(HRESULT __stdcall) FakeDirectDraw::FakePrimary::Flip(LPDIRECTDRAWSURFACE, DWORD)
  {
    return parentPtr->renderNextFrame(parentPtr->back.getBitmapPtr());
  }



  /** FakeBackbuffer **/

  unsigned short* FakeDirectDraw::FakeBackbuffer::getBitmapPtr()
  {
    return bitData.get();
  }

  void FakeDirectDraw::FakeBackbuffer::createBitData(int size)
  {
    bitData = std::make_unique<unsigned short[]>(size);
  }

  void FakeDirectDraw::FakeBackbuffer::getBltData(IN LPDIRECTDRAWSURFACE otherSurf, OUT unsigned short** bitDataPtr, OUT int* otherSurfWidthPtr)
  {
    // should not be reached by anything but fakeOffscreen
    if (otherSurf == &parentPtr->offMain)
    {
      *bitDataPtr = parentPtr->offMain.getBitmapPtr();
      *otherSurfWidthPtr = parentPtr->getRenderTexWidth();
    }
    else // is offscreen Mmap
    {
      *bitDataPtr = parentPtr->offMap.getBitmapPtr();
      *otherSurfWidthPtr = 4056;
    }
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeDirectDraw::FakeBackbuffer::Blt(LPRECT toRect, LPDIRECTDRAWSURFACE fromSurf, LPRECT fromRect,
    DWORD, LPDDBLTFX)
  {
    unsigned short* otherSurfData;
    int otherSurfWidth;
    getBltData(fromSurf, &otherSurfData, &otherSurfWidth);

    FakeBlt(bitData.get(), toRect->left, toRect->top, parentPtr->getRenderTexWidth(), otherSurfData, fromRect->left,
      fromRect->top, fromRect->right - fromRect->left, fromRect->bottom - fromRect->top, otherSurfWidth);
    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeDirectDraw::FakeBackbuffer::BltFast(DWORD x, DWORD y, LPDIRECTDRAWSURFACE fromSurf,
    LPRECT fromRect, DWORD)
  {
    unsigned short* otherSurfData;
    int otherSurfWidth;
    getBltData(fromSurf, &otherSurfData, &otherSurfWidth);

    FakeBlt(bitData.get(), x, y, parentPtr->getRenderTexWidth(), otherSurfData, fromRect->left,
      fromRect->top, fromRect->right - fromRect->left, fromRect->bottom - fromRect->top, otherSurfWidth);
    return DD_OK;
  }



  /** FakeOffscreenMain **/

  unsigned short* FakeDirectDraw::FakeOffscreenMain::getBitmapPtr()
  {
    return bitData.get();
  }

  void FakeDirectDraw::FakeOffscreenMain::createBitData(int size)
  {
    bitData = std::make_unique<unsigned short[]>(size);
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeDirectDraw::FakeOffscreenMain::Lock(LPRECT, LPDDSURFACEDESC descriptionPtr, DWORD, HANDLE)
  {
    // let hope that this is enough

    DDSURFACEDESC& des{ *descriptionPtr };
    des.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_CAPS;

    des.dwHeight = parentPtr->getRenderTexHeight();
    des.dwWidth = parentPtr->getRenderTexWidth();

    des.lPitch = 2 * des.dwWidth; // lPitch is the number of bytes from the start of one line to the next
    des.lpSurface = getBitmapPtr();

    des.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    return DD_OK;
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeDirectDraw::FakeOffscreenMain::GetPixelFormat(LPDDPIXELFORMAT format)
  {
    fillPixelFormat(format, parentPtr->getPixelFormat());
    return DD_OK;
  }



  /** FakeOffscreenMap **/

  unsigned short* FakeDirectDraw::FakeOffscreenMap::getBitmapPtr()
  {
    return bitData.get();
  }

  STDMETHODIMP_(HRESULT __stdcall) FakeDirectDraw::FakeOffscreenMap::Lock(LPRECT, LPDDSURFACEDESC descriptionPtr, DWORD, HANDLE)
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

  STDMETHODIMP_(HRESULT __stdcall) FakeDirectDraw::FakeOffscreenMap::GetPixelFormat(LPDDPIXELFORMAT format)
  {
    fillPixelFormat(format, parentPtr->getPixelFormat());
    return DD_OK;
  }
}