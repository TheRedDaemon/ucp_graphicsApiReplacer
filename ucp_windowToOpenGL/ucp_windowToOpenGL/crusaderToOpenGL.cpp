
#include "pch.h"

#include <string>

#include "windowCore.h"
#include "fakeSurfaces.h"
#include "crusaderToOpenGL.h"

#include <chrono>
#include <thread>


namespace UCPtoOpenGL
{

	// lua calls

	HWND CrusaderToOpenGL::createWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
		int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
	{
		std::this_thread::sleep_for(std::chrono::seconds(10)); // 20 seconds to attach

		// prevent second window -> TODO: Test ALT TAB
		if (window.getWindowHandle() == NULL && window.createWindow())
		{
			windowDone = true;
			//return window.getWindowHandle();
		}
		else
		{
			windowDone = false;
		}

		// if it fails:
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
				//texWidth = 1024; // test
				if (texWidth > 0)
				{
					return texWidth;
				}
				break;
			}
			case SM_CYSCREEN:
			{
				int texHeight{ window.getTexStrongSizeH() };
				//texHeight = 768; // test
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
