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

-- Extend Npc with convenience methods
function opengothic.Npc:takeAllFrom(srcInv)
    local items = srcInv:items()
    local transferred = {}
    local dstInv = self:inventory()
    local world = self:world()
    for _, item in ipairs(items) do
        if not item.equipped then
            dstInv:transfer(srcInv, item.id, item.count, world)
            table.insert(transferred, {name = item.name, count = item.count})
        end
    end
    return transferred
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
