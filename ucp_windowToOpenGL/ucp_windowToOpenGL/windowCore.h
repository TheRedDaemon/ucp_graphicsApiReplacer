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

		HRESULT renderNextScreen(unsigned short* backData);

	private:

		// pointer to openGL Window
		GLFWwindow* window = nullptr;

		// wgl stuff -> both of these should actually get released and deleted at the end
		// TODO: should it work, create another hook to release them... or let lua do it
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