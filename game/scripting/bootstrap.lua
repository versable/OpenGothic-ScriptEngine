-- OpenGothic Lua Bootstrap
-- This file sets up the Lua scripting environment

-- ============================================================
-- Persistent Storage System
-- ============================================================

-- Storage table - modders put persistent data here
opengothic.storage = {}

-- Serialize storage to string map (called from C++)
function opengothic._serializeStorage()
    local result = {}
    for key, value in pairs(opengothic.storage) do
        if type(key) == "string" then
            local t = type(value)
            if t == "string" then
                result[key] = "s:" .. value
            elseif t == "number" then
                result[key] = "n:" .. tostring(value)
            elseif t == "boolean" then
                result[key] = "b:" .. (value and "1" or "0")
            -- Ignore other types (functions, tables, userdata)
            end
        end
    end
    return result
end

-- Deserialize storage from string map (called from C++)
function opengothic._deserializeStorage(data)
    opengothic.storage = {}
    if not data then return end

    for key, encoded in pairs(data) do
        local typeChar = encoded:sub(1, 1)
        local value = encoded:sub(3)

        if typeChar == "s" then
            opengothic.storage[key] = value
        elseif typeChar == "n" then
            opengothic.storage[key] = tonumber(value)
        elseif typeChar == "b" then
            opengothic.storage[key] = (value == "1")
        end
    end
end

-- Event system
opengothic.events = {
    _handlers = {},
    _nextHandlerId = 1
}

function opengothic.events.register(eventName, callback)
    if type(eventName) ~= "string" or type(callback) ~= "function" then
        return nil
    end

    if not opengothic.events._handlers[eventName] then
        opengothic.events._handlers[eventName] = {}
    end

    local handlerId = opengothic.events._nextHandlerId
    opengothic.events._nextHandlerId = handlerId + 1
    table.insert(opengothic.events._handlers[eventName], {
        id = handlerId,
        callback = callback
    })

    return handlerId
end

function opengothic.events.unregister(eventName, handlerId)
    if type(eventName) ~= "string" or type(handlerId) ~= "number" then
        return false
    end

    local handlers = opengothic.events._handlers[eventName]
    if not handlers then
        return false
    end

    for i, entry in ipairs(handlers) do
        if entry.id == handlerId then
            if entry.callback == nil then
                return false
            end
            entry.callback = nil
            return true
        end
    end

    return false
end

-- Called from C++ to dispatch events
function opengothic._dispatchEvent(eventName, ...)
    local handlers = opengothic.events._handlers[eventName]
    if not handlers then
        return false
    end

    for _, entry in ipairs(handlers) do
        if entry.callback ~= nil then
            local handled = entry.callback(...)
            if handled then
                return true
            end
        end
    end
    return false
end

-- Print message to game screen
function opengothic.printMessage(msg)
    opengothic._printMessage(msg)
end

-- Print message at screen position
-- x, y: position in percent (0-100), use -1 to center
-- time: display duration in seconds (default 5)
-- font: font name (default "font_old_10_white.tga")
function opengothic.printScreen(msg, x, y, time, font)
    opengothic._printScreen(msg, x, y, time or 5, font or "font_old_10_white.tga")
end

-- UI helper module (safe high-level wrappers around print primitives)
opengothic.ui = {}

function opengothic.ui.notify(text)
    if type(text) ~= "string" then
        return false, "invalid_text"
    end

    local ok = pcall(function()
        opengothic.printMessage(text)
    end)

    if not ok then
        return false, "notify_failed"
    end

    return true, nil
end

function opengothic.ui.toast(text, seconds, style)
    if type(text) ~= "string" then
        return false, "invalid_text"
    end

    local x = -1
    local y = 85
    local font = nil
    local duration = seconds

    if duration ~= nil and type(duration) ~= "number" then
        return false, "invalid_duration"
    end

    if type(style) == "table" then
        if style.x ~= nil and type(style.x) == "number" then
            x = style.x
        end
        if style.y ~= nil and type(style.y) == "number" then
            y = style.y
        end
        if style.font ~= nil and type(style.font) == "string" then
            font = style.font
        end
    end

    local ok = pcall(function()
        opengothic.printScreen(text, x, y, duration, font)
    end)

    if not ok then
        return false, "toast_failed"
    end

    return true, nil
