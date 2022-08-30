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
  local relAddr = core.readInteger(callAddr + 1)
  return callAddr + relAddr + 5
end

local function getPtrToJumpAddr(jmpAddr) -- created for FF "num?" addr calls
  return core.readInteger(jmpAddr + 2)
end

local function getPtrAddrForCallToJmp(callAddr) -- helper for both funcs
  return getPtrToJumpAddr(getRelativeCallDest(callAddr))
end

local function getPtrAddrFromMov(movAddr) -- if the address of the address to call is moved in register 
  return core.readInteger(movAddr + 2)
end

local function getPtrAddrFromCall(callAddr) -- if the call is made using the address from a ptr address 
  return core.readInteger(callAddr + 2)
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
  binkControlObjAddr                = {"b9 ? ? ? 02 89 3d ? ? f9 00 e8 ? ? f9 ff"}, -- finds two, but both yield the information
  setSomeColorsFuncAddr             = {"81 3d ? ? f9 00 55 05 00 00 66 c7"},
  initDirectDraw                    = {"81 ec e8 01 00 00 53 55 8b d9 83 bb ec"},
  getSystemMetricAddr               = {"8B 1D ? ? ? ? 57 50 57 57 6A 01 FF D3"},
  getCursorPosAddr                  = {"FF 15 ? ? ? ? 8B 44 ? ? 3B C7 8B 4C ? ? 89 ? ? 89 ? ? 7F"},
  getForegroundWindowAddr           = {"FF 15 ? ? ? ? 3b 86 ac 00 00 00 0f 85 d4 00 00 00 39 ae f8 00 00 00"},
}

local addrFail = {}

