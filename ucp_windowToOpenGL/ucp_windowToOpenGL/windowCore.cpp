/****************************************************************************************************/
//  Tutorial Source: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/
/****************************************************************************************************/

#include "pch.h"

#include "windowCore.h"

namespace UCPtoOpenGL
{
  WindowCore::WindowCore() {};
  WindowCore::~WindowCore() {};

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
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to create dummy window class.");
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
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to create dummy window.");
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
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to get dummy pixel format.");
      return false;
    }
    if (!SetPixelFormat(dummyDC, pixelFormat, &pfd))
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to set dummy pixel format.");
      return false;
    }

    HGLRC dummyContext = wglCreateContext(dummyDC);
    if (!dummyContext)
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to create dummy context.");
      return false;
    }

    if (!wglMakeCurrent(dummyDC, dummyContext))
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to make dummy context current.");
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

    if (!gotFuncs)
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to receive context creation functions.");
    }

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


  // debugging information

  void APIENTRY WindowCore::debugMsgProxy(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const void* userParam)
  {
    // TODO: c-style cast breaks const contract -> this is awful... and it stays for the moment
    ((WindowCore*)userParam)->debugMsg(source, type, id, severity, length, message, userParam);
  }

  void WindowCore::debugMsg(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const void* userParam)
  {
    // currently uses level set by logger, split throw the severities
    // downside is: every message currently roundtrips through Lua and is only discarded based on the logger level
    // since this logging is optional, and almost nothing is done in OpenGL (at the moment), I think this ok for now
    // better handling tutorial if required: https://learnopengl.com/In-Practice/Debugging

    // using severity
    LogLevel levelToUse{ LOG_NONE };
    switch (severity)
    {
      case GL_DEBUG_SEVERITY_NOTIFICATION:
        levelToUse = LOG_DEBUG;
        break;
      case GL_DEBUG_SEVERITY_LOW:
        levelToUse = LOG_INFO;
        break;
      case GL_DEBUG_SEVERITY_MEDIUM:
        levelToUse = LOG_WARNING;
        break;
      case GL_DEBUG_SEVERITY_HIGH:
        levelToUse = LOG_ERROR;
        break;
      default:
        Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Received OpenGL Debug Message without valid severity.");
        break;
    }

    if (levelToUse != LOG_NONE)
    {
      std::string strMessage = std::string(message);

      // remove whitespace
      // source: https://stackoverflow.com/a/217605
      
      // trim from start
      strMessage.erase(strMessage.begin(), std::find_if(strMessage.begin(), strMessage.end(),
          [](unsigned char ch)
          {
            return !std::isspace(ch);
          }
        ));

      // trim from end
      strMessage.erase(std::find_if(strMessage.rbegin(), strMessage.rend(),
          [](unsigned char ch)
          {
            return !std::isspace(ch);
          }
        ).base(), strMessage.end());

      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: " + strMessage);
    }
  }


  // window creation

  bool WindowCore::createWindow(HWND win)
  {
    if (!confPtr)
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Did not receive configuration.");
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
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to get pixel format.");
      return false;
    }

    if (!SetPixelFormat(deviceContext, format_index, &pfd))
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to set pixel format.");
      return false;
    }

    // double check, because originally set structure optional? -> maybe remove later:
    // -> this
    auto active_format_index{ GetPixelFormat(deviceContext) };
    if (!active_format_index)
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to get chosen pixel format.");
      return false;
    }

    if (!DescribePixelFormat(deviceContext, active_format_index, sizeof(pfd), &pfd))
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to describe pixel format.");
      return false;
    }

    if ((pfd.dwFlags & PFD_SUPPORT_OPENGL) != PFD_SUPPORT_OPENGL)
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Pixel buffer does not support OpenGL.");
      return false;
    }
    // until here -> but maybe need to validate that windows choose a fitting thing?

    // create attribute list
    const int attribList[] =
    {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4, // asking for OpenGL 4.0+ (can be changed based on requirements)
      WGL_CONTEXT_MINOR_VERSION_ARB, 0,
      WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,  // core profile

      WGL_CONTEXT_FLAGS_ARB, confPtr->graphic.debug == DEBUG_DEBUG_CONTEXT_ENABLED ? WGL_CONTEXT_DEBUG_BIT_ARB : NULL ,
      0, // End (needed to indicate end of list)
    };

    renderingContext = ownPtr_wglCreateContextAttribsARB(deviceContext, NULL, attribList);//wglCreateContext(deviceContext);

    if (!renderingContext || !wglMakeCurrent(deviceContext, renderingContext))
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to create and set main rendering context.");
      return false;
    }

    //std::string test{ reinterpret_cast<const char*>(glGetString(GL_VERSION)) };

    // enable debug (should already be if create with debug bit) and set return message
    if (confPtr->graphic.debug != DEBUG_OFF)
    {
      // check if context is debug context
      if (confPtr->graphic.debug == DEBUG_DEBUG_CONTEXT_ENABLED)
      {
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT))
        {
          Log(LOG_WARNING, "[graphicsApiReplacer]: [OpenGL]: Despite requested, no debug context was created.");
        }
      }

      if (getAnyGLFuncAddress("glDebugMessageCallback", (void**)&ownPtr_glDebugMessageCallback))
      {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // slower, but in theory able to do more stuff
        ownPtr_glDebugMessageCallback(debugMsgProxy, this);
      }
      else
      {
        glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDisable(GL_DEBUG_OUTPUT);
      }
    }

    // load functions -> currently no checks are made before
    // if the functions are not found in the context, the game window will simply close
    if (!loadGLFunctions())
    {
      Log(LOG_ERROR, "[graphicsApiReplacer]: [OpenGL]: Failed to obtain needed functions.");
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

  void WindowCore::setOnlyTexSize(Size<int> texSize)
  {
    strongTexSize = texSize;
  }

  void WindowCore::adjustTexSizeAndViewport(Size<int> texSize, Size<int> viewSize, Size<double> scale)
  {
    glViewport(0, 0, viewSize.w, viewSize.h);
    strongTexSize = texSize;

    float sW{ static_cast<float>(scale.w) };
    float sH{ static_cast<float>(scale.h) };
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize.w, texSize.h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr); // RGB565
        break;
      case ARGB_1555:
      default:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, texSize.w, texSize.h, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr); // ARGB1555
        break;
    }
  }


  HRESULT WindowCore::renderNextScreen(unsigned short* backData)
  {
    // update texture
    switch (pixFormat)
    {
      case RGB_565:
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, strongTexSize.w, strongTexSize.h, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, backData); // RGB565
        break;
      case ARGB_1555:
      default:
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, strongTexSize.w, strongTexSize.h, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, backData); // ARGB1555
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

    return S_OK;
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
    // -> mipmaps could be possible, but they would need to be created on every frame...
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    // currently only GL_LINEAR for a little bit filtering, only other without mipmaps would be raw GL_NEAREST
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