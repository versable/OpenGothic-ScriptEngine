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
    _handlers = {}
}

function opengothic.events.register(eventName, callback)
    if not opengothic.events._handlers[eventName] then
        opengothic.events._handlers[eventName] = {}
    end
    table.insert(opengothic.events._handlers[eventName], callback)
end

-- Called from C++ to dispatch events
function opengothic._dispatchEvent(eventName, ...)
    local handlers = opengothic.events._handlers[eventName]
    if not handlers then
        return false
    end

    for _, handler in ipairs(handlers) do
        local handled = handler(...)
        if handled then
            return true
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
    _hookRegistered = false,
    _lastMinuteStamp = nil
}

local function _timerRunTask(taskId, task)
    local ok, err = pcall(task.fn, taskId)
    if not ok then
        print("[Timer] callback error (" .. tostring(taskId) .. "): " .. tostring(err))
    end
end

local function _timerOnUpdate(dt)
    if type(dt) ~= "number" or dt < 0 then
        dt = 0
    end

    -- Realtime timers
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
                opengothic.timer._tasks[taskId] = nil
            end
        end
    end

    -- Game-minute timers
    local world = opengothic.world()
    if not world then
        opengothic.timer._lastMinuteStamp = nil
        return false
    end

    local hour, minute = world:time()
    if type(hour) ~= "number" or type(minute) ~= "number" then
        return false
    end

    local stamp = hour * 60 + minute
    if opengothic.timer._lastMinuteStamp == nil then
        opengothic.timer._lastMinuteStamp = stamp
        return false
    end

    if stamp ~= opengothic.timer._lastMinuteStamp then
        opengothic.timer._lastMinuteStamp = stamp
        for _, taskId in ipairs(opengothic.timer._order) do
            local task = opengothic.timer._tasks[taskId]
            if task and task.kind == "game_minute" then
                _timerRunTask(taskId, task)
            end
        end
    end

    return false
end

local function _timerEnsureHook()
    if opengothic.timer._hookRegistered then
        return
    end

    opengothic.events.register("onUpdate", _timerOnUpdate)
    opengothic.timer._hookRegistered = true
end

local function _timerCreateTask(task)
    local taskId = opengothic.timer._nextId
    opengothic.timer._nextId = taskId + 1
    opengothic.timer._tasks[taskId] = task
    table.insert(opengothic.timer._order, taskId)
    _timerEnsureHook()
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

    if opengothic.timer._tasks[taskId] ~= nil then
        opengothic.timer._tasks[taskId] = nil
        return true
    end

    return false
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

-- Transfer all non-equipped items from source inventory to this NPC
function opengothic.Npc:takeAllFrom(srcInv)
    local items = srcInv:items()
    local transferred = {}
    local dstInv = self:inventory()
    local world = self:world()
    for _, item in ipairs(items) do
        if not item:isEquipped() then
            dstInv:transfer(srcInv, item:clsId(), item:count(), world)
            table.insert(transferred, {name = item:displayName(), count = item:count()})
        end
    end
    return transferred
end

-- Check if current time is within a range (handles overnight ranges like 20:00 to 6:00)
function opengothic.World:isTime(startHour, startMin, endHour, endMin)
    local h, m = self:time()
    local current = h * 60 + m
    local startTime = startHour * 60 + startMin
    local endTime = endHour * 60 + endMin

    if startTime <= endTime then
        -- Normal range (e.g., 8:00 to 18:00)
        return current >= startTime and current < endTime
    else
        -- Overnight range (e.g., 20:00 to 6:00)
        return current >= startTime or current < endTime
    end
end

-- Find NPCs near another NPC (convenience wrapper for findNpcsInRange)
function opengothic.World:findNpcsNear(npc, range)
    local x, y, z = npc:position()
    return self:findNpcsInRange(x, y, z, range)
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
-- High-Level Conveniences (built on Daedalus bridge)
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
    opengothic.daedalus.call("Log_CreateTopic", topicName, section)
end

-- Set quest status
function opengothic.quest.setState(topicName, status)
    if type(status) == "string" then
        status = opengothic.quest.STATUS[status:upper()] or 0
    end
    opengothic.daedalus.call("Log_SetTopicStatus", topicName, status)
end

-- Add a log entry to a quest
function opengothic.quest.addEntry(topicName, entryText)
    opengothic.daedalus.call("Log_AddEntry", topicName, entryText)
end

-- Dialog/Info helpers
opengothic.dialog = {}

-- Check if player knows a specific info
function opengothic.dialog.playerKnows(infoName)
    local infoId = opengothic.resolve(infoName)
    if not infoId then return false end
    local player = opengothic.player()
    if not player then return false end
    return opengothic.daedalus.call("Npc_KnowsInfo", player, infoId) ~= 0
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
    if value == nil then
        return false
    end

    local ok, result = pcall(function()
        return value:isPlayer()
    end)

    return ok and type(result) == "boolean"
end

local function _isInventory(value)
    if value == nil then
        return false
    end

    local ok, result = pcall(function()
        return value:items()
    end)

    return ok and type(result) == "table"
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

    if predicate ~= nil and type(predicate) ~= "function" then
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
    return opengothic.inventory.transferByPredicate(dstNpc, srcInv, nil, opts)
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
