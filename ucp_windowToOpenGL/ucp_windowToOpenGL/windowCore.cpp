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
      getAnyGLFuncAddress("glUseProgram", (void**)&ownPtr_glUseProgram);
  }



  bool WindowCore::createWindow(HWND win)
  {
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


    renderingContext = wglCreateContext(deviceContext);

    if (!renderingContext || !wglMakeCurrent(deviceContext, renderingContext))
    {
      return false;
    }

    std::string test{ reinterpret_cast<const char*>(glGetString(GL_VERSION)) };

    // load functions -> currently no checks are made before
    // if the functions are not found in the context, the game window will simply close
    if (!loadGLFunctions())
    {
      return false;
    }

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
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr); // RGB565
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, wTex, hTex, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr); // ARGB1555 (?)
  }


  HRESULT WindowCore::renderNextScreen(unsigned short* backData)
  {
    // update texture
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, strongTexW, strongTexH, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, backData); // RGB565
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, strongTexW, strongTexH, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, backData); // ARGB1555 (?)

    // Clear the screen.
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the triangle (size should be number of indicies)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    SwapBuffers(deviceContext);

    return DD_OK;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


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