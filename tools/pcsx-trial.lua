-- Direct TRIAL boot, new-mission setup, card check, and terrain-editor input.
-- All execution probes verify the selected EXE word. Input uses ordinary pad
-- overrides; this probe does not patch game memory or bypass a menu transition.
-- Keep all callbacks referenced, and fail on callback errors or CPU exceptions.

local ffi = require('ffi')
local function num(key)
    return assert(tonumber(os.getenv('TENCHU_TRIAL_' .. key)), key)
end
local addr, word = {}, {}
for _, key in ipairs({'MAIN', 'MENU', 'SETTINGS', 'CARD', 'EDITOR', 'MAP', 'READPAD', 'REALPAD'}) do
    addr[key] = num(key)
    word[key] = num(key .. '_WORD')
end
local function ptr(address)
    return ffi.cast('uint8_t*', PCSX.getMemPtr()) + bit.band(address, 0x1fffff)
end
local function u32(address)
    return tonumber(ffi.cast('uint32_t*', ptr(address))[0])
end
local function s16(address)
    return tonumber(ffi.cast('int16_t*', ptr(address))[0])
end
local pad = PCSX.SIO0.slots[1].pads[1]
local buttons = PCSX.CONSTS.PAD.BUTTON
local used = {buttons.DOWN, buttons.CIRCLE, buttons.START, buttons.RIGHT, buttons.LEFT}
local function release()
    for _, button in ipairs(used) do pad.clearOverride(button) end
end
TenchuTrialProbe = {
    breakpoints = {},
    vsyncs = 0,
    screen = 'boot',
    ready = false,
    ticks = 0,
    mapPolls = 0,
    main = false,
    done = false,
    moved = false,
    returned = false,
}
local p = TenchuTrialProbe
local mapEnd, cursor, mission = num('MAP_END'), num('CURSOR'), num('MISSION')
local function fail(reason)
    if p.done then return end
    p.done = true
    release()
    local regs = PCSX.getRegisters()
    printError(string.format(
        'TENCHU_TRIAL FAIL %s screen=%s pc=%08x cause=%08x vsyncs=%d polls=%d',
        reason, p.screen, tonumber(regs.pc), tonumber(regs.CP0.r[13]), p.vsyncs, p.mapPolls))
    PCSX.quit(1)
end
local function screen(name)
    release()
    p.screen = name
    p.ready = false
    p.ticks = 0
    print('TENCHU_TRIAL screen=' .. name .. ' vsync=' .. p.vsyncs)
end
local function ready()
    if p.ready then return end
    p.ready = true
    p.ticks = 0
    print('TENCHU_TRIAL ready=' .. p.screen)
end
local function hook(key, fn)
    p.breakpoints[key] = PCSX.addBreakpoint(addr[key], 'Exec', 4, '', function()
        local ok, err = pcall(function()
            if u32(addr[key]) ~= word[key] then return end
            if not p.main and key ~= 'MAIN' then return end
            fn()
        end)
        if not ok then fail(tostring(err)) end
        return true
    end, 'TRIAL probe ' .. key)
end
hook('MAIN', function() p.main = true end)
hook('MENU', function() screen('menu') end)
hook('SETTINGS', function() screen('settings') end)
hook('CARD', function()
    if p.screen ~= 'card' then screen('card') end
end)
hook('EDITOR', function() screen('editor-loading') end)
hook('MAP', function() screen('map') end)
hook('READPAD', function()
    if p.screen == 'menu' or p.screen == 'settings' then ready() end
end)
hook('REALPAD', function()
    local ra = tonumber(PCSX.getRegisters().GPR.n.ra)
    if p.screen == 'map' and ra >= addr.MAP and ra < mapEnd then
        p.mapPolls = p.mapPolls + 1
        if p.mapPolls >= 2 then ready() end
    end
end)
p.pause = PCSX.Events.createEventListener('ExecutionFlow::Pause', function(event)
    if event.exception then
        fail('CPU exception')
    elseif not p.done then
        fail('unexpected pause')
    end
end)
p.vsync = PCSX.Events.createEventListener('GPU::Vsync', function()
    local ok, err = pcall(function()
        if p.done then return end
        p.vsyncs = p.vsyncs + 1
        if p.vsyncs > 12000 then
            fail('frame budget exceeded')
            return
        end
        if not p.ready then return end
        p.ticks = p.ticks + 1
        local t = p.ticks
        if p.screen == 'menu' then
            if t == 5 then pad.setOverride(buttons.DOWN)
            elseif t == 13 then pad.clearOverride(buttons.DOWN)
            elseif t == 25 then pad.setOverride(buttons.CIRCLE)
            elseif t == 33 then pad.clearOverride(buttons.CIRCLE) end
        elseif p.screen == 'settings' then
            if t == 15 then pad.setOverride(buttons.START)
            elseif t == 23 then pad.clearOverride(buttons.START) end
        elseif p.screen == 'map' then
            local x = s16(cursor + 4) -- GsSPRITE.x
            if t == 1 and x ~= 0 then
                fail('initial cursor assertion')
                return
            end
            if x == 25 then p.moved = true end
            if t > 50 and p.moved and x == 0 then p.returned = true end
            if t == 10 then pad.setOverride(buttons.RIGHT)
            elseif t == 18 then pad.clearOverride(buttons.RIGHT)
            elseif t == 45 then pad.setOverride(buttons.LEFT)
            elseif t == 53 then pad.clearOverride(buttons.LEFT) end
            if t >= 90 then
                if not p.moved or not p.returned or p.mapPolls < 12 then
                    fail('cursor/input assertion')
                    return
                end
                -- Serialized TrialMissionData: theme, player kind, and grid x/z.
                if s16(mission + 6) ~= 0 or s16(mission + 0x8ec) ~= 0
                    or s16(mission + 0x8ee) ~= 16 or s16(mission + 0x8f0) ~= 18 then
                    fail('new mission placement assertion')
                    return
                end
                p.done = true
                release()
                print(string.format(
                    'TENCHU_TRIAL PASS create/settings/card/editor cursor=0->25->0 polls=%d frames=%d vsyncs=%d',
                    p.mapPolls, t, p.vsyncs))
                PCSX.quit(0)
            end
        end
    end)
    if not ok then fail(tostring(err)) end
end)
print('TENCHU_TRIAL armed')
