#pragma once

namespace UCPtoOpenGL
{
	class WindowCore
	{
	public:
		WindowCore() {};
		~WindowCore() {};

		bool createWindow(HWND win);

		void setTexStrongSize(int w, int h);
		int getTexStrongSizeW()
		{
			return strongTexW;
		}
		int getTexStrongSizeH()
		{
			return strongTexH;
		}

		// will likely later receive
		void setNewWindowStyle();
		
		HRESULT renderNextScreen(unsigned short* backData);

	private:

		// wgl stuff -> both of these should actually get released and deleted at the end
		// TODO: should it work, create another hook to release them... or let lua do it
		HWND winHandle{ 0 };	// the actual window -> stronghold should clean this up
		HDC deviceContext{ 0 };
		HGLRC renderingContext{ 0 };

		// data infos:
		int strongTexW{ 0 };
		int strongTexH{ 0 };

		// openGL pointer
		GLuint vertexArrayID{ NULL };
		GLuint quadBufferID{ NULL };
		GLuint quadIndexBuffer{ NULL };
		GLuint strongholdScreenTex{ NULL };

		// dummy currently 
		void initSystems();

		// also -> if possible (should one day the stuff work), try to get clean up point
	};
}