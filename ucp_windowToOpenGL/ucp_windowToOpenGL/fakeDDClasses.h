#pragma once

namespace UCPtoOpenGL
{
  class FakeDirectDraw : public IDirectDraw
  {
  public:

    FakeDirectDraw() {};

    virtual ~FakeDirectDraw() {};

    // Inferface methods: Most are changed to do nothing but return DD_OK

    /*
      Parts of the interfaces were taken from the ddraw_logger: https://github.com/JeremyAnsel/ddraw_logger
      Copyright (c) 2014 Jérémy Ansel
      Licensed under the MIT license.
    */

    /*** not used ***/

    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID FAR*)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD_(ULONG, AddRef) (THIS)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(Compact)(THIS)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR*)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR*)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR*)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(FlipToGDISurface)(THIS)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(GetDisplayMode)(THIS_ LPDDSURFACEDESC)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR*)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(GetScanLine)(THIS_ LPDWORD)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(Initialize)(THIS_ GUID FAR*)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(RestoreDisplayMode)(THIS)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD)
    {
      return DD_OK;	// called, but ignored
    }
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE)
    {
      return DD_OK;	// not called by Crusader
    }
    STDMETHOD_(ULONG, Release) (THIS)
    {
      return DD_OK; // called, by now ignored, there is no interface to free
    }

    /*** modified ***/

    // uses windows function to get the display modes
    STDMETHOD(EnumDisplayModes)(THIS_ DWORD dw, LPDDSURFACEDESC lpsurf, LPVOID lpvoid, LPDDENUMMODESCALLBACK callback) = 0;
    // return as if no compatibility
    STDMETHOD(GetCaps)(THIS_ LPDDCAPS cap1, LPDDCAPS cap2) = 0;
    // needs handling
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR*, IUnknown FAR*) = 0;
    // needs handling -> place to get current resolution
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD, DWORD) = 0;

  
  private:

    /*
      The interface structure was taken from the ddraw_logger: https://github.com/JeremyAnsel/ddraw_logger
      Copyright (c) 2014 Jérémy Ansel
      Licensed under the MIT license.
    */
    class FakeSurface : public IDirectDrawSurface
    {
    public:

      // Most methods are not used, instead, they return DD_OK and are done.
      // Some others might be used by the fake surfaces, and are virtual as a result

      FakeSurface(FakeDirectDraw* interfacePtr) : parentPtr(interfacePtr) {};

      virtual ~FakeSurface() {};


      /*** not used ***/

      STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID FAR*)
      {
        return DD_OK; // not called
      }
      STDMETHOD_(ULONG, AddRef) (THIS)
      {
        return DD_OK;  // not called
      }
      STDMETHOD_(ULONG, Release) (THIS)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD, LPDIRECTDRAWSURFACE)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID, LPDDENUMSURFACESCALLBACK)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD, LPVOID, LPDDENUMSURFACESCALLBACK)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(GetBltStatus)(THIS_ DWORD)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(GetCaps)(THIS_ LPDDSCAPS)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(GetDC)(THIS_ HDC FAR*)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(GetFlipStatus)(THIS_ DWORD)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(IsLost)(THIS)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(ReleaseDC)(THIS_ HDC)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDOVERLAYFX)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE)
      {
        return DD_OK;  // not called
      }
      STDMETHOD(Restore)(THIS)
      {
        return DD_OK;  // called on Focus switch, but we do not care
      }
      // called by the offscreen surfaces
      STDMETHOD(Unlock)(THIS_ LPVOID)
      {
        return DD_OK; // we do not care
      }


      /*** used ***/

      // called by backbuffer, surfaces need handling
      STDMETHOD(Blt)(THIS_ LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX)
      {
        return DD_OK;
      }
      // called by backbuffer, surfaces need handling
      STDMETHOD(BltFast)(THIS_ DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD)
      {
        return DD_OK;
      }
      // called by the primary surface
      STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD)
      {
        return DD_OK;
      }
      // called by the primary surface to get backbuffer
      STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR*)
      {
        return DD_OK;
      }
      // called by the backbuffer once
      STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC)
      {
        return DD_OK; // needs proper handling, relevant infos need to be added
      }
      // called by the offscreen surfaces
      STDMETHOD(Lock)(THIS_ LPRECT, LPDDSURFACEDESC, DWORD, HANDLE)
      {
        return DD_OK;
      }
      // called, BUT actually from bink video -> should have taken this serious
      // offscreen main need handling
      STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT)
      {
        return DD_OK;
      }

    protected:

      FakeDirectDraw* const parentPtr;

      // The blt methods are an issue. For the moment I ignore potential scaling
      // and also guess that black (NULL) will be transparent: -> works
      void FakeBlt(unsigned short* bltTo, int toX, int toY, int toWidth,
        unsigned short* bltFrom, int fromX, int fromY, int lenX, int lenY, int fromWidth);

      static void fillPixelFormat(LPDDPIXELFORMAT, PixelFormat);

    };


    /*** Real FakeSurfaces ***/

    // will fake the primary surface
    class FakePrimary : public FakeSurface
    {
    public:
      FakePrimary(FakeDirectDraw* parentPtr) : FakeSurface(parentPtr) {};
      ~FakePrimary() {};

      STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD) override;
      STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR*) override;
    };


    // will fake the backbuffer surface
    class FakeBackbuffer : public FakeSurface
    {
    public:
      FakeBackbuffer(FakeDirectDraw* parentPtr) : FakeSurface(parentPtr) {};
      ~FakeBackbuffer() {};

      STDMETHOD(Blt)(THIS_ LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX) override;
      STDMETHOD(BltFast)(THIS_ DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD) override;
      STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) override;

      unsigned short* getBitmapPtr();
      void createBitData(int size);

    private:

      // 16 bpp -> short; needs to change
      std::unique_ptr<unsigned short[]> bitData{};

      void getBltData(IN LPDIRECTDRAWSURFACE otherSurf, OUT unsigned short** bitDataPtr, OUT int* otherSurfWidthPtr);
    };


    // will fake the fake main offscreen surface
    class FakeOffscreenMain : public FakeSurface
    {
    public:
      FakeOffscreenMain(FakeDirectDraw* parentPtr) : FakeSurface(parentPtr) {};
      ~FakeOffscreenMain() {};

      STDMETHOD(Lock)(THIS_ LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) override;
      STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) override;

      unsigned short* getBitmapPtr();
      void createBitData(int size);

    private:

      // 16 bpp -> short; needs to change
      std::unique_ptr<unsigned short[]> bitData{};
    };


    // will fake the map offscreen surface
    class FakeOffscreenMap : public FakeSurface
    {
    public:
      FakeOffscreenMap(FakeDirectDraw* parentPtr) : FakeSurface(parentPtr) {};
      ~FakeOffscreenMap() {};

      STDMETHOD(Lock)(THIS_ LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) override;

      unsigned short* getBitmapPtr();

    private:

      // 16 bpp -> short; unchanged, but need to go on "heap"
      const std::unique_ptr<unsigned short[]> bitData{ std::make_unique<unsigned short[]>(4056 * 2076) };
    };

    // funcs used to call get information for surfaces

    virtual HRESULT renderNextFrame(unsigned short* sourcePtr) = 0;
    virtual int getRenderTexWidth() = 0;
    virtual int getRenderTexHeight() = 0;
    virtual PixelFormat getPixelFormat() = 0;

  protected:

    FakePrimary prim{ this };
    FakeBackbuffer back{ this };
    FakeOffscreenMain offMain{ this };
    FakeOffscreenMap offMap{ this };
  };
}