end

function opengothic.ui.debug(text)
    if text == nil then
        return
    end

    print(tostring(text))
end

-- Timer helper module (safe scheduler on top of onUpdate hook)
opengothic.timer = {
    _nextId = 1,
    _tasks = {},
    _order = {},
    _dirtyOrder = false,
    _updateHookId = nil,
    _minuteHookId = nil
}

local function _timerRunTask(taskId, task)
    local ok, err = pcall(task.fn, taskId)
    if not ok then
        print("[Timer] callback error (" .. tostring(taskId) .. "): " .. tostring(err))
    end
end

local function _timerCompactOrder()
    if not opengothic.timer._dirtyOrder then
        return
    end

    local compact = {}
    for _, taskId in ipairs(opengothic.timer._order) do
        if opengothic.timer._tasks[taskId] ~= nil then
            table.insert(compact, taskId)
        end
    end
    opengothic.timer._order = compact
    opengothic.timer._dirtyOrder = false
end

local function _timerMarkRemoved(taskId)
    if opengothic.timer._tasks[taskId] ~= nil then
        opengothic.timer._tasks[taskId] = nil
        opengothic.timer._dirtyOrder = true
    end
end

local function _timerHasTasks()
    return next(opengothic.timer._tasks) ~= nil
end

local _timerMaybeReleaseHooks

local function _timerOnUpdate(dt)
    if type(dt) ~= "number" or dt < 0 then
        dt = 0
    end

    _timerCompactOrder()

    for _, taskId in ipairs(opengothic.timer._order) do
        local task = opengothic.timer._tasks[taskId]
        if task and task.kind == "realtime" then
            task.remaining = task.remaining - dt

            if task.repeatEvery ~= nil then
                local guard = 0
                while task.remaining <= 0 and opengothic.timer._tasks[taskId] ~= nil and guard < 8 do
                    _timerRunTask(taskId, task)
                    guard = guard + 1
                    if opengothic.timer._tasks[taskId] ~= nil then
                        task.remaining = task.remaining + task.repeatEvery
                    end
                end
            elseif task.remaining <= 0 then
                _timerRunTask(taskId, task)
                _timerMarkRemoved(taskId)
            end
        end
    end

    _timerCompactOrder()
    _timerMaybeReleaseHooks()
    return false
end

local function _timerOnGameMinuteChanged(day, hour, minute)
    _timerCompactOrder()

    for _, taskId in ipairs(opengothic.timer._order) do
        local task = opengothic.timer._tasks[taskId]
        if task and task.kind == "game_minute" then
            _timerRunTask(taskId, task)
        end
    end

    _timerCompactOrder()
    _timerMaybeReleaseHooks()
    return false
end

local function _timerEnsureHooks()
    if opengothic.timer._updateHookId == nil then
        opengothic.timer._updateHookId = opengothic.events.register("onUpdate", _timerOnUpdate)
    end
    if opengothic.timer._minuteHookId == nil then
        opengothic.timer._minuteHookId = opengothic.events.register("onGameMinuteChanged", _timerOnGameMinuteChanged)
    end
end

_timerMaybeReleaseHooks = function()
    if _timerHasTasks() then
        return
    end

    if opengothic.timer._updateHookId ~= nil then
        opengothic.events.unregister("onUpdate", opengothic.timer._updateHookId)
        opengothic.timer._updateHookId = nil
    end
    if opengothic.timer._minuteHookId ~= nil then
        opengothic.events.unregister("onGameMinuteChanged", opengothic.timer._minuteHookId)
        opengothic.timer._minuteHookId = nil
    end
end

local function _timerCreateTask(task)
    local taskId = opengothic.timer._nextId
    opengothic.timer._nextId = taskId + 1
    opengothic.timer._tasks[taskId] = task
    table.insert(opengothic.timer._order, taskId)
    _timerEnsureHooks()
    return taskId
end

function opengothic.timer.after(seconds, fn)
    if type(seconds) ~= "number" or seconds < 0 or type(fn) ~= "function" then
        return nil, "invalid_args"
    end

    local taskId = _timerCreateTask({
        kind = "realtime",
        fn = fn,
        remaining = seconds,
        repeatEvery = nil
    })
    return taskId, nil
end

