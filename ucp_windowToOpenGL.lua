package.cpath = "ucp/modules/windowToOpenGL/?.dll;" .. package.cpath

ucpOpenGL = require("ucp_windowToOpenGL") -- loads the dll in memory and runs luaopen_ucp_windowToOpenGL

-- binding: currently hardcoded for extreme 1.41

-- actually a _thiscall, but changed to call own function
writeCode(0x00470189, {0xe8, ucpOpenGL.funcAddress_CreateWindow - 0x00470189 - 5})

-- replaces entries in jmp (table?)
writeCode(0x0059E010, {ucpOpenGL.funcAddress_DirectDrawCreate})
writeCode(0x0059E1D0, {ucpOpenGL.funcAddress_GetSystemMetrics})
writeCode(0x0059E200, {ucpOpenGL.funcAddress_SetRect})
writeCode(0x0059E1F8, {ucpOpenGL.funcAddress_SetWindowPos})
writeCode(0x0059E1E8, {ucpOpenGL.funcAddress_GetCursorPos})
writeCode(0x0059E1F4, {ucpOpenGL.funcAddress_UpdateWindow})
writeCode(0x0059E1FC, {ucpOpenGL.funcAddress_AdjustWindowRect})

-- already a call, replacing only address
writeCode(0x0057CCCA + 1, {ucpOpenGL.funcAddress_DetouredWindowLongPtrReceive - 0x0057CCCA - 5})

-- address of crusaders windowProcCallback needed, fill address of given variable with callback address
writeCode(ucpOpenGL.address_FillWithWindowProcCallback, {0x004B2C50})


--[[
    Configuration:
    The struct is structured like this:
    
    config
    {
      window
      {
        type                -- default: window      -- see ucpOpenGL.windowType table
        width               -- default: 1280;       -- num >= 0 (and <= 20000)
        height              -- default: 720;        -- num >= 0 (and <= 20000)
        pos                 -- default: middle      -- see ucpOpenGL.windowPos table
      }
      
      graphic
      {
        filterLinear        -- default: true        -- bool     -- if the texture filter should be linear, otherwise nearest
        vsync               -- default: true        -- bool     -- most likely only relevant for fullscreen modes
        waitWithGLFinish    -- default: false       -- bool     -- calls glFinish after swap -> also seems to prevent tearing, do not know what is better...
        pixFormat           -- default: argb1555                -- see ucpOpenGL.pixelFormat table
      }
      
      control
      {
        clipCursor          -- default: true        -- bool                     -- cursor is bound to the game screen
        scrollActive        -- default: true        -- bool                     -- sets only mouse scroll
        margin              -- default: 0           -- num >= 0 (and <= 1000)   -- window pixels to extend the scroll zone outward (only no clipper, window modes)
        padding             -- default: 0           -- num >= 0 (and <= 1000)   -- window pixels to extend the scroll zone inward
      }
    }
    
    "ucpOpenGL.setConfigField" sets the values. Example:
        ucpOpenGL.setConfigField("window", "width", 600)
        
    Currently, the config should be modified before the game starts.
    Modifications after will cause undefined behavior.
    
    NOTES:
        - There is still stuff going on that I do not understand.
            - Crusader still can execute a few window functions. It does not seems to have much of an effect.
            - Sometimes the screen breaks during a switch. This can then only be seen in the minimized window in windows, not in-game. So no real effect.
            - Sometimes the extreme effect list goes missing... bug of the new system, or vanilla?
]]--


-- enum helper tables:

ucpOpenGL.windowType = {
  window                =   0,
  borderlessWindow      =   1,
  borderlessFullscreen  =   2,
}

-- only relevant for windowed modes
ucpOpenGL.windowPos = {
  middle                =   0,
  topLeft               =   1,
  bottomLeft            =   2,
  topRight              =   3,
  bottomRight           =   4,
}

ucpOpenGL.pixelFormat = {
  argb1555              =   0,
  rgb565                =   1,
}


-- do actual configuration

ucpOpenGL.setConfigField("window", "type", ucpOpenGL.windowType.borderlessFullscreen)
-- ucpOpenGL.setConfigField("window", "width", 200) -- stupid test -> 200 for example causes issue with the scrolling system without padding
-- ucpOpenGL.setConfigField("control", "padding", 5) -- stupid test

-- print(MyModule.dummyFunction)

-- writeCode(hookAddress, {JMPFAR, MyModule.dummyFunction}) -- hook the game code up to the dll