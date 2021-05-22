/****************************************************************************************************/
//  Tutorial Source: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-1-opening-a-window/
/****************************************************************************************************/

#include "pch.h"

#include "windowCore.h"

namespace UCPtoOpenGL
{
  bool WindowCore::createWindow()
  {
    // Initialise GLFW
    glewExperimental = true; // Needed for core profile
    if (!glfwInit())
    {
      return false;
    }

    // OPenGl 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    // This will get other values later
    window = glfwCreateWindow(1024, 768, "Tutorial 01", NULL, NULL);
    if (window == NULL)
    {
      return false;
    }

    glfwMakeContextCurrent(window); // Initialize GLEW
    glewExperimental = true; // Needed in core profile // @TheRedDaemon: Tutorial sets it twice? I keep it this way.
    if (glewInit() != GLEW_OK)
    {
      return false;
    }

    initSystems();
    return true;
  }


  HWND WindowCore::getWindowHandle()
  {
    return glfwGetWin32Window(window);
  }


  void WindowCore::setTexStrongSize(int w, int h)
  {
    strongTexW = w;
    strongTexH = h;
    
    // dummy

    // create tex
    unsigned short testData[]{ 0xFFFF, 0x5555, 0xFFFF, 0x5555, 0xFFFF, 0x5555, 0xFFFF, 0x5555, 0xFFFF };
    unsigned short* t = testData;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 3, 3, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, (void*)t);
    GLenum test = glGetError();
  }


  HRESULT WindowCore::renderNextScreen(unsigned short* backData)
  {
    // update texture
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, strongTexW, strongTexH, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, backData);
    glActiveTexture(0);
    // Clear the screen.
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the triangle (size should be number of indicies)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Swap buffers
    glfwSwapBuffers(window);
    
    // glfwPollEvents(); // The tutorial is also written for input. Lets just hope Stronghold takes care of this.
    return DD_OK;
  }


  void WindowCore::initSystems()
  {
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
    //glActiveTexture(0);
    glGenTextures(1, &strongholdScreenTex);
    glBindTexture(GL_TEXTURE_2D, strongholdScreenTex);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
        vec4 col = texture(strongTexture, TexCoord);
        if (col.x == 0.0 && col.z == 0.0){
          finalColor = vec4(TexCoord.x, TexCoord.y, TexCoord.x, TexCoord.y);
        }
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