function opengothic.timer.every(seconds, fn)
    if type(seconds) ~= "number" or seconds <= 0 or type(fn) ~= "function" then
        return nil, "invalid_args"
    end

    local taskId = _timerCreateTask({
        kind = "realtime",
        fn = fn,
        remaining = seconds,
        repeatEvery = seconds
    })
    return taskId, nil
end

function opengothic.timer.everyGameMinute(fn)
    if type(fn) ~= "function" then
        return nil, "invalid_args"
    end

    local taskId = _timerCreateTask({
        kind = "game_minute",
        fn = fn
    })
    return taskId, nil
end

function opengothic.timer.cancel(taskId)
    if type(taskId) ~= "number" then
        return false
    end

    local existed = opengothic.timer._tasks[taskId] ~= nil
    _timerMarkRemoved(taskId)
    _timerCompactOrder()
    _timerMaybeReleaseHooks()
    return existed
end

-- Bridge ergonomics wrappers (non-throwing helpers over bridge primitives)
opengothic.daedalus = opengothic.daedalus or {}
opengothic.vm = opengothic.vm or {}

local function _bridgeIsNpc(value)
    local core = opengothic.core
    if core and type(core.isNpc) == "function" then
        return core.isNpc(value)
    end

    if value == nil then
        return false
    end

    local ok, result = pcall(function()
        return value:isPlayer()
    end)

    return ok and type(result) == "boolean"
end

function opengothic.daedalus.tryCall(funcName, ...)
    if type(funcName) ~= "string" or funcName == "" then
        return false, nil, "invalid_function_name"
    end

    local ok, result = pcall(opengothic.daedalus.call, funcName, ...)
    if not ok then
        return false, nil, tostring(result)
    end

    return true, result, nil
end

function opengothic.daedalus.trySet(symbolName, value, index)
    if type(symbolName) ~= "string" or symbolName == "" then
        return false, "invalid_symbol_name"
    end

    if index ~= nil and type(index) ~= "number" then
        return false, "invalid_index"
    end

    local ok, err = pcall(opengothic.daedalus.set, symbolName, value, index)
    if not ok then
        return false, tostring(err)
    end

    return true, nil
end

function opengothic.daedalus.exists(symbolName)
    if type(symbolName) ~= "string" or symbolName == "" then
        return false
    end

    local vm = opengothic.vm
    if type(vm) ~= "table" or type(vm.getSymbol) ~= "function" then
        return false
    end

    local ok, sym = pcall(vm.getSymbol, symbolName)
    if not ok then
        return false
    end

    return sym ~= nil
end

function opengothic.vm.callContextSafe(funcName, context, ...)
    if type(funcName) ~= "string" or funcName == "" or type(context) ~= "table" then
        return false, nil, "invalid_args"
    end

    local ok, result = pcall(opengothic.vm.callWithContext, funcName, context, ...)
    if not ok then
        return false, nil, tostring(result)
    end

    return true, result, nil
end

function opengothic.vm.callSelf(funcName, selfNpc, ...)
    if not _bridgeIsNpc(selfNpc) then
        return false, nil, "invalid_self"
    end

    return opengothic.vm.callContextSafe(funcName, { self = selfNpc }, ...)
end

function opengothic.vm.callSelfOther(funcName, selfNpc, otherNpc, ...)
    if not _bridgeIsNpc(selfNpc) then
        return false, nil, "invalid_self"
    end

    if not _bridgeIsNpc(otherNpc) then
        return false, nil, "invalid_other"
    end

    return opengothic.vm.callContextSafe(funcName, {
        self = selfNpc,
        other = otherNpc
    }, ...)
end

-- DamageCalculator convenience: combines primitives for common case
function opengothic.DamageCalculator.calculate(attacker, victim, isSpell, spellId)
    local dmg = {}
    for i = 0, 7 do dmg[i] = 0 end

    if isSpell and spellId > 0 then
        local spell = attacker:world():spellDesc(spellId)
        if spell then
            local damageType = spell.damageType
            for i = 0, 7 do
                if bit32.band(damageType, bit32.lshift(1, i)) ~= 0 then
                    dmg[i] = spell.damagePerLevel
                end
            end
        end
    end

    return opengothic.DamageCalculator.damageValue(attacker, victim, isSpell, dmg)
end

