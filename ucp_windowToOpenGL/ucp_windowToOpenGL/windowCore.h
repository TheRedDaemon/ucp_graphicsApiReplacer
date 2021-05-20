#pragma once

class WindowCore
{
public:
	WindowCore() {};
	~WindowCore() {};

	bool createWindow();
	HWND getWindowHandle();
	
	void setTexStrongSize(int w, int h);

private:

	// pointer to openGL Window
	GLFWwindow* window = nullptr;

	// data infos:
	int strongTexW{ 0 };
	int strongTexH{ 0 };

};