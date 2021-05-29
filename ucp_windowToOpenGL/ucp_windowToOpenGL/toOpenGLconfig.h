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
    TYPE_FULLSCREEN,
    TYPE_BORDERLESS_FULLSCREEN
  };

  struct WindowConfig
  {
    WindowType type{ TYPE_WINDOW };
    int width{ 1280 };
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
    bool vsync{ true };  // win10 and vsync are no friends -> instead of screen tearing, it becomes relativly slow and forces fullscreen
    bool waitWithGLFinish{ false }; // calls glFinish after swap -> also seems to prevent tearing, do not know what is better...
    PixelFormat pixFormat{ ARGB_1555 };
  };


  /** scroll/control **/

  struct ControlConfig
  {
    bool clipCursor{ false }; // cursor is bound to the game window (problems with title bar?)
    
    bool scrollActive{ true };
    int padding{ 0 }; // window pixels to extend the scroll zone inward
    int margin{ 0 }; // window pixels to extend the scroll zone outward (for window)
  };


  /** complete **/

  struct ToOpenGLConfig
  {
    WindowConfig window;
    GraphicConfig graphic;
    ControlConfig control;
  };
}