-- ============================================================
-- Lua Convenience Methods (using only non-Daedalus primitives)
-- ============================================================

-- Check if NPC matches a specific instance name
function opengothic.Npc:isInstance(name)
    local id = opengothic.resolve(name)
    return id ~= nil and self:instanceId() == id
end

-- Check if NPC is sneaking
function opengothic.Npc:isSneaking()
    return self:walkMode() == opengothic.CONSTANTS.WalkBit.WM_Sneak
end

-- Test Framework
opengothic.test = {
    _passed = 0,
    _failed = 0,
    _current = "unknown"
}

function opengothic.test.suite(name)
    opengothic.test._current = name
    print("=== Test Suite: " .. name .. " ===")
end

function opengothic.test.assert_true(condition, message)
    if condition then
        opengothic.test._passed = opengothic.test._passed + 1
    else
        opengothic.test._failed = opengothic.test._failed + 1
        print("[FAIL] " .. opengothic.test._current .. ": " .. (message or "assertion failed"))
    end
end

function opengothic.test.assert_eq(actual, expected, message)
    if actual == expected then
        opengothic.test._passed = opengothic.test._passed + 1
    else
        opengothic.test._failed = opengothic.test._failed + 1
        print("[FAIL] " .. opengothic.test._current .. ": " .. (message or "expected " .. tostring(expected) .. ", got " .. tostring(actual)))
    end
end

function opengothic.test.assert_not_nil(value, message)
    opengothic.test.assert_true(value ~= nil, message or "expected non-nil value")
end

function opengothic.test.assert_type(value, expectedType, message)
    opengothic.test.assert_eq(type(value), expectedType, message or "type mismatch")
end

function opengothic.test.summary()
    local total = opengothic.test._passed + opengothic.test._failed
    print("=== Test Summary ===")
    print("Passed: " .. opengothic.test._passed .. "/" .. total)
    print("Failed: " .. opengothic.test._failed .. "/" .. total)
    return opengothic.test._failed == 0
end

-- ============================================================
-- High-Level Conveniences
-- ============================================================

-- Quest helpers
opengothic.quest = {}

-- Log topic status constants
opengothic.quest.STATUS = {
    FREE = 0,
    RUNNING = 1,
    SUCCESS = 2,
    FAILED = 3,
    OBSOLETE = 4
}

-- Log topic sections
opengothic.quest.SECTION = {
    MISSIONS = 0,
    NOTES = 1
}

-- Create a new quest topic
function opengothic.quest.create(topicName, section)
    section = section or opengothic.quest.SECTION.MISSIONS
    if type(opengothic._questCreateTopic) ~= "function" then
        print("[quest] create unavailable: missing _questCreateTopic hook")
        return
    end
    opengothic._questCreateTopic(topicName, section)
end

-- Set quest status
function opengothic.quest.setState(topicName, status)
    if type(status) == "string" then
        status = opengothic.quest.STATUS[status:upper()] or 0
    end
    if type(opengothic._questSetTopicStatus) ~= "function" then
        print("[quest] setState unavailable: missing _questSetTopicStatus hook")
        return
    end
    opengothic._questSetTopicStatus(topicName, status)
end

-- Add a log entry to a quest
function opengothic.quest.addEntry(topicName, entryText)
    if type(opengothic._questAddEntry) ~= "function" then
        print("[quest] addEntry unavailable: missing _questAddEntry hook")
        return
    end
    opengothic._questAddEntry(topicName, entryText)
end

-- Dialog/Info helpers
opengothic.dialog = {}
opengothic.dialog._optionRules = {
    blocked = {},
    timeWindows = {},
    hookId = nil
}

local function _dialogIsNpc(value)
    local core = opengothic.core
    if core and type(core.isNpc) == "function" then
        return core.isNpc(value)
    end

    if value == nil then
        return false
    end

    local ok, result = pcall(function()
        return value:isPlayer()
    end)

    return ok and type(result) == "boolean"
end

local function _dialogIsOptionName(infoName)
    return type(infoName) == "string"
        and infoName ~= ""
        and string.sub(infoName, 1, 5) == "INFO_"
end

local function _dialogValidClock(hour, minute)
    return type(hour) == "number"
        and type(minute) == "number"
        and hour >= 0 and hour < 24
        and minute >= 0 and minute < 60
