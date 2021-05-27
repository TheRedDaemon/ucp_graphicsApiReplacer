#pragma once

namespace UCPtoOpenGL
{
  class WindowCore
  {
  public:
    WindowCore() {};
    ~WindowCore() {};

    bool createWindow(HWND win);

    void adjustTexSizeAndViewport(int wTex, int hTex, int wView, int hView, double scaleW, double scaleH);
    
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



    // load shader functions

    bool getAnyGLFuncAddress(const char* name, void** ptrToFuncPtr);

    // pointers
    PFNGLGENVERTEXARRAYSPROC ownPtr_glGenVertexArrays{ nullptr };
    PFNGLBINDVERTEXARRAYPROC ownPtr_glBindVertexArray{ nullptr };

    PFNGLGENBUFFERSPROC ownPtr_glGenBuffers{ nullptr };
    PFNGLBINDBUFFERPROC ownPtr_glBindBuffer{ nullptr };
    PFNGLBUFFERDATAPROC ownPtr_glBufferData{ nullptr };
    PFNGLBUFFERSUBDATAPROC ownPtr_glBufferSubData{ nullptr };

    PFNGLVERTEXATTRIBPOINTERPROC ownPtr_glVertexAttribPointer{ nullptr };
    PFNGLENABLEVERTEXATTRIBARRAYPROC ownPtr_glEnableVertexAttribArray{ nullptr };

    PFNGLCREATESHADERPROC ownPtr_glCreateShader{ nullptr };
    PFNGLSHADERSOURCEPROC ownPtr_glShaderSource{ nullptr };
    PFNGLCOMPILESHADERPROC ownPtr_glCompileShader{ nullptr };
    PFNGLATTACHSHADERPROC ownPtr_glAttachShader{ nullptr };
    PFNGLDETACHSHADERPROC ownPtr_glDetachShader{ nullptr };
    PFNGLDELETESHADERPROC ownPtr_glDeleteShader{ nullptr };

    PFNGLBINDATTRIBLOCATIONPROC ownPtr_glBindAttribLocation{ nullptr };
    PFNGLBINDFRAGDATALOCATIONPROC ownPtr_glBindFragDataLocation{ nullptr };

    PFNGLCREATEPROGRAMPROC ownPtr_glCreateProgram{ nullptr };
    PFNGLLINKPROGRAMPROC ownPtr_glLinkProgram{ nullptr };
    PFNGLUSEPROGRAMPROC ownPtr_glUseProgram{ nullptr };

    bool loadGLFunctions();
  };
}