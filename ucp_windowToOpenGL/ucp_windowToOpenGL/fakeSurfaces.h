#pragma once


/*
	The interface structure was taken from the ddraw_logger: https://github.com/JeremyAnsel/ddraw_logger
	Copyright (c) 2014 Jérémy Ansel
	Licensed under the MIT license. See LICENSE.txt
*/
class FakeSurface : public IDirectDrawSurface
{
public:

	// Most methods are not used, instead, they return DD_OK and are done.
	// Some others might be used by the fake surfaces, and are virtual as a result

	FakeSurface() {};

	virtual ~FakeSurface() {};

	
	/*** not used ***/

	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj)
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

	STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT)
	{
		return DD_OK;  // called, BUT I think from ddraw itself, to this should not happen here
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
	virtual STDMETHOD(Blt)(THIS_ LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX)
	{
		return DD_OK; 
	}

	// called by backbuffer, surfaces need handling
	virtual STDMETHOD(BltFast)(THIS_ DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD)
	{
		return DD_OK;
	}

	// called by the primary surface
	virtual STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD)
	{
		return DD_OK; 
	}

	// called by the primary surface to get backbuffer
	virtual STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR*)
	{
		return DD_OK;
	}

	// called by the backbuffer once
	virtual STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC)
	{
		return DD_OK; // needs proper handling, relevant infos need to be added
	}

	// called by the offscreen surfaces
	virtual STDMETHOD(Lock)(THIS_ LPRECT, LPDDSURFACEDESC, DWORD, HANDLE)
	{
		return DD_OK;
	}

protected:
	
	// helper
	virtual unsigned short* getBitmapPtr()
	{
		return nullptr;	// should only get called where implemented
	}
};


// will fake the primary surface
class FakePrimary : public FakeSurface
{
public:
	FakePrimary();
	~FakePrimary();

	STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD) override;
	STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR*) override;

private:

};


// will fake the backbuffer surface
class FakeBackbuffer : public FakeSurface
{
public:
	FakeBackbuffer();
	~FakeBackbuffer();

	STDMETHOD(Blt)(THIS_ LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX) override;
	STDMETHOD(BltFast)(THIS_ DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD) override;
	STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) override;

private:

	// 16 bpp -> short; needs to change
	std::unique_ptr<unsigned short[]> bitData; // = std::make_unique<unsigned short[]>(16000);

};


// will fake the OffscreenNormal surface
class FakeOffscreenNormal : public FakeSurface
{
public:
	FakeOffscreenNormal();
	~FakeOffscreenNormal();

	STDMETHOD(Lock)(THIS_ LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) override;

protected:

	// helper
	unsigned short* getBitmapPtr() override
	{
		return bitData.get();
	}


private:

	// 16 bpp -> short; needs to change
	std::unique_ptr<unsigned short[]> bitData; // = std::make_unique<unsigned short[]>(16000);
};


// will fake the FakeOffscreenMap surface
class FakeOffscreenMap : public FakeSurface
{
public:
	FakeOffscreenMap();
	~FakeOffscreenMap();

	STDMETHOD(Lock)(THIS_ LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) override;

protected:

	// helper
	unsigned short* getBitmapPtr() override
	{
		return bitData;
	}

private:

	// 16 bpp -> short; unchanged
	unsigned short bitData[4056 * 2076];
};