end

local function _dialogHasOptionRules()
    return next(opengothic.dialog._optionRules.blocked) ~= nil
        or next(opengothic.dialog._optionRules.timeWindows) ~= nil
end

local function _dialogOptionRulesHandler(_npc, _player, infoName)
    if not _dialogIsOptionName(infoName) then
        return false
    end

    if opengothic.dialog._optionRules.blocked[infoName] == true then
        return true
    end

    local window = opengothic.dialog._optionRules.timeWindows[infoName]
    if window == nil then
        return false
    end

    local world = opengothic.world()
    if world == nil then
        return false
    end

    local ok, inWindow = pcall(function()
        return world:isTime(window.startH, window.startM, window.endH, window.endM)
    end)
    if not ok then
        return false
    end

    return inWindow ~= true
end

local function _dialogEnsureOptionRulesHook()
    if opengothic.dialog._optionRules.hookId ~= nil then
        return true
    end

    local hookId = opengothic.events.register("onDialogOption", _dialogOptionRulesHandler)
    if type(hookId) ~= "number" then
        return false
    end

    opengothic.dialog._optionRules.hookId = hookId
    return true
end

local function _dialogMaybeReleaseOptionRulesHook()
    if opengothic.dialog._optionRules.hookId == nil then
        return
    end

    if _dialogHasOptionRules() then
        return
    end

    opengothic.events.unregister("onDialogOption", opengothic.dialog._optionRules.hookId)
    opengothic.dialog._optionRules.hookId = nil
end

-- Check if player knows a specific info
function opengothic.dialog.playerKnows(infoName)
    local infoId = opengothic.resolve(infoName)
    if not infoId then return false end
    local player = opengothic.player()
    if not player then return false end
    return opengothic.daedalus.call("Npc_KnowsInfo", player, infoId) ~= 0
end

-- Check if a symbol name resolves to a dialog option symbol
function opengothic.dialog.isOption(infoName)
    if not _dialogIsOptionName(infoName) then
        return false
    end

    return opengothic.resolve(infoName) ~= nil
end

-- Check if NPC appears talkable for dialog flow guards
function opengothic.dialog.canTalkTo(npc)
    if not _dialogIsNpc(npc) then
        return false
    end

    local playerTargetOk, isPlayerTarget = pcall(function()
        return npc:isPlayer()
    end)
    if playerTargetOk and isPlayerTarget then
        return false
    end

    local player = opengothic.player()
    if not _dialogIsNpc(player) then
        return false
    end

    if npc == player then
        return false
    end

    local deadOk, isDead = pcall(function()
        return npc:isDead()
    end)
    if not deadOk or isDead then
        return false
    end

    local downOk, isDown = pcall(function()
        return npc:isDown()
    end)
    if not downOk or isDown then
        return false
    end

    local talkingOk, isTalking = pcall(function()
        return npc:isTalking()
    end)
    if talkingOk and isTalking then
        return false
    end

    return true
end

-- Register a safe onDialogOption callback wrapper
function opengothic.dialog.onOption(callback)
    if type(callback) ~= "function" then
        return nil, "invalid_callback"
    end

    local hookId = opengothic.events.register("onDialogOption", function(npc, player, infoName)
        if not _dialogIsOptionName(infoName) then
            return false
        end

        local ok, blocked = pcall(callback, npc, player, infoName)
        if not ok then
            print("[Dialog] onOption callback error: " .. tostring(blocked))
            return false
        end

        return blocked == true
    end)

    if type(hookId) ~= "number" then
        return nil, "register_failed"
    end

    return hookId, nil
end

-- Unregister a callback previously returned by dialog.onOption
function opengothic.dialog.offOption(handlerId)
    if type(handlerId) ~= "number" then
        return false
    end

    return opengothic.events.unregister("onDialogOption", handlerId)
end

-- Block or unblock a dialog option by name
function opengothic.dialog.blockOption(infoName, blocked)
    if not _dialogIsOptionName(infoName) then
        return false, "invalid_info_name"
    end

    local shouldBlock = blocked
    if shouldBlock == nil then
        shouldBlock = true
    end
    if type(shouldBlock) ~= "boolean" then
        return false, "invalid_blocked_flag"
    end

    if shouldBlock then
        if not _dialogEnsureOptionRulesHook() then
            return false, "register_failed"
        end
        opengothic.dialog._optionRules.blocked[infoName] = true
    else
        opengothic.dialog._optionRules.blocked[infoName] = nil
        _dialogMaybeReleaseOptionRulesHook()
    end

    return true, nil
