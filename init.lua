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
  windowCreateCallAddr              = {"E8 ? ? EF FF 89 2D ? ? F2 00 E8 ? ? EC FF"},
  releaseDirectDrawAddr             = {"53 8B 5C 24 08 56 57 33 FF 3b DF 8B F1 89 3E 74 1B"},
  restoreDirectDrawAddr             = {"56 8b f1 83 be f8 00 00 00 01 75 1b 83 be f4"},
  getDeviceCapsAddr                 = {"51 53 56 57 6a 00 8b f1 ff 15 ? ? 59 00 8b 1d"},
  conditionForResHandlingAddr       = {"05 89 41 24 8d 04 3f 89 79 38 89 71 3c 89 41 40"},
  setWindowRectAddr                 = {"83 ec 10 53 55 56 57 8b f1 8b 7e 58 8b 86"},
  winSetRectObjBaseAddr             = {"b9 ? ? ? ? e8 ? ? ff ff 83 7e 58 00 74 11"},
  
  directDrawCreateAddr              = {"E8 ? ? ? ? BE 01 00 00 00 89 B3 F8 00 00 00"},
  getSystemMetricAddr               = {"8B 1D ? ? ? ? 57 50 57 57 6A 01 FF D3"},
  setWindowPosAddr                  = {"FF 15 ? ? ? ? 8B 96 ? ? ? ? 52 FF 15 ? ? ? ? 8B 86 ? ? ? ? 50 FF 15 ? ? ? ? 5F 5E 5D"},
  getCursorPosAddr                  = {"FF 15 ? ? ? ? 8B 44 ? ? 3B C7 8B 4C ? ? 89 ? ? 89 ? ? 7F"},
  updateWindowAddr                  = {"FF 15 ? ? ? ? 8B 86 ? ? ? ? 50 FF 15 ? ? ? ? 5F 5E 5D 5B 83 C4 10 C3"},
  adjustWindowRectAddr              = {"FF 15 ? ? ? ? 8B 56 ? 8B 46 ? 52 50 6A 00 6A 00 B9"},
  windowProcCallAddr                = {"83 EC 48 A1 ? ? ? ? 33 C4 89 44 ? ? 8B 44 ? ? 8B 4C ? ? 53 55 56 57"},
  getForegroundWindowAddr           = {"FF 15 ? ? ? ? 3b 86 ac 00 00 00 0f 85 d4 00 00 00 39 ae f8 00 00 00"},
  
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
  -- kinda unsafe -> call addresses in ret object

  -- actually a _thiscall, but changed to call own function
  writeCode(
    addresses.windowCreateCallAddr[2], -- extreme 1.41.1-E address: 0x0057C390 
    {0xe8, ucpOpenGL.funcAddress_CreateWindow - addresses.windowCreateCallAddr[2] - 5}
  )

  -- __thiscall function that would release the DirectDraw objects,
  -- however, only ret -> always able to draw (do not know if this breaks something
  writeCode(
    addresses.releaseDirectDrawAddr[2],  -- extreme 1.41.1-E address: 0x00467FB0
    {
      -- 0x31, 0xc0,       -- xor    eax,eax               (zero)
      -- 0x89, 0x01,       -- mov    DWORD PTR [ecx],eax   (set DrawReady to 0)
      0xc2, 0x04, 0x00  -- ret    0x4                   (remove one strack value)
    }
  )
  
  -- __thiscall function that would restore the DirectDraw objects,
  -- however, ret 0, too not trigger window init again
  writeCode(
    addresses.restoreDirectDrawAddr[2],  -- extreme 1.41.1-E address: 0x004680F0
    {
      0x31, 0xc0,       -- xor    eax,eax               (zero)
      0xc3              -- ret
    }
  )
  
  -- __thiscall function that sets some initial values,
  -- however, it will become the main init function
  writeCode(
    addresses.getDeviceCapsAddr[2],  -- extreme 1.41.1-E address: 0x00467D70
    {
      0xb8, ucpOpenGL.funcAddress_MainDrawInit,       -- mov eax, absolute address  -- call pos extreme: 0x00472C3D
      0xff, 0xe0                                      -- jmp eax
    }
  )
  
  -- 1366x768 seems to have a special handling that seems no obsolete
  -- replace enum with unused byte, to prevent different handling
  writeCode(
    addresses.conditionForResHandlingAddr[2],  -- extreme 1.41.1-E address: 0x0046FC2D
    {0x1F}
  )
  
    -- simply ret, we do not need this func anymore
  writeCode(
    addresses.setWindowRectAddr[2], -- extreme 1.41.1-E address: 0x0046F970
    {0xc3}
  )
  
  -- there is a "not really" rect structure that needs to be set
  writeCode(
    ucpOpenGL.address_FillWithWinSetRectObjBaseAddr, -- extreme 1.41.1-E address: 0x0046F9D6
    {readInteger(addresses.winSetRectObjBaseAddr[2] + 1)}
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
  
  writeCode(
    getPtrAddrFromCall(addresses.getForegroundWindowAddr[2]), -- extreme 1.41.1-E address: 0x0059E20C
    {ucpOpenGL.funcAddress_GetForegroundWindow}
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
          continueOutOfFocus  -- default: pause       -- see ucpOpenGL.windowContinue table
        }
        
        graphic
        {
          filterLinear        -- default: true        -- bool     -- if the texture filter should be linear, otherwise nearest
          vsync               -- default: true        -- bool     -- most likely only relevant for fullscreen modes
          waitWithGLFinish    -- default: false       -- bool     -- calls glFinish after swap -> also seems to prevent tearing, do not know what is better...
          pixFormat           -- default: argb1555                -- see ucpOpenGL.pixelFormat table
          debug               -- default: none                    -- (no use at the moment) see ucpOpenGL.debugOpenGL table
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
  -- name is equal to option
  ucpOpenGL.window = {}
  ucpOpenGL.graphic = {}
  ucpOpenGL.control = {}

  ucpOpenGL.window.type = {
    window                =   0,
    borderlessWindow      =   1,
    borderlessFullscreen  =   2,
  }

  -- only relevant for windowed modes
  ucpOpenGL.window.pos = {
    middle                =   0,
    topLeft               =   1,
    bottomLeft            =   2,
    topRight              =   3,
    bottomRight           =   4,
  }
  
  ucpOpenGL.window.continueOutOfFocus = {
    pause                 =   0,
    continue              =   1,
    render                =   2,
  }

  ucpOpenGL.graphic.pixFormat = {
    argb1555              =   0x555,
    rgb565                =   0x565,
  }
  
  -- no effect at the moment
  ucpOpenGL.graphic.debug = {
    none                  =   0,
    enabled               =   1,  -- just enabled debug messages, might have no effect        
    debugContextEnabled   =   2,  -- tries to create a debug context, this should produce messages
  }

  -- do actual configuration
  
  for option, fields in pairs(moduleConfig) do
    for field, value in pairs(fields) do
      local enum = ucpOpenGL[option]
      if enum ~= nil then
        enum = enum[field]
        if enum ~= nil then
          value = enum[value]
        end
      end

      local status, err = pcall(ucpOpenGL.setConfigField, option, field, value)
      if not status then
        print(err);
      end
    end
  end
end

exports.disable = function(self, moduleConfig, globalConfig) error("not implemented") end

return exports
