# UCP Graphics API Replacer

This repository contains a module for the "Unofficial Crusader Patch Version 3" (UCP3), a modification for Stronghold Crusader.
The module allows to switch the displaying system to a more modern that uses either DirectX or OpenGL.

### Motivation and Plan

Stronghold Crusader uses its own software renderer to create frames.
To actually display them, though, an OS compatible graphics API is required.
Crusader uses [DirectDraw](https://en.wikipedia.org/wiki/DirectDraw).
The API is old, most hooks by overlays and recording software do not work and it imposes certain restrictions on other parts, like the window.
Crusader Version 1.41 additionally only works in a fullscreen mode, which makes task switching bothersome.

The module intends to provide a custom made solution explicitly for Crusader that provides better compatibility and user experience.
This is archived by modifying and taking control of the window creation, the few used DirectDraw calls and certain input functions.
At its core, the module pretends to be DirectDraw and provides the memory for Crusader to draw its frames in.
The result is then treated as a texture and displayed using either [DirectX 11](https://en.wikipedia.org/wiki/DirectX) or [OpenGL](https://www.opengl.org/).
Especially mouse inputs are modified to fit the changed display.

### Usage

The module is part of the UCP3. Certain commits of the main branch of this repository are included as a git submodule.
It is therefore not needed to download additional content.

However, should issues or suggestions arise that are related to this module, feel free to add a new GitHub issue.
Support is currently only guaranteed for the western versions of Crusader 1.41 and Crusader Extreme 1.41.1-E.
Other, eastern versions of 1.41 might work. Versions prior to the HD versions (1.3?) will definitely NOT work.

The module has the [winProcHandler](https://github.com/TheRedDaemon/ucp_winProcHandler) as dependency and it registers its winProc function with an early **-100000** priority (smaller mean earlier).

### Options

The module provides multiple options. Some things are general:
* The in-game resolution will only set the render resolution. The window resolution needs to be set through this options.
* The game resolution ratio is preserved. Empty spaces in the window will stay black. Note, that these parts are still part of the rendered screen, so they will appear in recordings.
* If one of the three main mouse keys is held, the cursor is clipped to the game part of the window.
* Option changes currently require a restart.

The following list will go through every option and explain it brief. The structure is taken from the configuration.

* **window** - General options related to the window.

  * ***type*** - The type of window to create. Default: *window*
    * *window* - Creates a draggable window with a title bar.
    * *borderlessWindow* - Creates a fixed window without a border.
    * *borderlessFullscreen* - Creates a borderless window taking the whole screen. Ignores size and position settings. Minimizes on task switch.

  * ***width*** - Width of the created window. Default: *1280*
    * *Integer between 0 and 20000*

  * ***height*** - Height of the created window. Default: *720*
    * *Integer between 0 and 20000*

  * ***pos*** - Position of the created window. Default: *middle*
    * *middle*
    * *topLeft*
    * *bottomLeft*
    * *topRight*
    * *bottomRight*

  * ***continueOutOfFocus*** - Sets how the game handles losing the focus. Default: *pause*
    * *pause* - Normal game behavior. The game pauses by freezing.
    * *continue* - The game continues and sounds play, but the window freezes.
    * *render* - The game will run as if it has the focus.

  * ***minimizeOnLostFocus*** - Sets if the game window should minimize when losing the focus. Default: *false*
    * *true* or *false*


* **graphic** - Options for DirectX and OpenGL.

  * ***api*** - The graphics api to use. Default: *DirectX*
    * *DirectX* - Recommended. Uses DirectX 11. Turned out far more compatible with overlays and recordings.
    * *OpenGL* - Deprecated. Uses OpenGL. Potentially easier to get working, but issues with recordings and overlays.

  * ***filterLinear*** - A linear texture filter is applied. Might improve picture if window and render resolution is not equal. Default: *true*
    * *true* or *false*

  * ***vsync*** - Activates VSync. Reduces number of rendered frames to display frequency. Default: *true*
    * *true* or *false*
    * Note:
      * Map scrolling is currently not independent from the frame rate.
        As a kind of hack, setting VSync will therefore improve the experience by slowing down the scrolling on low resolutions,
        since less frames are rendered.

  * ***waitWithGLFinish*** - Calls glFinish after screen buffer swap if OpenGL is used. Might have no effect whatsoever. Possible subject of removal. No effect on DirectX. Default: *false*
    * *true* or *false*

  * ***pixFormat*** - Pixel color format. Transformed to 32 bit RGBA anyway, but sets the software renderer option. Default: *argb1555*
    * *argb1555* - Red 5 bit, Green 5 bit, Blue 5 bit. Likely the native texture format.
    * *rgb565* - Red 5 bit, Green 6 bit, Blue 5 bit.

  * ***debug*** - Activates debug messages. Should be off for normal usage. At least in case of DirectX it requires Debug libraries. Default: *none*
    * *none* - No messages.
    * *enabled*:
      * DirectX: Activate debug messages. Requires installed debug libraries.
      * OpenGL: Activates debug messages without debug context. Might produce only a few or no messages.
    * *debugContextEnabled*:
      * DirectX: Activate debug messages. Same as *enabled*. No additional effect.
      * OpenGL: Creates a debug context that provides all messages.


* **control** - Options for the input control.

  * ***clipCursor*** - Clips the cursor to the game part of the window in general. Default: *true*
    * *true* or *false*

  * ***scrollActive*** - Border scrolling is active. Default: *true*
    * *true* or *false*

  * ***margin*** - Window pixels to extend the scroll zone outward (only no clipper, window modes). Default: *0*
    * *Integer between 0 and 1000*

  * ***padding*** - Window pixels to extend the scroll zone inward. Default: *0*
    * *Integer between 0 and 1000*

### Special Thanks

To all of the UCP Team, the [Ghidra project](https://github.com/NationalSecurityAgency/ghidra) and
of course to [Firefly Studios](https://fireflyworlds.com/), the creators of Stronghold Crusader.