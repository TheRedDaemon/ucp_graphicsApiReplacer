/****************************************************************************************************/
//  Tutorial Source: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/
/****************************************************************************************************/

#include "pch.h"

#include "windowCore.h"

// test
#include <string>

namespace UCPtoOpenGL
{
  // used to get function pointers, mostly based on:
  // source: https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
  bool WindowCore::getAnyGLFuncAddress(const char* name, void** ptrToFuncPtr)
  {
    void*& p{ *ptrToFuncPtr };
    p = (void*)wglGetProcAddress(name);
    if (p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1))
    {
      static HMODULE module = LoadLibraryA("opengl32.dll");
      if (module != NULL)
      {
        p = (void*)GetProcAddress(module, name);
      }
    }

    return p != nullptr;
  }

  // code largely unchanged
  // returns false if something goes wrong along the way
  // source: https://gist.github.com/nickrolfe/1127313ed1dbf80254b614a721b3ee9c
  bool WindowCore::loadWGLFunctions(HINSTANCE hInstance)
  {
    // Before we can load extensions, we need a dummy OpenGL context, created using a dummy window
    WNDCLASSA windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = DefWindowProcA;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = "Dummy_WGL_Window";

    if (!RegisterClassA(&windowClass))
    {
      return false;
    }

    HWND dummyWindow = CreateWindowExA(
      0,
      windowClass.lpszClassName,
      "Dummy OpenGL Window",
      0,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      0,
      0,
      windowClass.hInstance,
      0
    );

    if (!dummyWindow)
    {
      return false;
    }

    HDC dummyDC = GetDC(dummyWindow);

    PIXELFORMATDESCRIPTOR pfd{ sizeof(pfd), 1 };
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    int pixelFormat = ChoosePixelFormat(dummyDC, &pfd);
    if (!pixelFormat)
    {
      return false;
    }
    if (!SetPixelFormat(dummyDC, pixelFormat, &pfd))
    {
      return false;
    }

    HGLRC dummyContext = wglCreateContext(dummyDC);
    if (!dummyContext)
    {
      return false;
    }

    if (!wglMakeCurrent(dummyDC, dummyContext))
    {
      return false;
    }

    // get actual functions
    bool gotFuncs{ true };
    gotFuncs = gotFuncs && getAnyGLFuncAddress("wglCreateContextAttribsARB", (void**)&ownPtr_wglCreateContextAttribsARB);
    gotFuncs = gotFuncs && getAnyGLFuncAddress("wglChoosePixelFormatARB", (void**)&ownPtr_wglChoosePixelFormatARB);

    wglMakeCurrent(dummyDC, 0);
    wglDeleteContext(dummyContext);
    ReleaseDC(dummyWindow, dummyDC);
    DestroyWindow(dummyWindow);

    return gotFuncs;
  }


  bool WindowCore::loadGLFunctions()
  {
    return
      getAnyGLFuncAddress("glGenVertexArrays", (void**)&ownPtr_glGenVertexArrays) &&
      getAnyGLFuncAddress("glBindVertexArray", (void**)&ownPtr_glBindVertexArray) &&

      getAnyGLFuncAddress("glGenBuffers", (void**)&ownPtr_glGenBuffers) &&
      getAnyGLFuncAddress("glBindBuffer", (void**)&ownPtr_glBindBuffer) &&
      getAnyGLFuncAddress("glBufferData", (void**)&ownPtr_glBufferData) &&
      getAnyGLFuncAddress("glBufferSubData", (void**)&ownPtr_glBufferSubData) &&

      getAnyGLFuncAddress("glVertexAttribPointer", (void**)&ownPtr_glVertexAttribPointer) &&
      getAnyGLFuncAddress("glEnableVertexAttribArray", (void**)&ownPtr_glEnableVertexAttribArray) &&

      getAnyGLFuncAddress("glCreateShader", (void**)&ownPtr_glCreateShader) &&
      getAnyGLFuncAddress("glShaderSource", (void**)&ownPtr_glShaderSource) &&
      getAnyGLFuncAddress("glCompileShader", (void**)&ownPtr_glCompileShader) &&
      getAnyGLFuncAddress("glAttachShader", (void**)&ownPtr_glAttachShader) &&
      getAnyGLFuncAddress("glDetachShader", (void**)&ownPtr_glDetachShader) &&
      getAnyGLFuncAddress("glDeleteShader", (void**)&ownPtr_glDeleteShader) &&

      getAnyGLFuncAddress("glBindAttribLocation", (void**)&ownPtr_glBindAttribLocation) &&
      getAnyGLFuncAddress("glBindFragDataLocation", (void**)&ownPtr_glBindFragDataLocation) &&

      getAnyGLFuncAddress("glCreateProgram", (void**)&ownPtr_glCreateProgram) &&
      getAnyGLFuncAddress("glLinkProgram", (void**)&ownPtr_glLinkProgram) &&
      getAnyGLFuncAddress("glUseProgram", (void**)&ownPtr_glUseProgram) &&

      getAnyGLFuncAddress("wglSwapIntervalEXT", (void**)&ownPtr_wglSwapIntervalEXT);
  }



  bool WindowCore::createWindow(HWND win)
  {
    if (!confPtr)
    {
      return false;
    }

    // INFO: I guess there is a lot of trust going on here, no safety, no additional driver checks, etc. etc...

    // wgl Context creation after this source: https://stackoverflow.com/a/6316595

    deviceContext = GetDC(win); // this one will not get closed for a while
    
    PIXELFORMATDESCRIPTOR pfd{ sizeof(pfd), 1 };
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    int format_index{ ChoosePixelFormat(deviceContext, &pfd) };
    if (!format_index)
    {
      return false;
    }

    if (!SetPixelFormat(deviceContext, format_index, &pfd))
    {
      return false;
    }

    // double check, because originally set structure optional? -> maybe remove later:
    // -> this
    auto active_format_index{ GetPixelFormat(deviceContext) };
    if (!active_format_index)
    {
      return false;
    }

    if (!DescribePixelFormat(deviceContext, active_format_index, sizeof(pfd), &pfd))
    {
      return false;
    }

    if ((pfd.dwFlags & PFD_SUPPORT_OPENGL) != PFD_SUPPORT_OPENGL)
    {
      return false;
    }
    // until here -> but maybe needd to validate that windows choose a fitting thing?

    // TODO:
    // after a certain point, the context creation is not cleaned up properly!

    renderingContext = wglCreateContext(deviceContext);

    if (!renderingContext || !wglMakeCurrent(deviceContext, renderingContext))
    {
      return false;
    }

    //std::string test{ reinterpret_cast<const char*>(glGetString(GL_VERSION)) };

    // load functions -> currently no checks are made before
    // if the functions are not found in the context, the game window will simply close
    if (!loadGLFunctions())
    {
      return false;
    }

    // set vsync
    if (confPtr->graphic.vsync)
    {
      ownPtr_wglSwapIntervalEXT(1);
    }

    // set pix format
    pixFormat = confPtr->graphic.pixFormat;

    initSystems();

    return true;
  }

  void WindowCore::setOnlyTexSize(int wTex, int hTex)
  {
    strongTexW = wTex;
    strongTexH = hTex;
  }

  void WindowCore::adjustTexSizeAndViewport(int wTex, int hTex, int wView, int hView, double scaleW, double scaleH)
  {
    glViewport(0, 0, wView, hView);
    strongTexW = wTex;
    strongTexH = hTex;

    float sW{ static_cast<float>(scaleW) };
    float sH{ static_cast<float>(scaleH) };
    GLfloat newPos[]{
      -1.0f * sW, -1.0f * sH,
      1.0f * sW, -1.0f * sH,
      -1.0f * sW, 1.0f * sH,
      1.0f * sW, 1.0f * sH
    };

    ownPtr_glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(float), newPos);

    // create initial texture
    // Stronghold supports multiple pixel formats -> what the game would "prefer" still needs more research
    switch (pixFormat)
    {
      case RGB_565:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wTex, hTex, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr); // RGB565
        break;
      case ARGB_1555:
      default:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, wTex, hTex, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr); // ARGB1555
        break;
    }
  }


  HRESULT WindowCore::renderNextScreen(unsigned short* backData)
  {
    // update texture
    switch (pixFormat)
    {
      case RGB_565:
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, strongTexW, strongTexH, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, backData); // RGB565
        break;
      case ARGB_1555:
      default:
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, strongTexW, strongTexH, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, backData); // ARGB1555
        break;
    }

    // Clear the screen.
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the triangle (size should be number of indicies)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    SwapBuffers(deviceContext);
    if (confPtr->graphic.waitWithGLFinish)
    {
      glFinish();
    }

    return DD_OK;
  }

  void WindowCore::releaseContext(HWND hwnd)
  {
    wglMakeCurrent(NULL, NULL); // no context
    ReleaseDC(hwnd, deviceContext);
    wglDeleteContext(renderingContext);
    // window should get destroyed by Crusader
  }


  void WindowCore::initSystems()
  {

    // the clear color
    glClearColor(0.0, 0.0, 0.0, 0.0);

    /*======== geometry is simple quad ===========*/
    
    GLfloat vertexInfos[]{
      
      // pos
      -1.0f, -1.0f,
      1.0f, -1.0f,
      -1.0f, 1.0f,
      1.0f, 1.0f,

      // texcoord
      0.0f, 1.0f,
      1.0f, 1.0f,
      0.0f, 0.0f,
      1.0f, 0.0f
    };

    GLuint indices[]{
      0, 1, 2, 1, 3, 2
    };

    ownPtr_glGenVertexArrays(1, &vertexArrayID);
    ownPtr_glBindVertexArray(vertexArrayID);


    ownPtr_glGenBuffers(1, &quadBufferID); // create empty buffer
    ownPtr_glBindBuffer(GL_ARRAY_BUFFER, quadBufferID);  // bind buffer
    ownPtr_glBufferData(GL_ARRAY_BUFFER, sizeof(vertexInfos), vertexInfos, GL_STATIC_DRAW); // vertex data to buffer

    // vertex position (is modified)
    ownPtr_glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0); // point to vertex buffer
    ownPtr_glEnableVertexAttribArray(0); // enable

    // texture coords
    ownPtr_glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(8 * sizeof(float))); // point to vertex buffer
    ownPtr_glEnableVertexAttribArray(1); // enable

    // indicies
    ownPtr_glGenBuffers(1, &quadIndexBuffer); // create empty buffer
    ownPtr_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndexBuffer); // bind buffer
    ownPtr_glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // vertex data to the buffer


    // create texture, currently static
    glGenTextures(1, &strongholdScreenTex);
    glBindTexture(GL_TEXTURE_2D, strongholdScreenTex);

    // set so that OpenGL does not expect midmaps
    // TODO: check more options
    // -> mipmaps could be possible, but the would need to be created on every frame...
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    // currently only GL_LINEAR for a little bit filtering, only other without mipmaps would be raw GL_NEAREST
    // TODO?: make filtermode changeable
    GLint filterMode{confPtr->graphic.filterLinear ? GL_LINEAR : GL_NEAREST};
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);


    /*================ Shaders ====================*/

    // Shader will get hardcoded into the code:

    // vertex shader

    const char* vertShaderString = R"(
      #version 330 core

      in vec3 vertex_position;
      in vec2 inTexCoord;

      out vec2 TexCoord;

      void main()
      {
        gl_Position = vec4(vertex_position, 1);
        TexCoord = inTexCoord;
      }
    )";

    // create
    GLuint vertShader{ ownPtr_glCreateShader(GL_VERTEX_SHADER) };
    ownPtr_glShaderSource(vertShader, 1, &vertShaderString, nullptr);
    ownPtr_glCompileShader(vertShader);


    // fragment-shader

    const char* fragShadeString = R"(
      #version 330 core

      out vec4 finalColor;

      in vec2 TexCoord;

      uniform sampler2D strongTexture;

      void main()
      {
        finalColor = texture(strongTexture, TexCoord);
        
        //// test stuff:
        //if (finalColor.x < 0.1 && finalColor.y < 0.1 && finalColor.z < 0.1)
        //{
        //  finalColor = vec4(1.0, 1.0, 1.0, 1.0);
        //}
      }
    )";
    
    // create
    GLuint fragShader{ ownPtr_glCreateShader(GL_FRAGMENT_SHADER) };
    ownPtr_glShaderSource(fragShader, 1, &fragShadeString, nullptr);
    ownPtr_glCompileShader(fragShader);

    
    // Create a shader program object to store
    
    GLuint program = ownPtr_glCreateProgram(); // Create a shader program
    ownPtr_glAttachShader(program, vertShader); // Attach a vertex shader
    ownPtr_glAttachShader(program, fragShader); // Attach a fragment shader
    ownPtr_glLinkProgram(program);  // Link both the programs


    // remove not needed programs
    ownPtr_glDetachShader(program, vertShader);
    ownPtr_glDetachShader(program, fragShader);
    ownPtr_glDeleteShader(vertShader);
    ownPtr_glDeleteShader(fragShader);
    
    // bind shader values
    ownPtr_glBindAttribLocation(program, 0, "vertex_position");
    ownPtr_glBindAttribLocation(program, 1, "inTexCoord");
    ownPtr_glBindFragDataLocation(program, 0, "finalColor");
    //glUniform1i(glGetUniformLocation(program, "strongTexture"), 0); // bind to sampler

    // use only program
    ownPtr_glUseProgram(program);
  }
}