end

-- Add/update a time window for a dialog option
function opengothic.dialog.setOptionTimeWindow(infoName, startH, startM, endH, endM)
    if not _dialogIsOptionName(infoName) then
        return false, "invalid_info_name"
    end

    if not _dialogValidClock(startH, startM) or not _dialogValidClock(endH, endM) then
        return false, "invalid_time_window"
    end

    if not _dialogEnsureOptionRulesHook() then
        return false, "register_failed"
    end

    opengothic.dialog._optionRules.timeWindows[infoName] = {
        startH = startH,
        startM = startM,
        endH = endH,
        endM = endM
    }

    return true, nil
end

-- Remove a previously registered time window for a dialog option
function opengothic.dialog.clearOptionTimeWindow(infoName)
    if not _dialogIsOptionName(infoName) then
        return false, "invalid_info_name"
    end

    opengothic.dialog._optionRules.timeWindows[infoName] = nil
    _dialogMaybeReleaseOptionRulesHook()
    return true, nil
end

-- NPC convenience methods (extend Npc metatable)

-- Call a Daedalus function with this NPC as SELF
function opengothic.Npc:callDaedalus(funcName, ...)
    return opengothic.vm.callWithContext(funcName, { self = self }, ...)
end

-- Get NPC's current AI state name
function opengothic.Npc:getStateName()
    -- Returns the state function symbol index, not a string
    return opengothic.daedalus.call("Npc_GetStateTime", self)
end

-- Check if NPC is in a specific state
function opengothic.Npc:isInState(stateName)
    local stateId = opengothic.resolve(stateName)
    if not stateId then return false end
    return opengothic.daedalus.call("Npc_IsInState", self, stateId) ~= 0
end

-- Give item to NPC
function opengothic.Npc:giveItem(itemName, count)
    count = count or 1
    local itemId = opengothic.resolve(itemName)
    if not itemId then return false end
    opengothic.daedalus.call("CreateInvItems", self, itemId, count)
    return true
end

-- Remove item from NPC
function opengothic.Npc:removeItem(itemName, count)
    count = count or 1
    local itemId = opengothic.resolve(itemName)
    if not itemId then return 0 end
    return opengothic.daedalus.call("Npc_RemoveInvItems", self, itemId, count)
end

-- Check if NPC has items
function opengothic.Npc:hasItem(itemName, minCount)
    minCount = minCount or 1
    local itemId = opengothic.resolve(itemName)
    if not itemId then return false end
    local count = opengothic.daedalus.call("Npc_HasItems", self, itemId)
    return count >= minCount
end

-- Equip an item
function opengothic.Npc:equipItem(itemName)
    local itemId = opengothic.resolve(itemName)
    if not itemId then return false end
    opengothic.daedalus.call("EquipItem", self, itemId)
    return true
end

-- AI helper module (safe high-level wrappers around NPC primitives)
opengothic.ai = {}

local function _isNpc(value)
    local core = opengothic.core
    if core and type(core.isNpc) == "function" then
        return core.isNpc(value)
    end

    if value == nil then
        return false
    end

    local ok, result = pcall(function()
        return value:isPlayer()
    end)

    return ok and type(result) == "boolean"
end

local function _isInventory(value)
    local core = opengothic.core
    if core and type(core.isInventory) == "function" then
        return core.isInventory(value)
    end

    if value == nil then
        return false
    end

    local ok, result = pcall(function()
        return value:items()
    end)

    return ok and type(result) == "table"
end

local function _isItem(value)
    local core = opengothic.core
    if core and type(core.isItem) == "function" then
        return core.isItem(value)
    end

    if value == nil then
        return false
    end

    local ok, result = pcall(function()
        return value:clsId()
    end)

    return ok and type(result) == "number"
end

function opengothic.ai.attackTarget(npc, target)
    if not _isNpc(npc) or not _isNpc(target) then
        return false, "invalid_npc_or_target"
    end

    local ok = pcall(function()
        npc:setTarget(target)
        npc:attack()
    end)

    if not ok then
        return false, "attack_failed"
    end

    return true, nil
