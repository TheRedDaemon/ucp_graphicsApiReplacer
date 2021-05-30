#pragma once

namespace UCPtoOpenGL
{
  // idea? -> config struct is just pointer received from lua?

  // using non class enums? -> maybe easier conversion? If no issues, can become enum class later


  /** window **/

  enum WindowPos
  {
    POS_MIDDLE,
    POS_TOP_LEFT,
    POS_BOTTOM_LEFT,
    POS_TOP_RIGHT,
    POS_BOTTOM_RIGHT
  };

  enum WindowType
  {
    TYPE_WINDOW,
    TYPE_BORDERLESS_WINDOW,
    TYPE_FULLSCREEN,              // NOT_IMPL, currently like borderless fullscreen -> what should it do? -> maybe like normal crusader, adapt screen size to set resolution
    TYPE_BORDERLESS_FULLSCREEN
  };

  struct WindowConfig
  {
    WindowType type{ TYPE_WINDOW };
    int width{ 1280 };            // only relevant for window modes
    int height{ 720 };
    WindowPos pos{ POS_MIDDLE };  // only used for non fullscreen
  };


  /** graphics **/

  enum PixelFormat
  {
    ARGB_1555,
    RGB_565
  };

  struct GraphicConfig
  {
    bool filterLinear{ true };  // if the texture filter should be linear, otherwise nearest
    bool vsync{ true }; // there might be issue with the win10 screen composition...
    bool waitWithGLFinish{ false }; // calls glFinish after swap -> also seems to prevent tearing, do not know what is better...
    PixelFormat pixFormat{ ARGB_1555 };
  };


  /** scroll/control **/

  struct ControlConfig
  {
    bool clipCursor{ true }; // cursor is bound to the game screen (problems with title bar?)
    
    bool scrollActive{ true };
    int margin{ 0 }; // window pixels to extend the scroll zone outward (for window)
    int padding{ 0 }; // window pixels to extend the scroll zone inward
  };


  /** complete **/

  struct ToOpenGLConfig
  {
    WindowConfig window;
    GraphicConfig graphic;
    ControlConfig control;
  };
}