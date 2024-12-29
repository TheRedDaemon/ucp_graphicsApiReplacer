# Graphics API Replacer

**Authors**: TheRedDaemon, gynt  
**Version**: 1.3.0  
**Repository**: [https://github.com/TheRedDaemon/ucp_graphicsApiReplacer](https://github.com/TheRedDaemon/ucp_graphicsApiReplacer)

This module allows to switch the old DirectDraw API displaying system to a more modern that uses either DirectX or OpenGL. This is archived by modifying and taking control of the window creation, the few used DirectDraw calls and certain input functions. At its core, the module pretends to be DirectDraw and provides the memory for Crusader to draw its software rendered frames in. The result is then treated as a texture and displayed using either DirectX 11 or OpenGL. Mouse inputs are modified to fit the changed display.

Support is currently only guaranteed for the western versions of Crusader 1.41 and Crusader Extreme 1.41.1-E. Other, eastern versions of 1.41 might work. Versions prior to the HD versions (1.3?) will definitely **NOT** work.

### Options

The module provides multiple options which are explained are explained in detail in the configuration tab. Some general things apply, though:

* The in-game resolution will only set the render resolution. The window resolution needs to be set through the provided options.
* The game resolution ratio is preserved. Empty spaces in the window will stay black. Note, that these parts are still part of the rendered screen, so they will appear in recordings.
* If one of the three main mouse keys is held, the cursor is clipped to the game part of the window.
* Options can not be changed without a game restart.

### Feedback

Should issues or suggestions arise that are related to this module, feel free to add a new GitHub issue.