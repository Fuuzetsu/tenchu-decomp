-- Direct TRIAL boot, new-mission setup, card check, and terrain-editor input.
-- All execution probes verify the selected EXE word. Input uses ordinary pad
-- overrides; this probe does not patch game memory or bypass a menu transition.
-- Keep all callbacks referenced, and fail on callback errors or CPU exceptions.

local ffi = require('ffi')
local function num(key)
    return assert(tonumber(os.getenv('TENCHU_TRIAL_' .. key)), key)
end
local addr, word = {}, {}
for _, key in ipairs({'MAIN', 'MENU', 'SETTINGS', 'CARD', 'EDITOR', 'MAP', 'READPAD', 'REALPAD',
                     'COMMANDS', 'EDITOR_SAVE', 'CARD_SAVE', 'TEXT', 'TEXT_INPUT'}) do
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
local used = {buttons.DOWN, buttons.CIRCLE, buttons.START, buttons.RIGHT, buttons.LEFT,
              buttons.SELECT, buttons.L2, buttons.R2}
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
local save, saveState = num('SAVE') ~= 0, num('SAVE_STATE')
local textEntry = num('TEXT_ENTRY') ~= 0
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
hook('SETTINGS', function()
    if textEntry and p.textEntered then
        -- EditTrialTextField copies the output to this header and clears its
        -- shared scratch buffer before the settings form is called again.
        local header = tonumber(PCSX.getRegisters().GPR.n.a1)
        if not p.textExpected or ffi.string(ptr(header + 8), 30) ~= p.textExpected then
            fail('text-entry Shift-JIS export assertion')
            return
        end
        p.textPassed = true
        print(string.format('TENCHU_TRIAL text-entry category=0->1->0 code=%04x export=verified',
                            p.textCode))
    end
    screen('settings')
end)
hook('TEXT', function()
    if textEntry then
        p.textEntered = true
        screen('text-loading')
    end
end)
hook('TEXT_INPUT', function()
    if textEntry and p.screen == 'text-loading' then
        screen('text')
        ready()
    end
end)
hook('CARD', function()
    if p.screen ~= 'card' then screen('card') end
end)
hook('EDITOR', function() screen('editor-loading') end)
hook('MAP', function()
    p.entryPolls = 0
    screen('map')
end)
hook('COMMANDS', function()
    if not save or p.screen == 'commands' then return end
    if bit.band(tonumber(PCSX.getRegisters().GPR.n.a0), 0xffff) ~= 0 then
        screen('commands')
        ready()
    end
end)
hook('EDITOR_SAVE', function()
    if save then screen('save-fade') end
end)
hook('CARD_SAVE', function()
    if save and p.screen ~= 'save' then
        screen('save')
        ready()
    end
end)
hook('READPAD', function()
    if p.screen == 'menu' or p.screen == 'settings' then ready() end
end)
hook('REALPAD', function()
    local ra = tonumber(PCSX.getRegisters().GPR.n.ra)
    if p.screen == 'map' and ra >= addr.MAP and ra < mapEnd then
        p.mapPolls = p.mapPolls + 1
        p.entryPolls = p.entryPolls + 1
        if p.entryPolls >= 2 then ready() end
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
            local button = textEntry and not p.textEntered and buttons.CIRCLE or buttons.START
            if t == 15 then pad.setOverride(button)
            elseif t == 23 then pad.clearOverride(button) end
        elseif p.screen == 'text' then
            local keyboard = u32(num('KEYBOARD_CURSOR'))
            local edited = u32(num('TEXT_CURSOR'))
            if t == 15 then pad.setOverride(buttons.R2)
            elseif t == 23 then pad.clearOverride(buttons.R2)
            elseif t == 35 then
                if s16(keyboard + 4) ~= 1 then
                    fail('text-entry next-category assertion')
                    return
                end
            elseif t == 45 then pad.setOverride(buttons.L2)
            elseif t == 53 then pad.clearOverride(buttons.L2)
            elseif t == 65 then
                if s16(keyboard + 4) ~= 0 then
                    fail('text-entry previous-category assertion')
                    return
                end
                p.textCode = bit.band(s16(u32(keyboard + 12)), 0xffff)
                if p.textCode == 0 or p.textCode == 0x8140 then
                    fail('text-entry selected a blank key')
                    return
                end
                pad.setOverride(buttons.CIRCLE)
            elseif t == 73 then pad.clearOverride(buttons.CIRCLE)
            elseif t == 100 then
                local characters = u32(edited + 8)
                if s16(edited) ~= 1 or bit.band(s16(characters), 0xffff) ~= p.textCode then
                    fail('text-entry insertion assertion')
                    return
                end
                local codes, last = {}, 0
                for i = 1, 15 do
                    codes[i] = bit.band(s16(characters + (i - 1) * 2), 0xffff)
                    if codes[i] ~= 0 and codes[i] ~= 0x8140 then last = i end
                end
                local bytes = {}
                for i = 1, 15 do
                    local code = i <= last and codes[i] or 0
                    bytes[i] = string.char(bit.rshift(code, 8), bit.band(code, 0xff))
                end
                p.textExpected = table.concat(bytes)
                pad.setOverride(buttons.START)
            elseif t == 108 then pad.clearOverride(buttons.START) end
        elseif p.screen == 'commands' then
            if t == 10 or t == 35 or t == 60 then pad.setOverride(buttons.DOWN)
            elseif t == 18 or t == 43 or t == 68 then pad.clearOverride(buttons.DOWN)
            elseif t == 90 then
                if s16(num('COMMAND_SELECTION')) ~= 5 then
                    fail('Save command was not selected')
                    return
                end
                pad.setOverride(buttons.CIRCLE)
            elseif t == 98 then pad.clearOverride(buttons.CIRCLE) end
        elseif p.screen == 'save' then
            local state = s16(saveState)
            if state ~= p.saveState then
                release()
                p.saveState = state
                p.promptTicks = 0
                print('TENCHU_TRIAL save-state=' .. state)
            end
            if state == 10 or state == 70 then
                if state == 70 then p.saved = true end
                p.promptTicks = p.promptTicks + 1
                if p.promptTicks == 30 then pad.setOverride(buttons.CIRCLE)
                elseif p.promptTicks == 38 then pad.clearOverride(buttons.CIRCLE) end
            elseif state >= 20 and state ~= 71 and state ~= 99 then
                fail('unexpected save/error prompt ' .. state)
            end
        elseif p.screen == 'map' then
            if save and p.saved then
                if t >= 15 then
                    local file = assert(io.open(os.getenv('TENCHU_TRIAL_SNAPSHOT'), 'wb'))
                    file:write(ffi.string(ptr(mission), 0x1d3c))
                    file:close()
                    p.done = true
                    release()
                    print(string.format(
                        'TENCHU_TRIAL PASS create/settings/card/editor/save returned-to-map polls=%d vsyncs=%d',
                        p.mapPolls, p.vsyncs))
                    PCSX.quit(0)
                end
                return
            end
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
                if textEntry and (not p.textPassed or
                    ffi.string(ptr(mission + 8), 30) ~= p.textExpected) then
                    fail('text-entry mission-title assertion')
                    return
                end
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
                if save then
                    if t == 100 then pad.setOverride(buttons.SELECT)
                    elseif t == 108 then pad.clearOverride(buttons.SELECT) end
                else
                    p.done = true
                    release()
                    print(string.format(
                        'TENCHU_TRIAL PASS create/settings/card/editor cursor=0->25->0 polls=%d frames=%d vsyncs=%d',
                        p.mapPolls, t, p.vsyncs))
                    PCSX.quit(0)
                end
            end
        end
    end)
    if not ok then fail(tostring(err)) end
end)
print('TENCHU_TRIAL armed')
