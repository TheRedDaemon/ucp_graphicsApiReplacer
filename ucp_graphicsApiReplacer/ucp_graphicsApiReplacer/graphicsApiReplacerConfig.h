#pragma once

namespace UCPGraphicsApiReplacer
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
    TYPE_BORDERLESS_FULLSCREEN,
    TYPE_FULLSCREEN              // NOT_IMPL, currently like borderless fullscreen -> what should it do? -> maybe like normal crusader, adapt screen size to set resolution
  };

  enum ContinueOutOfFocus
  {
    NOFOCUS_PAUSE,      // normal
    NOFOCUS_CONTINUE,   // the game continues to run, but it does not render (sound is active, though)
    NOFOCUS_RENDER      // the game continues to run and render
  };

  struct WindowConfig
  {
    WindowType type{ TYPE_WINDOW };
    int width{ 1280 };            // only relevant for window modes
    int height{ 720 };
    WindowPos pos{ POS_MIDDLE };  // only used for non fullscreen

    ContinueOutOfFocus continueOutOfFocus{ NOFOCUS_PAUSE }; // the game and the rendering continue
    bool minimizeOnLostFocus{ false };
  };


  /** graphics **/

  enum GraphicsApi
  {
    GRAPHICS_API_DIRECT_X,
    GRAPHICS_API_OPEN_GL,
  };

  enum PixelFormat
  {
    ARGB_1555 = 0x555,
    RGB_565   = 0x565,
  };

  enum DebugOption
  {
    DEBUG_OFF,
    DEBUG_ENABLED,
    DEBUG_DEBUG_CONTEXT_ENABLED
  };

  struct GraphicConfig
  {
    GraphicsApi graphicsApi{ GRAPHICS_API_DIRECT_X };

    bool filterLinear{ true };  // if the texture filter should be linear, otherwise nearest
    bool vsync{ true }; // there might be issue with the win10 screen composition...
    bool waitWithGLFinish{ false }; // calls glFinish after swap -> also seems to prevent tearing, do not know what is better...
    PixelFormat pixFormat{ ARGB_1555 };

    DebugOption debug{ DEBUG_OFF };
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

  struct GraphicsAPIReplacerConfig
  {
    WindowConfig window;
    GraphicConfig graphic;
    ControlConfig control;
  };
}