exports.enable = function(self, moduleConfig, globalConfig)

  for name, aob in pairs(addresses) do -- add found address to table
    local addr = core.AOBScan(aob[1], 0x400000)
    if addr == nil then
      addrFail[#addrFail + 1] = name
    end
    aob[2] = addr
  end

  if next(addrFail) ~= nil then
    log(ERROR, "'graphicsApiReplacer' was unable to find the following AOBs:")
    for _, name in pairs(addrFail) do
      log(ERROR, string.format("%s:%s", name, addresses[name][1]))
    end
    error("'graphicsApiReplacer' can not be initialized.")
    return
  end


  --[[ load module ]]--

  local graphicsApiReplacer = require("graphicsApiReplacer.dll") -- loads the dll in memory and runs luaopen_graphicsApiReplacer
  self.graphicsApiReplacer = graphicsApiReplacer


  --[[ binding ]]--
  -- kinda unsafe -> call addresses in ret object

  -- actually a _thiscall, but changed to call own function
  core.writeCode(
    addresses.windowCreateCallAddr[2], -- extreme 1.41.1-E address: 0x0057C390 
    {0xe8, graphicsApiReplacer.funcAddress_CreateWindow - addresses.windowCreateCallAddr[2] - 5}
  )

  -- __thiscall function that would release the DirectDraw objects,
  -- however, only ret -> always able to draw (do not know if this breaks something
  core.writeCode(
    addresses.releaseDirectDrawAddr[2],  -- extreme 1.41.1-E address: 0x00467FB0
    {
      -- 0x31, 0xc0,       -- xor    eax,eax               (zero)
      -- 0x89, 0x01,       -- mov    DWORD PTR [ecx],eax   (set DrawReady to 0)
      0xc2, 0x04, 0x00  -- ret    0x4                   (remove one strack value)
    }
  )
  
  -- __thiscall function that would restore the DirectDraw objects,
  -- however, ret 0, too not trigger window init again
  core.writeCode(
    addresses.restoreDirectDrawAddr[2],  -- extreme 1.41.1-E address: 0x004680F0
    {
      0x31, 0xc0,       -- xor    eax,eax               (zero)
      0xc3              -- ret
    }
  )
  
  -- __thiscall function that sets some initial values,
  -- however, it will become the main init function
  core.writeCode(
    addresses.getDeviceCapsAddr[2],  -- extreme 1.41.1-E address: 0x00467D70
    {
      0xb8, graphicsApiReplacer.funcAddress_MainDrawInit,       -- mov eax, absolute address  -- call pos extreme: 0x00472C3D
      0xff, 0xe0                                      -- jmp eax
    }
  )
  
  -- 1366x768 seems to have a special handling that seems no obsolete
  -- replace enum with unused byte, to prevent different handling
  core.writeCode(
    addresses.conditionForResHandlingAddr[2],  -- extreme 1.41.1-E address: 0x0046FC2D
    {0x1F}
  )
  
    -- simply ret, we do not need this func anymore
  core.writeCode(
    addresses.setWindowRectAddr[2], -- extreme 1.41.1-E address: 0x0046F970
    {0xc3}
  )
  
  -- there is a "not really" rect structure that needs to be set
  core.writeCode(
    graphicsApiReplacer.address_FillWithWinSetRectObjBaseAddr, -- extreme 1.41.1-E address: 0x0046F9D6
    {core.readInteger(addresses.winSetRectObjBaseAddr[2] + 1)}
  )
  
  -- need set some values, need ptr to bink control obj
  core.writeCode(
    graphicsApiReplacer.address_FillWithBinkControlObjAddr, -- extreme 1.41.1-E address: 0x0047023D
    {core.readInteger(addresses.binkControlObjAddr[2] + 1)}
  )
  
  -- need to call this
  core.writeCode(
    graphicsApiReplacer.address_FillWithSetSomeColorsAddr, -- extreme 1.41.1-E address: 0x00467AC0
    {addresses.setSomeColorsFuncAddr[2]}
  )
  
  -- original init DD only needs to return 1
  core.writeCode(
    addresses.initDirectDraw[2],  -- extreme 1.41.1-E address: 0x0046FC90
    {
      0xb8, 0x01, 0x00, 0x00, 0x00,       -- mov eax, 0x00000001
      0xc3                                -- ret
    }
  )

  core.writeCode(
    getPtrAddrFromMov(addresses.getSystemMetricAddr[2]), -- extreme 1.41.1-E address: 0x0059E1D0
    {graphicsApiReplacer.funcAddress_GetSystemMetrics}
  )

  core.writeCode(
    getPtrAddrFromCall(addresses.getCursorPosAddr[2]), -- extreme 1.41.1-E address: 0x0059E1E8
    {graphicsApiReplacer.funcAddress_GetCursorPos}
  )
  
  core.writeCode(
    getPtrAddrFromCall(addresses.getForegroundWindowAddr[2]), -- extreme 1.41.1-E address: 0x0059E20C
    {graphicsApiReplacer.funcAddress_GetForegroundWindow}
  )



  --[[
      Configuration:
      The struct is structured like this:
      
      config
      {
        window
        {
          type                -- default: window      -- see graphicsApiReplacer.window.type table
          width               -- default: 1280;       -- num >= 0 (and <= 20000)
          height              -- default: 720;        -- num >= 0 (and <= 20000)
          pos                 -- default: middle      -- see graphicsApiReplacer.window.pos table
          continueOutOfFocus  -- default: pause       -- see graphicsApiReplacer.window.continueOutOfFocus table
        }
        
        graphic
        {
          api                 -- default: DirectX                 -- see graphicsApiReplacer.graphic.api table
          filterLinear        -- default: true        -- bool     -- if the texture filter should be linear, otherwise nearest
          vsync               -- default: true        -- bool     -- DirectX -> Works, OpenGL -> window mode enforces VSync
          waitWithGLFinish    -- default: false       -- bool     -- DirectX -> No effect, OpenGL -> calls glFinish after swap, to no effect?
          pixFormat           -- default: argb1555                -- see graphicsApiReplacer.graphic.pixFormat table
          debug               -- default: none                    -- see graphicsApiReplacer.graphic.debug table
        }
        
        control
        {
          clipCursor          -- default: true        -- bool                     -- cursor is bound to the game screen
          scrollActive        -- default: true        -- bool                     -- sets only mouse scroll
          margin              -- default: 0           -- num >= 0 (and <= 1000)   -- window pixels to extend the scroll zone outward (only no clipper, window modes)
          padding             -- default: 0           -- num >= 0 (and <= 1000)   -- window pixels to extend the scroll zone inward
        }
      }
      
      "graphicsApiReplacer.setConfigField" sets the values. Example:
          graphicsApiReplacer.setConfigField("window", "width", 600)
          
      Currently, the config should be modified before the game starts.
      Modifications after will cause undefined behavior.
      
      NOTES:
          - There is still stuff going on that I do not understand.
              - Crusader still can execute a few window functions. It does not seems to have much of an effect.
              - Sometimes the screen breaks during a switch. This can then only be seen in the minimized window in windows, not in-game. So no real effect.
              - Sometimes the extreme effect list goes missing... bug of the new system, or vanilla? -> Vanilla
              
      Potential TODO:
          - One thing Crusader is still allowed to do is "SetFocus".
              - Call might be redundant, but maybe removing this prevents even more unnecessary actions?
              - Future Me: I am not sure if this is still relevant.
  ]]--


  -- enum helper tables:
  -- name is equal to option
  graphicsApiReplacer.window = {}
  graphicsApiReplacer.graphic = {}
  graphicsApiReplacer.control = {}

  graphicsApiReplacer.window.type = {
    window                =   0,
    borderlessWindow      =   1,
    borderlessFullscreen  =   2,
  }

  -- only relevant for windowed modes
  graphicsApiReplacer.window.pos = {
    middle                =   0,
    topLeft               =   1,
    bottomLeft            =   2,
    topRight              =   3,
    bottomRight           =   4,
  }
  
  graphicsApiReplacer.window.continueOutOfFocus = {
    pause                 =   0,
    continue              =   1,
    render                =   2,
  }

  graphicsApiReplacer.graphic.pixFormat = {
    argb1555              =   0x555,
    rgb565                =   0x565,
  }

  graphicsApiReplacer.graphic.debug = {
    none                  =   0,
    enabled               =   1,  -- DirectX -> Enabled Debug Mode, OpenGL -> just enabled debug messages, might have no effect        
    debugContextEnabled   =   2,  -- DirectX -> Same as enabled, OpenGL -> tries to create a debug context, this should produce messages
  }
  
  graphicsApiReplacer.graphic.api = {
    DirectX     =   0,  -- far more compatible with stuff, but requires at least Windows 8
    OpenGL      =   1,  -- legacy, even more unlikely to receive new features and broken in some ways, but proven
  }

  -- do actual configuration
  
  for option, fields in pairs(moduleConfig) do
    for field, value in pairs(fields) do
      local enum = graphicsApiReplacer[option]
      if enum ~= nil then
        enum = enum[field]
        if enum ~= nil then
          value = enum[value]
        end
      end

      local status, err = pcall(graphicsApiReplacer.setConfigField, option, field, value)
      if not status then
        log(ERROR, err);
      end
    end
  end
end

exports.disable = function(self, moduleConfig, globalConfig) error("not implemented") end

return exports