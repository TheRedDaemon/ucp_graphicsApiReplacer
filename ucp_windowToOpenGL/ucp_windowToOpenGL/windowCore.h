#pragma once

namespace UCPtoOpenGL
{
  class WindowCore
  {
  public:
    WindowCore();
    ~WindowCore();

    void setConf(ToOpenGLConfig* conf)
    {
      confPtr = conf;
    };

    bool loadWGLFunctions(HINSTANCE hInstance);  // creates fake context to get OpenGL functions
    bool createWindow(HWND win);

    // does nothing, only sets tex size
    void setOnlyTexSize(Size<int> texSize);

    void adjustTexSizeAndViewport(Size<int> texSize, Size<int> viewSize, Size<double> scale);
    
    Size<int> getTexStrongSize()
    {
      return strongTexSize;
    }
    
    HRESULT renderNextScreen(unsigned short* backData);

    void releaseContext(HWND hwnd);  // for a bit clean up

  private:

    // config:
    ToOpenGLConfig* confPtr{ nullptr };
    PixelFormat pixFormat{ ARGB_1555 };

    // wgl stuff -> both of these should actually get released and deleted at the end
    // TODO: should it work, create another hook to release them... or let lua do it
    HDC deviceContext{ 0 };
    HGLRC renderingContext{ 0 };

    // data infos:
    Size<int> strongTexSize{ 0, 0 };

    // openGL pointer
    GLuint vertexArrayID{ NULL };
    GLuint quadBufferID{ NULL };
    GLuint quadIndexBuffer{ NULL };
    GLuint strongholdScreenTex{ NULL };

    // dummy currently 
    void initSystems();

    // also -> if possible (should the stuff work some day), try to get clean up point

    // load shader functions

    bool getAnyGLFuncAddress(const char* name, void** ptrToFuncPtr);

    // pointers WGL functions
    PFNWGLCREATECONTEXTATTRIBSARBPROC ownPtr_wglCreateContextAttribsARB{ nullptr };
    PFNWGLCHOOSEPIXELFORMATARBPROC ownPtr_wglChoosePixelFormatARB{ nullptr };

    // pointers GL functions
    PFNGLDEBUGMESSAGECALLBACKPROC ownPtr_glDebugMessageCallback{ nullptr };

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

    PFNWGLSWAPINTERVALEXTPROC ownPtr_wglSwapIntervalEXT{ nullptr };

    bool loadGLFunctions();


    // debug functions

    // the proxy is kinda stupid
    static void APIENTRY debugMsgProxy(GLenum source, GLenum type, GLuint id, GLenum severity,
      GLsizei length, const GLchar* message, const void* userParam);

    void debugMsg(GLenum source, GLenum type, GLuint id, GLenum severity,
      GLsizei length, const GLchar* message, const void* userParam);
  };
}