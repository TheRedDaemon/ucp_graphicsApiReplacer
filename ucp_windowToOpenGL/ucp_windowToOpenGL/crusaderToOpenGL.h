
#pragma once

class CrusaderToOpenGL : public IDirectDraw
{
public:
	CrusaderToOpenGL();

	virtual ~CrusaderToOpenGL();

	// Inferface methods: Most are changed to do nothing but return DD_OK

	/*
		Parts of the interfaces were taken from the ddraw_logger: https://github.com/JeremyAnsel/ddraw_logger
		Copyright (c) 2014 Jérémy Ansel
		Licensed under the MIT license. See LICENSE.txt
	*/

	/*** IUnknown methods ***/

	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj)
	{
		return DD_OK;	// not called by Crusader
	}

	STDMETHOD_(ULONG, AddRef) (THIS)
	{
		return DD_OK;	// not called by Crusader
	}

	STDMETHOD_(ULONG, Release) (THIS)
	{
		return DD_OK;	// called, but not important, realInterface is kept for re-init, until this is stopped
	}

	/*** IDirectDraw methods ***/

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

	// needs handling
	STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR*, IUnknown FAR*);

	STDMETHOD(DuplicateSurface)(THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR*)
	{
		return DD_OK;	// not called by Crusader
	}

	STDMETHOD(EnumDisplayModes)(THIS_ DWORD dw, LPDDSURFACEDESC lpsurf, LPVOID lpvoid, LPDDENUMMODESCALLBACK callback)
	{
		// calls original, mode for values is set somewhere else
		return realInterface->EnumDisplayModes(dw, lpsurf, lpvoid, callback);
	}

	STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK)
	{
		return DD_OK;	// not called by Crusader
	}

	STDMETHOD(FlipToGDISurface)(THIS)
	{
		return DD_OK;	// not called by Crusader
	}

	STDMETHOD(GetCaps)(THIS_ LPDDCAPS cap1, LPDDCAPS cap2)
	{
		// calls original, since there are currently many open questions regarding the needed return
		return realInterface->GetCaps(cap1, cap2);
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

	// needs handling -> place to get current resolution
	STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD, DWORD);

	STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE)
	{
		return DD_OK;	// not called by Crusader
	}

	/* copied structure end */

	HWND createWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y,
		int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

	HRESULT createDirectDraw(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter);

private:

	WindowCore window;
	bool windowDone{ false };

	// DirectDraw interface
	IDirectDraw* realInterface = nullptr;

};