#pragma once

namespace UCPtoOpenGL
{
  // holds possible render resolutions
  enum GameResolution
  {
    RES_800_X_600 = 0x1,
    RES_1024_X_768 = 0x2,
    RES_1280_X_720 = 0x3,
    RES_1280_X_1024 = 0x4,
    RES_1366_X_768 = 0x5,
    RES_1440_X_900 = 0x6,
    RES_1600_X_900 = 0x7,
    RES_1600_X_1200 = 0x8,
    RES_1680_X_1050 = 0x9,
    RES_1920_X_1080 = 0xa,
    RES_1920_X_1200 = 0xb,
    RES_2560_X_1440 = 0xc,
    RES_2560_X_1600 = 0xd,
    RES_1360_X_768 = 0xe,
    RES_1024_X_600 = 0xf,
    RES_640_X_480 = 0x14,
    RES_NONE = 0xffffffff,
  };


  // hardcoded const resolutions of the game, index is the GameResolution enum
  inline constexpr std::array<int[2], 17> RESOLUTIONS{ {
    { 0     ,   0     },  // 0 has none
    { 800   ,   600   },
    { 1024  ,   768   },
    { 1280  ,   720   },
    { 1280  ,   1024  },
    { 1366  ,   768   },
    { 1440  ,   900   },
    { 1600  ,   900   },
    { 1600  ,   1200  },
    { 1680  ,   1050  },
    { 1920  ,   1080  },
    { 1920  ,   1200  },
    { 2560  ,   1440  },
    { 2560  ,   1600  },
    { 1360  ,   768   },
    { 1024  ,   600   },
    { 640   ,   480   },
  } };


  // based on Crusader 1.41 -> hopefully works at least for extreme 1.41.1-E
  struct SHCWindowOrMainStructFake
  {
    BOOL                  drawingReady;
    DWORD                 unknown_1;
    int                   screenWidthInPixels;
    int                   screenHeightInPixels;
    DWORD                 unknown_2[2];
    int                   gameResolutionX;
    int                   gameResolutionY;
    int                   mainMenuBorderWidth;
    int                   mainMenuBorderHeight;
    int                   gameOnScreenPosX;
    int                   gameOnScreenPosY;
    int                   gameInWindowPosX;
    int                   gameInWindowPosY;
    int                   gameResolutionX_2;
    int                   gameResolutionY_2;
    int                   gameResolutionXTimes2_2;
    int                   gameResolutionY_3;
    int                   numPixel_GameXTimes2_x_GameY;
    int                   numPixel_GameY_x_GameX_x_3;
    int                   colorDepth;
    PixelFormat           colorBitMode;
    BOOL                  runGameAsExclusiveFullscreen;
    GameResolution        currentGameResolution;
    DWORD                 unknown_3[2];
    BOOL                  resolutionSupported[16];  // like the main res 0x1 to 0xf, but with an unused 0x0 -> should stay 0?
    HINSTANCE             gameHInstance;
    HWND                  gameWindowHandle;
    RECT                  clientOnScreenCoords;
    DWORD                 unknown_4;
    DWORD                 isNotProcessingInputEvents;  // did not found this, do not knwo what it means
    BOOL                  gameFocused;
    DWORD                 unknown_5[2];
    unsigned short* surfacePtr_Game;
    unsigned short* surfacePtr_Map;
    DWORD                 unknown_6[4];
    IDirectDraw*          ddInterfacePtr;
    IDirectDrawSurface*   ddBackbufferSurfacePtr;
    IDirectDrawSurface*   ddPrimarySurfacePtr;
    BOOL                  NOT_selfBufferOrWindowMode;
    IDirectDrawSurface*   ddOffscreenSurfacePtr_Game;
    IDirectDrawSurface*   ddPrimarySurfacePtr_Map;
    DDSURFACEDESC         ddSurfDescForBink_Game;
    DDSURFACEDESC         ddSurfDescForBink_Map;
    DWORD                 unknown_7;
    DWORD                 windowCreationTime;
    DWORD                 unknown_8;  // maybe more?
  };
}