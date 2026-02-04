-- OpenGothic Lua Bootstrap
-- This file sets up the Lua scripting environment

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
        if not item.equipped then
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
