-- Humanoid.attribute bit observer, driven by tools/pcsx_attrbits.py.
--
-- A research probe, not a gate: boots the full disc chain exactly like
-- tools/pcsx-gameplay.lua, and once the character simulation is live it
-- steers the player forward (with periodic turns) to provoke detection
-- while polling every HumanGroup entry each frame.  Any change in a
-- human's attribute or status word is logged with full context
-- (type/status/attribute before and after, EmergencyNotice, the player's
-- current motID) so the attribute bits can be named from observed
-- transitions.  After the observation window it prints PASS and a
-- transition summary regardless of what was seen.
--
-- RETAIL-LAYOUT ONLY: the polled globals are the retail addresses below;
-- run it on the byte-identical .shake/build/tenchu image, never the
-- relink/growth artifacts.
--
-- PCSX-Redux Lua lessons baked in (from pcsx-gameplay.lua): keep every
-- addBreakpoint/createEventListener return referenced, and never throw
-- from a callback — hence the pcall guards.

local ffi = require("ffi")

local function requiredNumber(name)
    local value = os.getenv(name)
    local number = value and tonumber(value)
    if number == nil then
        printError("TENCHU_GAMEPLAY FAIL missing numeric environment variable " .. name)
        PCSX.quit(2)
        error("missing " .. name)
    end
    return number
end

local function optionalNumber(name)
    local value = os.getenv(name)
    return value and tonumber(value) or nil
end

local addresses = {
    main = requiredNumber("TENCHU_GAMEPLAY_MAIN"),
    loop = requiredNumber("TENCHU_GAMEPLAY_LOOP"),
    createstage = requiredNumber("TENCHU_GAMEPLAY_CREATESTAGE"),
    activate = requiredNumber("TENCHU_GAMEPLAY_ACTIVATE"),
    actstate = requiredNumber("TENCHU_GAMEPLAY_ACTSTATE"),
    actnormal = requiredNumber("TENCHU_GAMEPLAY_ACTNORMAL"),
    cbcheck = requiredNumber("TENCHU_GAMEPLAY_CBCHECK"),
}
local words = {
    main = requiredNumber("TENCHU_GAMEPLAY_MAIN_W"),
    loop = requiredNumber("TENCHU_GAMEPLAY_LOOP_W"),
    createstage = requiredNumber("TENCHU_GAMEPLAY_CREATESTAGE_W"),
    activate = requiredNumber("TENCHU_GAMEPLAY_ACTIVATE_W"),
    actstate = requiredNumber("TENCHU_GAMEPLAY_ACTSTATE_W"),
    actnormal = requiredNumber("TENCHU_GAMEPLAY_ACTNORMAL_W"),
    cbcheck = requiredNumber("TENCHU_GAMEPLAY_CBCHECK_W"),
}
local maxVsyncs = optionalNumber("TENCHU_GAMEPLAY_MAX_VSYNCS") or 60000
local observeVsyncs = optionalNumber("TENCHU_ATTR_OBSERVE") or 9000

