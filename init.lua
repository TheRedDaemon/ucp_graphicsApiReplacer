local exports = {}

-- helper funcs
local function strToHexStr(str)
  local t = {string.byte(str, 1, -1)}
  for i, num in ipairs(t) do
    t[i] = intToHex(num)
  end
  return table.concat(t, " ")
end

local function intToLEHexStr(int)
  local t = itob(int)
  for i, num in ipairs(t) do
    t[i] = intToHex(num)
  end
  return table.concat(t, " ")
end


local function getRelativeCallDest(callAddr) -- created for E8 calls
  local relAddr = readInteger(callAddr + 1)
  return callAddr + relAddr + 5
end

local function getPtrToJumpAddr(jmpAddr) -- created for FF "num?" addr calls
  return readInteger(jmpAddr + 2)
end

local function getPtrAddrForCallToJmp(callAddr) -- helper for both funcs
  return getPtrToJumpAddr(getRelativeCallDest(callAddr))
end

local function getPtrAddrFromMov(movAddr) -- if the address of the address to call is moved in register 
  return readInteger(movAddr + 2)
end

local function getPtrAddrFromCall(callAddr) -- if the call is made using the address from a ptr address 
  return readInteger(callAddr + 2)
end



--[[ addresses ]]--

-- get addresses
local addresses = {
  windowCreateCallAddr              = {"E8 ? ? ? ? 85 C0 74 2F 6A 00 FF 15"},
  directDrawCreateAddr              = {"E8 ? ? ? ? BE 01 00 00 00 89 B3 F8 00 00 00"},
  getSystemMetricAddr               = {"8B 1D ? ? ? ? 57 50 57 57 6A 01 FF D3"},
  setRectAddr                       = {"8B 1D ? ? ? ? 51 52 6A 00 6A 00 8D ? ? ? 50"},
  setWindowPosAddr                  = {"FF 15 ? ? ? ? 8B 96 ? ? ? ? 52 FF 15 ? ? ? ? 8B 86 ? ? ? ? 50 FF 15 ? ? ? ? 5F 5E 5D"},
  getCursorPosAddr                  = {"FF 15 ? ? ? ? 8B 44 ? ? 3B C7 8B 4C ? ? 89 ? ? 89 ? ? 7F"},
  updateWindowAddr                  = {"FF 15 ? ? ? ? 8B 86 ? ? ? ? 50 FF 15 ? ? ? ? 5F 5E 5D 5B 83 C4 10 C3"},
  adjustWindowRectAddr              = {"FF 15 ? ? ? ? 8B 56 ? 8B 46 ? 52 50 6A 00 6A 00 B9"},
  windowProcCallAddr                = {"83 EC 48 A1 ? ? ? ? 33 C4 89 44 ? ? 8B 44 ? ? 8B 4C ? ? 53 55 56 57"},
  
  -- handling different
  windowLongStrAddr                 = {strToHexStr("SetWindowLongA")}
}

local addrFail = {}

