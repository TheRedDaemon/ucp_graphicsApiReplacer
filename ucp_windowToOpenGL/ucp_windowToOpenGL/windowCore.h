#pragma once

namespace UCPtoOpenGL
{
	class WindowCore
	{
	public:
		WindowCore() {};
		~WindowCore() {};

		bool createWindow();
		HWND getWindowHandle();

		void setTexStrongSize(int w, int h);

		HRESULT renderNextScreen(unsigned short* backData);

	private:

		// pointer to openGL Window
		GLFWwindow* window = nullptr;

		// data infos:
		int strongTexW{ 0 };
		int strongTexH{ 0 };

	};
}