-- Retail data layout (config/symbols.main.exe.txt).
local HUMANS = 0x80097724        -- short: live humanoid count
local HUMANGROUP = 0x800be7b8    -- struct Humanoid *[]
local EMERGENCY = 0x800979c0     -- long EmergencyNotice
local MOTID = 0x80097f0c         -- short motID (current thinker's next motion)
local STAGEPLAYER = 0x80097c60   -- struct Humanoid *StagePlayer
local MAXHUMANS = 24

local function ram(address)
    local base = ffi.cast("uint8_t*", PCSX.getMemPtr())
    return base + bit.band(address, 0x1fffff)
end
local function readU32(address)
    return tonumber(ffi.cast("uint32_t*", ram(address))[0])
end
local function readU16(address)
    return tonumber(ffi.cast("uint16_t*", ram(address))[0])
end

TenchuAttr = {
    main = 0, loop = 0, createstage = 0, activate = 0, actstate = 0,
    actnormal = 0, cbcheck = 0, vsyncs = 0, failed = false, passed = false,
    breakpoints = {}, callbackErrors = {},
    observing = 0, transitions = 0,
    prevAttr = {}, prevStat = {},
}
local GP = TenchuAttr

local function fail(message)
    if GP.failed or GP.passed then return end
    GP.failed = true
    local regs = PCSX.getRegisters()
    printError(string.format(
        "TENCHU_GAMEPLAY FAIL %s pc=0x%08x create=%d act=%d/%d/%d cb=%d vsyncs=%d",
        message, tonumber(regs.pc), GP.createstage, GP.activate,
        GP.actstate, GP.actnormal, GP.cbcheck, GP.vsyncs))
    PCSX.quit(1)
end

local function counter(key)
    GP.breakpoints[key] = PCSX.addBreakpoint(
        addresses[key], "Exec", 4, "", function()
            local ok, err = pcall(function()
                if GP.main == 0 and key ~= "main" then return end
                if readU32(addresses[key]) ~= words[key] then return end
                GP[key] = GP[key] + 1
            end)
            if not ok and not GP.callbackErrors[key] then
                GP.callbackErrors[key] = true
                printError(string.format(
                    "TENCHU_GAMEPLAY callback error at %s: %s", key,
                    tostring(err)))
            end
            return true
        end, "Tenchu attr " .. key)
    end
for key in pairs(addresses) do counter(key) end

local pad = PCSX.SIO0.slots[1].pads[1]
local buttons = PCSX.CONSTS.PAD.BUTTON
local pulseButtons = {buttons.START, buttons.CIRCLE, buttons.CROSS,
                      buttons.UP, buttons.LEFT, buttons.RIGHT}
local function releasePad()
    for _, button in ipairs(pulseButtons) do pad.clearOverride(button) end
end

local function pollHumans()
    local player = readU32(STAGEPLAYER)
    local count = readU16(HUMANS)
    if count > MAXHUMANS then count = MAXHUMANS end
    for i = 0, count - 1 do
        local hp = readU32(HUMANGROUP + i * 4)
        if hp >= 0x80000000 and hp < 0x80200000 then
            local attr = readU16(hp + 4)
            local stat = readU16(hp + 2)
            local key = i
            local pa = GP.prevAttr[key]
            local ps = GP.prevStat[key]
            if pa ~= nil and (pa ~= attr or ps ~= stat) then
                GP.transitions = GP.transitions + 1
                print(string.format(
                    "TENCHU_ATTR t=%d h=%d%s type=%02x stat %02x->%02x attr %04x->%04x EN=%d motID=%04x",
                    GP.vsyncs, i, (hp == player) and "*" or "",
                    readU16(hp + 0), ps, stat, pa, attr,
                    readU32(EMERGENCY), readU16(MOTID)))
            end
            GP.prevAttr[key] = attr
            GP.prevStat[key] = stat
        end
    end
end

GP.pauseListener = PCSX.Events.createEventListener(
    "ExecutionFlow::Pause", function(event)
        if event.exception then
            fail("first-chance CPU exception")
        elseif not GP.failed and not GP.passed then
            fail("emulator paused unexpectedly")
        end
    end)

GP.vsyncListener = PCSX.Events.createEventListener("GPU::Vsync", function()
    GP.vsyncs = GP.vsyncs + 1

    local active = GP.activate > 0 or GP.actstate > 0 or GP.actnormal > 0
    if active then
        -- Observation phase: walk the player around to provoke detection.
        GP.observing = GP.observing + 1
        local ok, err = pcall(pollHumans)
        if not ok and not GP.callbackErrors.poll then
            GP.callbackErrors.poll = true
            printError("TENCHU_GAMEPLAY poll error: " .. tostring(err))
        end
        local phase = GP.observing % 600
        if phase == 1 then
            releasePad()
            pad.setOverride(buttons.UP)
        elseif phase == 420 then
            pad.setOverride(buttons.LEFT)
        elseif phase == 480 then
            pad.clearOverride(buttons.LEFT)
        end
        if GP.observing >= observeVsyncs then
            GP.passed = true
            releasePad()
            print(string.format(
                "TENCHU_GAMEPLAY PASS observed=%d transitions=%d act=%d/%d/%d cb=%d vsyncs=%d",
                GP.observing, GP.transitions, GP.activate, GP.actstate,
                GP.actnormal, GP.cbcheck, GP.vsyncs))
            PCSX.quit(0)
        end
    elseif GP.createstage > 0 then
        local phase = GP.vsyncs % 300
        if phase == 1 then pad.setOverride(buttons.START)
        elseif phase == 8 then pad.clearOverride(buttons.START)
        elseif phase == 150 then pad.setOverride(buttons.CIRCLE)
        elseif phase == 157 then pad.clearOverride(buttons.CIRCLE)
        end
    else
        local phase = GP.vsyncs % 90
        if phase == 1 then pad.setOverride(buttons.START)
        elseif phase == 8 then pad.clearOverride(buttons.START)
        elseif phase == 45 then pad.setOverride(buttons.CIRCLE)
        elseif phase == 52 then pad.clearOverride(buttons.CIRCLE)
        end
    end

    if GP.vsyncs % 1800 == 0 then
        print(string.format(
            "TENCHU_GAMEPLAY tick vsyncs=%d main=%d loop=%d create=%d act=%d/%d/%d cb=%d obs=%d tr=%d",
            GP.vsyncs, GP.main, GP.loop, GP.createstage, GP.activate,
            GP.actstate, GP.actnormal, GP.cbcheck, GP.observing, GP.transitions))
    end

    if not GP.passed and not GP.failed and GP.vsyncs > maxVsyncs then
        fail("observation window never opened before the vsync budget")
    end
end)

print(string.format(
    "TENCHU_GAMEPLAY armed (attr observer) main=0x%08x create=0x%08x activate=0x%08x observe=%d maxVsyncs=%d",
    addresses.main, addresses.createstage, addresses.activate,
    observeVsyncs, maxVsyncs))
