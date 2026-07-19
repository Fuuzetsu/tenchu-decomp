-- Non-interactive PCSX-Redux probe used by tools/pcsx-smoke.py.
--
-- Addresses and limits deliberately come from the wrapper rather than being
-- copied from the retail layout: a normal relink is expected to move them.

local function requiredNumber(name)
    local value = os.getenv(name)
    local number = value and tonumber(value)
    if number == nil then
        printError("TENCHU_SMOKE FAIL missing numeric environment variable " .. name)
        PCSX.quit(2)
        error("missing " .. name)
    end
    return number
end

local entryAddress = requiredNumber("TENCHU_SMOKE_ENTRY")
local mainAddress = requiredNumber("TENCHU_SMOKE_MAIN")
local loopAddress = requiredNumber("TENCHU_SMOKE_LOOP")
local requiredFrames = requiredNumber("TENCHU_SMOKE_FRAMES")
local requiredLoops = requiredNumber("TENCHU_SMOKE_LOOPS")
local autoPad = os.getenv("TENCHU_SMOKE_AUTOPAD") == "1"

local pad
local padButtons
local autoPadButtons
if autoPad then
    pad = PCSX.SIO0.slots[1].pads[1]
    padButtons = PCSX.CONSTS.PAD.BUTTON
    autoPadButtons = {padButtons.START, padButtons.CIRCLE, padButtons.CROSS}
end

local function releaseAutoPad()
    if not autoPad then return end
    for _, button in ipairs(autoPadButtons) do pad.clearOverride(button) end
end

TenchuSmoke = {
    entryHits = 0,
    mainHits = 0,
    loopHits = 0,
    totalVsyncs = 0,
    frames = 0,
    failed = false,
    passed = false,
}

local function fail(message)
    if TenchuSmoke.failed then return end
    TenchuSmoke.failed = true
    local regs = PCSX.getRegisters()
    printError(string.format(
        "TENCHU_SMOKE FAIL %s pc=0x%08x cause=0x%08x frames=%d loops=%d",
        message, tonumber(regs.pc), tonumber(regs.CP0.r[13]),
        TenchuSmoke.frames, TenchuSmoke.loopHits))
    PCSX.quit(1)
end

local function oneShot(counter, breakpoint)
    return function()
        TenchuSmoke[counter] = TenchuSmoke[counter] + 1
        if counter == "mainHits" then releaseAutoPad() end
        TenchuSmoke[breakpoint]:disable()
        return true
    end
end

TenchuSmoke.entryBreakpoint = PCSX.addBreakpoint(
    entryAddress, "Exec", 4, "", oneShot("entryHits", "entryBreakpoint"), "Tenchu smoke entry")
TenchuSmoke.mainBreakpoint = PCSX.addBreakpoint(
    mainAddress, "Exec", 4, "", oneShot("mainHits", "mainBreakpoint"), "Tenchu smoke main")
TenchuSmoke.loopBreakpoint = PCSX.addBreakpoint(
    loopAddress, "Exec", 4, "", function()
        TenchuSmoke.loopHits = TenchuSmoke.loopHits + 1
        if TenchuSmoke.loopHits >= requiredLoops then
            TenchuSmoke.loopBreakpoint:disable()
        end
        return true
    end, "Tenchu smoke main loop")

TenchuSmoke.pauseListener = PCSX.Events.createEventListener(
    "ExecutionFlow::Pause", function(event)
        if event.exception then
            fail("first-chance CPU exception")
        elseif not TenchuSmoke.failed and not TenchuSmoke.passed then
            fail("emulator paused unexpectedly")
        end
    end)

TenchuSmoke.vsyncListener = PCSX.Events.createEventListener(
    "GPU::Vsync", function()
        TenchuSmoke.totalVsyncs = TenchuSmoke.totalVsyncs + 1
        if autoPad and TenchuSmoke.mainHits == 0 then
            -- The disc path passes through SLPS and MENU before MAIN.EXE.  A
            -- short, separated START/CIRCLE/CROSS pulse train advances their
            -- default choices without assuming the confirm-button region.
            local phase = TenchuSmoke.totalVsyncs % 90
            if phase == 1 then
                pad.setOverride(padButtons.START)
            elseif phase == 4 then
                pad.clearOverride(padButtons.START)
            elseif phase == 31 then
                pad.setOverride(padButtons.CIRCLE)
            elseif phase == 34 then
                pad.clearOverride(padButtons.CIRCLE)
            elseif phase == 61 then
                pad.setOverride(padButtons.CROSS)
            elseif phase == 64 then
                pad.clearOverride(padButtons.CROSS)
            end
        end
        -- OpenBIOS and Tenchu's one-time startup both generate VSyncs.  Count
        -- only frames after the per-frame loop probe has actually fired.
        if TenchuSmoke.loopHits == 0 then return end
        TenchuSmoke.frames = TenchuSmoke.frames + 1
        if TenchuSmoke.frames < requiredFrames then return end
        if TenchuSmoke.entryHits ~= 0 and
           TenchuSmoke.mainHits ~= 0 and
           TenchuSmoke.loopHits >= requiredLoops then
            TenchuSmoke.passed = true
            print(string.format(
                "TENCHU_SMOKE PASS entry=%d main=%d frames=%d loops=%d cycles=%s",
                TenchuSmoke.entryHits, TenchuSmoke.mainHits, TenchuSmoke.frames,
                TenchuSmoke.loopHits, tostring(PCSX.getCPUCycles())))
            PCSX.quit(0)
        end
    end)

print(string.format(
    "TENCHU_SMOKE armed entry=0x%08x main=0x%08x loop=0x%08x frames=%d loops=%d",
    entryAddress, mainAddress, loopAddress, requiredFrames, requiredLoops))
