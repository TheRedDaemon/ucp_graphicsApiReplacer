
#include "pch.h"

#include "configUtil.h"

namespace UCPtoOpenGL
{
  // lot of config stuff from here: https://stackoverflow.com/a/1410917

  DWORD GetWindowStyle(WindowType type)
  {
    DWORD style{ WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN };  // returned window needs to be visible, other stuff because of tutorial
    switch(type)
    {
      case TYPE_FULLSCREEN:
      case TYPE_BORDERLESS_FULLSCREEN:
      case TYPE_BORDERLESS_WINDOW:
        return style | WS_POPUP;
      case TYPE_WINDOW: // is default
      default:
        return style | WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
  }

  DWORD GetExtendedWindowStyle(WindowType type)
  {
    DWORD style{ WS_EX_APPWINDOW };
    switch (type)
    {
      case TYPE_FULLSCREEN:
      case TYPE_BORDERLESS_FULLSCREEN:
      case TYPE_BORDERLESS_WINDOW:
        return style;
      case TYPE_WINDOW: // is default
      default:
        return style | WS_EX_WINDOWEDGE;
    }
  }

  RECT GetWindowRect(WindowConfig& winConf)
  {
    int screenWidth{ GetSystemMetrics(SM_CXSCREEN) };
    int screenHeight{ GetSystemMetrics(SM_CYSCREEN) };

    RECT newWinRect{ 0, 0, screenWidth, screenHeight };

    if (winConf.type == TYPE_FULLSCREEN || winConf.type == TYPE_BORDERLESS_FULLSCREEN)
    {
      return newWinRect; // screen size
    }

    switch (winConf.pos)
    {
      case POS_TOP_LEFT:
        newWinRect = { 0, 0, winConf.width, winConf.height };
        break;
      case POS_BOTTOM_LEFT:
        newWinRect = { 0, screenHeight - winConf.height, winConf.width, screenHeight };
        break;
      case POS_TOP_RIGHT:
        newWinRect = { screenWidth - winConf.width, 0, screenWidth, winConf.height };
        break;
      case POS_BOTTOM_RIGHT:
        newWinRect = { screenWidth - winConf.width, screenHeight - winConf.height, screenWidth, screenHeight };
        break;
      case POS_MIDDLE:
      default:
      {
        int xPos{ (screenWidth - winConf.width) / 2 };
        int yPos{ (screenHeight - winConf.height) / 2 };
        newWinRect = { xPos, yPos, xPos + winConf.width, yPos + winConf.height };
        break;
      }
    }

    // I assume borderless has no decor
    if (winConf.type == TYPE_BORDERLESS_WINDOW)
    {
      return newWinRect;
    }

    // only for TYPE_WINDOW
    // screenshots still do not work -> maybe they take the OpenGL surface, but the title bar edge
    AdjustWindowRectEx(&newWinRect, GetWindowStyle(winConf.type), false, GetExtendedWindowStyle(winConf.type));

    if (newWinRect.right > screenWidth)
    {
      int move{ newWinRect.right - screenWidth };
      newWinRect.right -= move;
      newWinRect.left -= move;
    }
    else if (newWinRect.left < 0)
    {
      int move{ newWinRect.left };
      newWinRect.right -= move;
      newWinRect.left -= move;
    }

    if (newWinRect.bottom > screenHeight)
    {
      int move{ newWinRect.bottom - screenHeight };
      newWinRect.bottom -= move;
      newWinRect.top -= move;
    }
    else if (newWinRect.top < 0)
    {
      int move{ newWinRect.top };
      newWinRect.bottom -= move;
      newWinRect.top -= move;
    }

    return newWinRect;
  }

  int GetGameWidth(WindowConfig& winConf)
  {
    return winConf.type == TYPE_WINDOW || winConf.type == TYPE_BORDERLESS_WINDOW ? winConf.width : GetSystemMetrics(SM_CXSCREEN);
  }

  int GetGameHeight(WindowConfig& winConf)
  {
    return winConf.type == TYPE_WINDOW || winConf.type == TYPE_BORDERLESS_WINDOW ? winConf.height : GetSystemMetrics(SM_CYSCREEN);
  }
}