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
                     'COMMANDS', 'EDITOR_SAVE', 'CARD_SAVE', 'TEXT', 'TEXT_INPUT',
                     'RUN_MISSION', 'ACTIVATE', 'TIMER', 'NOTICE', 'PREVIEW', 'DRAW_CHUNKS',
                     'CHARACTERS'}) do
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
              buttons.SELECT, buttons.L2, buttons.R2, buttons.CROSS, buttons.L1, buttons.UP}
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
local playtest, missionEnd = num('PLAYTEST') ~= 0, num('RUN_MISSION_END')
local preview, previewEnd = num('PREVIEW_ENABLED') ~= 0, num('PREVIEW_END')
local characters, charactersEnd = num('CHARACTERS_ENABLED') ~= 0, num('CHARACTERS_END')
local view, viewAngle = num('VIEW'), num('VIEW_ANGLE')
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
        print(string.format(
            'TENCHU_TRIAL text-entry latin=%04x kanji=%04x deletion/paging/index-return/export=verified',
            p.textCode, p.kanjiCode))
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
    if characters and p.charactersPassed and not p.charactersReturned then
        p.charactersReturned = true
        print('TENCHU_TRIAL characters returned-to-terrain=verified')
    end
    p.entryPolls = 0
    screen('map')
end)
hook('CHARACTERS', function()
    if not characters then return end
    p.charactersEntered = true
    p.characterPolls = 0
    -- Theme zero uses StageAppearance[1]; its third entry is the named boss.
    p.bossKind = s16(u32(num('ROSTER') + 4) + 4)
    screen('characters')
end)
hook('PREVIEW', function()
    if not preview then return end
    p.previewEntered = true
    p.previewPolls, p.chunkDraws = 0, 0
    p.angleBeforePreview = u32(viewAngle)
    screen('preview')
    -- screen() releases input; keep L1 held across the preview's first poll.
    pad.setOverride(buttons.L1)
end)
hook('DRAW_CHUNKS', function()
    if p.screen == 'preview' then p.chunkDraws = p.chunkDraws + 1 end
end)
hook('COMMANDS', function()
    if not (save or playtest or characters) or p.screen == 'commands' then return end
    if bit.band(tonumber(PCSX.getRegisters().GPR.n.a0), 0xffff) ~= 0 then
        if characters and not p.charactersEntered then
            p.commandTarget = 2
        elseif characters and p.charactersPassed and not p.charactersReturned then
            p.commandTarget = 1
        else
            p.commandTarget = playtest and not p.playtestEntered and 4 or 5
        end
        screen('commands')
        ready()
    end
end)
hook('RUN_MISSION', function()
    if not playtest then return end
    p.playtestEntered = true
    p.missionPolls, p.activationChecks, p.timerDraws, p.noticeDraws = 0, 0, 0, 0
    screen('mission')
end)
hook('ACTIVATE', function()
    if p.screen == 'mission' then p.activationChecks = p.activationChecks + 1 end
end)
hook('TIMER', function()
    if p.screen == 'mission' then p.timerDraws = p.timerDraws + 1 end
end)
hook('NOTICE', function()
    if p.screen == 'mission' and bit.band(tonumber(PCSX.getRegisters().GPR.n.a0), 0xffff) == 0xffff then
        p.noticeDraws = p.noticeDraws + 1
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
    if p.screen == 'preview' and ra >= addr.MAP and ra < mapEnd then
        if not p.previewExitRequested or u32(viewAngle) == p.angleBeforePreview then
            fail('preview return/saved-angle assertion')
            return
        end
        p.previewPassed = true
        print(string.format(
            'TENCHU_TRIAL preview rotation/height/return=verified polls=%d chunks=%d angle=%d',
            p.previewPolls, p.chunkDraws, bit.tobit(u32(viewAngle))))
        screen('map')
        p.entryPolls = 0
    end
    if p.screen == 'map' and ra >= addr.MAP and ra < mapEnd then
        p.mapPolls = p.mapPolls + 1
        p.entryPolls = p.entryPolls + 1
        if p.entryPolls >= 2 then ready() end
    elseif p.screen == 'mission' and ra >= addr.RUN_MISSION and ra < missionEnd then
        p.missionPolls = p.missionPolls + 1
        if p.missionPolls >= 2 then ready() end
    elseif p.screen == 'preview' and ra >= addr.PREVIEW and ra < previewEnd then
        p.previewPolls = p.previewPolls + 1
        if p.previewPolls >= 2 then ready() end
    elseif p.screen == 'characters' and ra >= addr.CHARACTERS and ra < charactersEnd then
        p.characterPolls = p.characterPolls + 1
        if p.characterPolls >= 2 then ready() end
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
                p.beforeDelete = {}
                for i = 1, 15 do
                    p.beforeDelete[i] = bit.band(s16(characters + (i - 1) * 2), 0xffff)
                end
                pad.setOverride(buttons.CROSS)
            elseif t == 108 then pad.clearOverride(buttons.CROSS)
            elseif t == 130 then
                local characters = u32(edited + 8)
                if s16(edited) ~= 0 then
                    fail('text-entry deletion cursor assertion')
                    return
                end
                for i = 1, 15 do
                    local expected = i < 15 and p.beforeDelete[i + 1] or 0x8140
                    if bit.band(s16(characters + (i - 1) * 2), 0xffff) ~= expected then
                        fail('text-entry deletion shift assertion')
                        return
                    end
                end
                pad.setOverride(buttons.CIRCLE)
            elseif t == 138 then pad.clearOverride(buttons.CIRCLE)
            elseif t == 160 then
                if s16(edited) ~= 1 or bit.band(s16(u32(edited + 8)), 0xffff) ~= p.textCode then
                    fail('text-entry reinsertion assertion')
                    return
                end
                pad.setOverride(buttons.R2)
            elseif t == 168 or t == 193 or t == 218 then pad.clearOverride(buttons.R2)
            elseif t == 185 or t == 210 then pad.setOverride(buttons.R2)
            elseif t == 240 then
                if s16(keyboard + 4) ~= 3 then
                    fail('text-entry kanji-index category assertion')
                    return
                end
                -- The second index column begins range 5, which has enough
                -- columns to exercise scrolling beyond the 13-column window.
                pad.setOverride(buttons.LEFT)
            elseif t == 248 then pad.clearOverride(buttons.LEFT)
            elseif t == 270 then
                if s16(keyboard) ~= 1 or s16(keyboard + 2) ~= 0 then
                    fail('text-entry kanji-index position assertion')
                    return
                end
                pad.setOverride(buttons.CIRCLE)
            elseif t == 278 then pad.clearOverride(buttons.CIRCLE)
            elseif t == 300 then
                if s16(keyboard + 4) ~= 5 or s16(num('KANJI_SELECTION') + 10) ~= 1 then
                    fail('text-entry kanji range assertion')
                    return
                end
            elseif t == 310 then pad.setOverride(buttons.LEFT)
            elseif t == 390 then pad.clearOverride(buttons.LEFT)
            elseif t == 420 then
                if s16(num('TEXT_PAGES') + 5 * 28 + 8) <= 0 then
                    fail('text-entry kanji paging assertion')
                    return
                end
                p.kanjiCode = bit.band(s16(u32(keyboard + 12)), 0xffff)
                if p.kanjiCode == 0 or p.kanjiCode == 0x8140 then
                    fail('text-entry selected a blank kanji')
                    return
                end
                pad.setOverride(buttons.CIRCLE)
            elseif t == 428 then pad.clearOverride(buttons.CIRCLE)
            elseif t == 450 then
                local characters = u32(edited + 8)
                if s16(keyboard + 4) ~= 3 or s16(keyboard) ~= 1 or s16(keyboard + 2) ~= 0
                    or s16(num('KANJI_SELECTION') + 10) ~= 0 then
                    fail('text-entry kanji index-return assertion')
                    return
                end
                if s16(edited) ~= 2 or bit.band(s16(characters), 0xffff) ~= p.textCode
                    or bit.band(s16(characters + 2), 0xffff) ~= p.kanjiCode then
                    fail('text-entry kanji insertion assertion')
                    return
                end
            elseif t == 480 then
                local characters = u32(edited + 8)
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
            elseif t == 488 then pad.clearOverride(buttons.START) end
        elseif p.screen == 'preview' then
            if t == 1 then
                p.previewEye = ffi.string(ptr(view), 12)
                p.previewLookX = u32(view + 12)
                p.previewLookY = bit.tobit(u32(view + 16))
            elseif t == 10 then pad.setOverride(buttons.RIGHT)
            elseif t == 30 then pad.clearOverride(buttons.RIGHT)
            elseif t == 35 then pad.setOverride(buttons.UP)
            elseif t == 50 then pad.clearOverride(buttons.UP)
            elseif t == 70 then
                if p.previewPolls < 20 or p.chunkDraws < 20 or
                    ffi.string(ptr(view), 12) ~= p.previewEye or
                    u32(view + 12) == p.previewLookX or
                    bit.tobit(u32(view + 16)) <= p.previewLookY then
                    fail('preview input/render/rotation/height assertion')
                    return
                end
            elseif t == 80 then
                p.previewExitRequested = true
                pad.clearOverride(buttons.L1)
            end
        elseif p.screen == 'commands' then
            local command = p.commandTarget
            if t == 10 or t == 35 or t == 60 then
                local selection = s16(num('COMMAND_SELECTION'))
                if selection ~= command then
                    pad.setOverride(selection < command and buttons.DOWN or buttons.UP)
                end
            elseif t == 18 or t == 43 or t == 68 then
                pad.clearOverride(buttons.DOWN)
                pad.clearOverride(buttons.UP)
            elseif t == 90 then
                if s16(num('COMMAND_SELECTION')) ~= command then
                    fail('expected editor command ' .. command .. ' was not selected')
                    return
                end
                pad.setOverride(buttons.CIRCLE)
            elseif t == 98 then pad.clearOverride(buttons.CIRCLE) end
        elseif p.screen == 'characters' then
            local player = mission + 0x8ec
            local boss = player + 2 * 0x208
            if t == 10 or t == 35 then pad.setOverride(buttons.DOWN)
            elseif t == 18 or t == 43 then pad.clearOverride(buttons.DOWN)
            elseif t == 60 then
                if s16(cursor + 4) ~= 0 or s16(cursor + 6) ~= 50 then
                    fail('character cursor/player-cell assertion')
                    return
                end
                pad.setOverride(buttons.CROSS)
            elseif t == 68 or t == 268 then pad.clearOverride(buttons.CROSS)
            elseif t == 85 then
                if s16(player) ~= 0 or s16(player + 2) ~= 16 or s16(player + 4) ~= 18 then
                    fail('character objective deletion protection assertion')
                    return
                end
                pad.setOverride(buttons.UP)
            elseif t == 93 or t == 118 then pad.clearOverride(buttons.UP)
            elseif t == 110 then pad.setOverride(buttons.UP)
            elseif t == 135 or t == 160 then pad.setOverride(buttons.R2)
            elseif t == 143 or t == 168 then pad.clearOverride(buttons.R2)
            elseif t == 185 then
                if s16(cursor + 4) ~= 0 or s16(cursor + 6) ~= 0 or
                    s16(cursor + 0x24 + 4) ~= 82 or bit.band(p.bossKind, 0xf0) ~= 0x80 then
                    fail('character palette/boss selection assertion')
                    return
                end
                pad.setOverride(buttons.CIRCLE)
            elseif t == 193 or t == 243 then pad.clearOverride(buttons.CIRCLE)
            elseif t == 210 then
                if s16(boss) ~= p.bossKind or s16(boss + 2) ~= 16 or
                    s16(boss + 4) ~= 16 or s16(boss + 6) ~= 0 then
                    fail('character boss placement assertion')
                    return
                end
                pad.setOverride(buttons.RIGHT)
            elseif t == 218 then pad.clearOverride(buttons.RIGHT)
            elseif t == 235 then pad.setOverride(buttons.CIRCLE)
            elseif t == 260 then
                local count = 0
                for i = 0, 9 do
                    if s16(player + i * 0x208) == p.bossKind then count = count + 1 end
                end
                if count ~= 1 or s16(boss + 2) ~= 17 or s16(boss + 4) ~= 16 then
                    fail('character unique boss movement assertion')
                    return
                end
                pad.setOverride(buttons.CROSS)
            elseif t == 285 then
                if s16(boss) ~= -1 or p.characterPolls < 100 or
                    s16(player) ~= 0 or s16(player + 2) ~= 16 or s16(player + 4) ~= 18 then
                    fail('character boss deletion/player preservation assertion')
                    return
                end
                p.charactersPassed = true
                print(string.format(
                    'TENCHU_TRIAL characters objective-protection/boss-place/move/delete=verified polls=%d kind=%02x',
                    p.characterPolls, p.bossKind))
                pad.setOverride(buttons.SELECT)
            elseif t == 293 then pad.clearOverride(buttons.SELECT) end
        elseif p.screen == 'mission' then
            if t == 160 then
                p.playtestExitRequested = true
                pad.setOverride(buttons.SELECT)
                pad.setOverride(buttons.START)
            elseif t == 172 then release() end
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
            if playtest and p.playtestEntered then
                if t >= 15 and not p.playtestPassed then
                    if not p.playtestExitRequested or p.missionPolls < 30 or
                        p.activationChecks < 2 or p.timerDraws < 20 or p.noticeDraws < 20 then
                        fail('playtest actor/HUD/input/return assertion')
                        return
                    end
                    p.playtestPassed = true
                    print(string.format(
                        'TENCHU_TRIAL playtest returned-to-map polls=%d activation=%d timer=%d notices=%d',
                        p.missionPolls, p.activationChecks, p.timerDraws, p.noticeDraws))
                end
                if p.playtestPassed then
                    if save then
                        if t == 30 then pad.setOverride(buttons.SELECT)
                        elseif t == 38 then pad.clearOverride(buttons.SELECT) end
                    else
                        p.done = true
                        release()
                        print('TENCHU_TRIAL PASS create/settings/card/editor/playtest returned-to-map vsyncs=' .. p.vsyncs)
                        PCSX.quit(0)
                    end
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
                if preview and not p.previewPassed then
                    if t == 100 then pad.setOverride(buttons.L1) end
                elseif save or playtest or (characters and not p.charactersReturned) then
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