end

function opengothic.ai.flee(npc)
    if not _isNpc(npc) then
        return false, "invalid_npc"
    end

    local ok = pcall(function()
        npc:flee()
    end)

    if not ok then
        return false, "flee_failed"
    end

    return true, nil
end

function opengothic.ai.reset(npc)
    if not _isNpc(npc) then
        return false, "invalid_npc"
    end

    local ok = pcall(function()
        npc:clearAI()
        npc:setTarget(nil)
    end)

    if not ok then
        return false, "reset_failed"
    end

    return true, nil
end

function opengothic.ai.isCombatReady(npc)
    if not _isNpc(npc) then
        return false
    end

    local okDead, dead = pcall(function()
        return npc:isDead()
    end)
    if not okDead or dead then
        return false
    end

    local okDown, down = pcall(function()
        return npc:isDown()
    end)
    if not okDown or down then
        return false
    end

    return true
end

-- Inventory/world utility helper modules (safe wrappers around primitives)
opengothic.inventory = {}
opengothic.worldutil = {}

function opengothic.inventory.transferByPredicate(dstNpc, srcInv, predicate, opts)
    if not _isNpc(dstNpc) or not _isInventory(srcInv) then
        return {}, "invalid_args"
    end

    if predicate == nil then
        return opengothic.inventory.transferAll(dstNpc, srcInv, opts)
    end

    if type(predicate) ~= "function" then
        return {}, "invalid_args"
    end

    if opts ~= nil and type(opts) ~= "table" then
        return {}, "invalid_args"
    end

    local includeEquipped = opts and opts.includeEquipped == true or false
    local includeMission = true
    if opts and opts.includeMission ~= nil then
        includeMission = opts.includeMission == true
    end
    local transferred = {}

    local dstInvOk, dstInv = pcall(function()
        return dstNpc:inventory()
    end)
    local worldOk, world = pcall(function()
        return dstNpc:world()
    end)
    local itemsOk, items = pcall(function()
        return srcInv:items()
    end)

    if not dstInvOk or not worldOk or not itemsOk then
        return {}, "transfer_context_failed"
    end

    for _, item in ipairs(items) do
        local shouldInclude = true

        if not includeEquipped then
            local equippedOk, isEquipped = pcall(function()
                return item:isEquipped()
            end)
            if equippedOk and isEquipped then
                shouldInclude = false
            end
        end

        if shouldInclude and not includeMission then
            local missionOk, isMission = pcall(function()
                return item:isMission()
            end)
            if missionOk and isMission then
                shouldInclude = false
            end
        end

        if shouldInclude and predicate ~= nil then
            local predOk, predResult = pcall(predicate, item)
            if not predOk or predResult ~= true then
                shouldInclude = false
            end
        end

        if shouldInclude then
            local idOk, itemId = pcall(function()
                return item:clsId()
            end)
            local countOk, count = pcall(function()
                return item:count()
            end)

            if idOk and countOk and count > 0 then
                local transferOk, moved = pcall(function()
                    return dstInv:transfer(srcInv, itemId, count, world)
                end)
                if transferOk and moved then
                    local nameOk, name = pcall(function()
                        return item:displayName()
                    end)
                    table.insert(transferred, {
                        id = itemId,
                        count = count,
                        name = nameOk and name or ("item#" .. tostring(itemId))
                    })
                end
            end
        end
    end

    return transferred, nil
end

function opengothic.inventory.transferAll(dstNpc, srcInv, opts)
    if not _isNpc(dstNpc) or not _isInventory(srcInv) then
        return {}, "invalid_args"
    end

    if opts ~= nil and type(opts) ~= "table" then
        return {}, "invalid_args"
    end

    local includeEquipped = opts and opts.includeEquipped == true or false
    local includeMission = true
    if opts and opts.includeMission ~= nil then
        includeMission = opts.includeMission == true
    end

    local dstInvOk, dstInv = pcall(function()
        return dstNpc:inventory()
    end)
    local worldOk, world = pcall(function()
        return dstNpc:world()
    end)
    if not dstInvOk or not worldOk then
        return {}, "transfer_context_failed"
    end

    local ok, result = pcall(function()
        return dstInv:transferAll(srcInv, world, includeEquipped, includeMission)
    end)
    if not ok or type(result) ~= "table" then
        return {}, "transfer_all_failed"
    end

    return result, nil
