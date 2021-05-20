
#include "pch.h"

#include <string>

#include "windowCore.h"
#include "crusaderToOpenGL.h"

// lua calls

HWND CrusaderToOpenGL::createWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
	int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{

	if (window.createWindow())
	{
		windowDone = true;
		return window.getWindowHandle();
	}

	// if it should fail:
	return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

HRESULT CrusaderToOpenGL::createDirectDraw(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
{
	// get library and func
	HMODULE ddraw{ GetModuleHandleA("ddraw.dll") };
	if (ddraw == NULL) return DDERR_GENERIC;	// lets hope the game just crashes normally... need better handling... but what?
	auto create{ (decltype(DirectDrawCreate)*)GetProcAddress(ddraw, "DirectDrawCreate") };
	
	HRESULT res;
	if (windowDone)
	{
		// use own if successful
		res = create(lpGUID, &realInterface, pUnkOuter);
		*lplpDD = this;
	}
	else{
		res = create(lpGUID, lplpDD, pUnkOuter);
	}

	return res;
}

// DirectDraw

STDMETHODIMP_(HRESULT __stdcall) CrusaderToOpenGL::SetDisplayMode(DWORD w, DWORD h, DWORD)
{
	window.setTexStrongSize(w, h);
	return DD_OK;
}