exports.enable = function(self, moduleConfig, globalConfig)

  for name, aob in pairs(addresses) do -- add found address to table
    local addr = scanForAOB(aob[1], 0x400000)
    if addr == nil then
      addrFail[#addrFail + 1] = name
    end
    aob[2] = addr
  end

  -- extra structure for windowLongDetourAddr, since I need the string position before
  do
    local windowLongStrAddr = addresses.windowLongStrAddr[2]
    if windowLongStrAddr ~= nil then
      addresses.windowLongDetourAddr = {"68" .. intToLEHexStr(windowLongStrAddr)}
      local addrForWindowLong = scanForAOB(addresses.windowLongDetourAddr[1], 0x400000)
      if addrForWindowLong == nil then
        addrFail[#addrFail + 1] = "windowLongDetourAddr"
      end
      addresses.windowLongDetourAddr[2] = addrForWindowLong + 10 -- needed to be on call
    end
  end

  if next(addrFail) ~= nil then
    print("'ucp_windowToOpenGL' was unable to find the following AOBs:")
    for _, name in pairs(addrFail) do
      print("", name, ":", addresses[name][1])
    end
    error("'ucp_windowToOpenGL' can not be initialized.")
    return
  end


  --[[ load module ]]--

  local ucpOpenGL = require("ucp_windowToOpenGL.dll") -- loads the dll in memory and runs luaopen_ucp_windowToOpenGL
  self.ucpOpenGL = ucpOpenGL


  --[[ binding ]]--

  -- actually a _thiscall, but changed to call own function
  writeCode(
    addresses.windowCreateCallAddr[2], -- extreme 1.41.1-E address: 0x00470189
    {0xe8, ucpOpenGL.funcAddress_CreateWindow - addresses.windowCreateCallAddr[2] - 5}
  )


  -- replaces entries in jmp (table?)
  writeCode(
    getPtrAddrForCallToJmp(addresses.directDrawCreateAddr[2]),  -- extreme 1.41.1-E address: 0x0059E010
    {ucpOpenGL.funcAddress_DirectDrawCreate}
  )

  writeCode(
    getPtrAddrFromMov(addresses.getSystemMetricAddr[2]), -- extreme 1.41.1-E address: 0x0059E1D0
    {ucpOpenGL.funcAddress_GetSystemMetrics}
  )

  writeCode(
    getPtrAddrFromMov(addresses.setRectAddr[2]), -- extreme 1.41.1-E address: 0x0059E200
    {ucpOpenGL.funcAddress_SetRect}
  )

  writeCode(
    getPtrAddrFromCall(addresses.setWindowPosAddr[2]), -- extreme 1.41.1-E address: 0x0059E1F8
    {ucpOpenGL.funcAddress_SetWindowPos}
  )

  writeCode(
    getPtrAddrFromCall(addresses.getCursorPosAddr[2]), -- extreme 1.41.1-E address: 0x0059E1E8
    {ucpOpenGL.funcAddress_GetCursorPos}
  )

  writeCode(
    getPtrAddrFromCall(addresses.updateWindowAddr[2]), -- extreme 1.41.1-E address: 0x0059E1F4
    {ucpOpenGL.funcAddress_UpdateWindow}
  )

  writeCode(
    getPtrAddrFromCall(addresses.adjustWindowRectAddr[2]), -- extreme 1.41.1-E address: 0x0059E1FC
    {ucpOpenGL.funcAddress_AdjustWindowRect}
  )


  -- already a call, replacing only address
  writeCode(
    addresses.windowLongDetourAddr[2] + 1,   -- extreme 1.41.1-E address: 0x0057CCCA
    {ucpOpenGL.funcAddress_DetouredWindowLongPtrReceive - addresses.windowLongDetourAddr[2] - 5}
  )


  -- address of crusaders windowProcCallback needed, fill address of given variable with callback address
  writeCode(
    ucpOpenGL.address_FillWithWindowProcCallback,
    {addresses.windowProcCallAddr[2]}  -- extreme 1.41.1-E address: 0x004B2C50
  )



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
          debug               -- default: off                     -- 
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
              
      Potential TODO:
          - One thing Crusader is still allowed to do is "SetFocus".
              - Call might be redundant, but maybe removing this prevents even more unnecessary actions?
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
  
  ucpOpenGL.debugOpenGL = {
    off                   =   0,
    enabled               =   1,  -- just enabled debug messages, might have no effect        
    debugContextEnabled   =   2,  -- tries to create a debug context, this should produce messages
  }


  -- do actual configuration
  
  -- debugOpenGL
  ucpOpenGL.setConfigField("graphic", "debug", ucpOpenGL.debugOpenGL.debugContextEnabled)

  -- moduleConfig.window.type
  -- ucpOpenGL.setConfigField("window", "type", ucpOpenGL.windowType[moduleConfig.window.type])
  
  ucpOpenGL.setConfigField("window", "type", ucpOpenGL.windowType[moduleConfig.window.type])
  ucpOpenGL.setConfigField("control", "clipCursor", moduleConfig.control.clipCursor)
  ucpOpenGL.setConfigField("window", "width", moduleConfig.window.width)
  ucpOpenGL.setConfigField("window", "height", moduleConfig.window.height)
  ucpOpenGL.setConfigField("window", "pos", ucpOpenGL.windowPos[moduleConfig.window.pos])
end

exports.disable = function(self, moduleConfig, globalConfig) error("not implemented") end

return exports