end

function opengothic.worldutil.findNearestNpc(origin, range, predicate)
    if not _isNpc(origin) or type(range) ~= "number" or range <= 0 then
        return nil
    end

    if predicate ~= nil and type(predicate) ~= "function" then
        return nil
    end

    local worldOk, world = pcall(function()
        return origin:world()
    end)
    if not worldOk then
        return nil
    end

    if predicate == nil then
        local nearestOk, nearest = pcall(function()
            return world:findNearestNpc(origin, range)
        end)
        if nearestOk then
            return nearest
        end
    end

    local listOk, npcs = pcall(function()
        return world:findNpcsNear(origin, range)
    end)
    if not listOk or type(npcs) ~= "table" then
        return nil
    end

    local nearest = nil
    local nearestDist = nil

    for _, npc in ipairs(npcs) do
        if _isNpc(npc) and npc ~= origin then
            local pass = true

            if predicate ~= nil then
                local predOk, predVal = pcall(predicate, npc)
                pass = predOk and predVal == true
            end

            if pass then
                local distOk, dist = pcall(function()
                    return origin:distanceTo(npc)
                end)
                if distOk and type(dist) == "number" and dist >= 0 then
                    if nearestDist == nil or dist < nearestDist then
                        nearest = npc
                        nearestDist = dist
                    end
                end
            end
        end
    end

    return nearest
end

function opengothic.worldutil.findNearestItem(origin, range, predicate)
    if not _isNpc(origin) or type(range) ~= "number" or range <= 0 then
        return nil
    end

    if predicate ~= nil and type(predicate) ~= "function" then
        return nil
    end

    local worldOk, world = pcall(function()
        return origin:world()
    end)
    if not worldOk then
        return nil
    end

    if predicate == nil then
        local nearestOk, nearest = pcall(function()
            return world:findNearestItem(origin, range)
        end)
        if nearestOk then
            return nearest
        end
    end

    local originPosOk, ox, oy, oz = pcall(function()
        return origin:position()
    end)
    if not originPosOk then
        return nil
    end

    local listOk, items = pcall(function()
        return world:detectItemsNear(origin, range)
    end)
    if not listOk or type(items) ~= "table" then
        return nil
    end

    local nearest = nil
    local nearestQDist = nil

    for _, item in ipairs(items) do
        if _isItem(item) then
            local pass = true

            if predicate ~= nil then
                local predOk, predVal = pcall(predicate, item)
                pass = predOk and predVal == true
            end

            if pass then
                local itemPosOk, ix, iy, iz = pcall(function()
                    return item:position()
                end)
                if itemPosOk then
                    local dx = ox - ix
                    local dy = oy - iy
                    local dz = oz - iz
                    local qDist = dx * dx + dy * dy + dz * dz
                    if nearestQDist == nil or qDist < nearestQDist then
                        nearest = item
                        nearestQDist = qDist
                    end
                end
            end
        end
    end

    return nearest
end

-- World convenience methods
function opengothic.World:insertNpc(npcName, waypoint)
    local npcId = opengothic.resolve(npcName)
    if not npcId then return nil end
    opengothic.daedalus.call("Wld_InsertNpc", npcId, waypoint)
    return self:findNpc(npcId)
end

function opengothic.World:insertItem(itemName, waypoint)
    local itemId = opengothic.resolve(itemName)
    if not itemId then return nil end
    opengothic.daedalus.call("Wld_InsertItem", itemId, waypoint)
    return self:findItem(itemId)
end


-- Sound helpers
opengothic.sound = {}

function opengothic.sound.play(soundName)
    opengothic.daedalus.call("Snd_Play", soundName)
end

function opengothic.sound.play3d(npc, soundName)
    opengothic.daedalus.call("Snd_Play3D", npc, soundName)
end

-- Effect helpers
opengothic.effect = {}

function opengothic.effect.play(effectName, source, target)
    target = target or source
    opengothic.daedalus.call("Wld_PlayEffect", effectName, source, target, 0, 0, 0, 0)
end

function opengothic.effect.stop(effectName)
    opengothic.daedalus.call("Wld_StopEffect", effectName)
end
