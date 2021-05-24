/****************************************************************************************************/
//  Tutorial Source: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/
/****************************************************************************************************/

#include "pch.h"

#include "windowCore.h"

// test
#include <string>

namespace UCPtoOpenGL
{
  bool WindowCore::createWindow(HWND win)
  {
    // INFO: I guess there is a lot of trust going on here, no safety, no additional driver checks, etc. etc...

    winHandle = win;

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

    // TODO: if possible, replace with fixed calls: https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows_2
    // if glfw is not needed anymore, lets just test it with glew to get at least the functions
    glewExperimental = true; // Needed in core profile
    if (glewInit() != GLEW_OK)
    {
      return false;
    }

    initSystems();
    return true;
  }


  void WindowCore::setTexStrongSize(int w, int h)
  {
    strongTexW = w;
    strongTexH = h;

    // dummy call
    setNewWindowStyle();
    
    // create initial texture, internal format either full GL_RGB, or GL_RGB5_A1 (should choose a format closer to real)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1, w, h, 0, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, nullptr);
  }


  void WindowCore::setNewWindowStyle()
  {
    // currently dummy stuff
    glViewport(0, 0, strongTexW, strongTexH);


    // this would set a new style and adjust the window
    // however, screenshots still do not work
    DWORD newStyle{ WS_OVERLAPPEDWINDOW | WS_VISIBLE };
    RECT newWinRect;
    GetWindowRect(winHandle, &newWinRect);
    AdjustWindowRectEx(&newWinRect, newStyle, false, NULL);

    SetWindowLongPtr(winHandle, GWL_STYLE, newStyle);
    MoveWindow(winHandle, newWinRect.left, newWinRect.top, newWinRect.right - newWinRect.left, newWinRect.bottom - newWinRect.top, true);
  }

  HRESULT WindowCore::renderNextScreen(unsigned short* backData)
  {
    // update texture (issue here?)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, strongTexW, strongTexH, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, backData);

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
    glClearColor(0.0, 1.0, 0.0, 1.0);

    /*======== geometry is simple quad ===========*/
    
    GLfloat vertexInfos[]{
      
      // pos        // texcoord
      -1.0, -1.0,   0.0, 1.0,
      1.0, -1.0,    1.0, 1.0,
      -1.0, 1.0,    0.0, 0.0,
      1.0, 1.0,     1.0, 0.0
    };

    GLuint indices[]{
      0, 1, 2, 1, 3, 2
    };

    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);


    glGenBuffers(1, &quadBufferID); // create empty buffer
    glBindBuffer(GL_ARRAY_BUFFER, quadBufferID);  // bind buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexInfos), vertexInfos, GL_STATIC_DRAW); // vertex data to buffer
    
    // vertex position (might be very often modified in the future)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0); // point to vertex buffer
    glEnableVertexAttribArray(0); // enable

    // texture coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))); // point to vertex buffer
    glEnableVertexAttribArray(1); // enable

    // indicies
    glGenBuffers(1, &quadIndexBuffer); // create empty buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndexBuffer); // bind buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // vertex data to the buffer


    // create texture, currently static
    glGenTextures(1, &strongholdScreenTex);
    glBindTexture(GL_TEXTURE_2D, strongholdScreenTex);

    // set so that OPenGL does not expect midmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


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
    GLuint vertShader{ glCreateShader(GL_VERTEX_SHADER) };
    glShaderSource(vertShader, 1, &vertShaderString, nullptr);
    glCompileShader(vertShader);


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
    GLuint fragShader{ glCreateShader(GL_FRAGMENT_SHADER) };
    glShaderSource(fragShader, 1, &fragShadeString, nullptr);
    glCompileShader(fragShader);

    
    // Create a shader program object to store
    
    GLuint program = glCreateProgram(); // Create a shader program
    glAttachShader(program, vertShader); // Attach a vertex shader
    glAttachShader(program, fragShader); // Attach a fragment shader
    glLinkProgram(program);  // Link both the programs


    // remove not needed programs
    glDetachShader(program, vertShader);
    glDetachShader(program, fragShader);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    // bind shader values
    glBindAttribLocation(program, 0, "vertex_position");
    glBindAttribLocation(program, 1, "inTexCoord");
    glBindFragDataLocation(program, 0, "finalColor");
    //glUniform1i(glGetUniformLocation(program, "strongTexture"), 0); // bind to sampler

    // use only program
    glUseProgram(program);
  }
}