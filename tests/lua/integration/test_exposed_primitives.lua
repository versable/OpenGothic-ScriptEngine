-- Exposed Primitives Test Suite
-- Validates newly exposed scripting primitives/hooks used by high-level helpers

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Exposed Primitives")

    local world = opengothic.world()
    local player = opengothic.player()
    local inv = player and player:inventory() or nil

    -- core type-check primitives
    test.assert_type(opengothic.core.isNpc, "function", "core.isNpc exists")
    test.assert_type(opengothic.core.isInventory, "function", "core.isInventory exists")
    test.assert_type(opengothic.core.isItem, "function", "core.isItem exists")
    test.assert_type(opengothic.core.isWorld, "function", "core.isWorld exists")

    test.assert_eq(opengothic.core.isNpc(player), true, "core.isNpc identifies Npc")
    test.assert_eq(opengothic.core.isInventory(inv), true, "core.isInventory identifies Inventory")
    test.assert_eq(opengothic.core.isWorld(world), true, "core.isWorld identifies World")
    test.assert_eq(opengothic.core.isNpc(nil), false, "core.isNpc rejects nil")

    local weapon = player and player:activeWeapon() or nil
    if weapon ~= nil then
        test.assert_eq(opengothic.core.isItem(weapon), true, "core.isItem identifies Item")
    end

    -- events.unregister contract
    local fired = 0
    local handlerId = opengothic.events.register("onJump", function(_npc)
        fired = fired + 1
        return false
    end)
    test.assert_type(handlerId, "number", "events.register returns handler id")
    test.assert_eq(opengothic.events.unregister("onJump", handlerId), true, "events.unregister removes handler")
    test.assert_eq(opengothic.events.unregister("onJump", handlerId), false, "events.unregister fails for unknown handler")

    -- Manual dispatch should not trigger removed handler
    opengothic._dispatchEvent("onJump", player)
    test.assert_eq(fired, 0, "removed handler does not fire")

    -- Newly exposed primitives used by wrappers
    test.assert_type(world.findNearestNpc, "function", "world:findNearestNpc primitive exists")
    local nearest = world:findNearestNpc(player, 5000)
    test.assert_true(nearest == nil or opengothic.core.isNpc(nearest), "findNearestNpc returns nil or Npc")

    test.assert_type(inv.transferAll, "function", "inventory:transferAll primitive exists")
    test.assert_type(world.isTime, "function", "world:isTime primitive exists")
    test.assert_type(world.findNpcsNear, "function", "world:findNpcsNear primitive exists")
    test.assert_type(world:isTime(0, 0, 23, 59), "boolean", "world:isTime returns boolean")

    local nearList = world:findNpcsNear(player, 5000)
    test.assert_type(nearList, "table", "world:findNpcsNear returns table")

    test.assert_type(player.takeAllFrom, "function", "npc:takeAllFrom primitive exists")
    local transferResult = player:takeAllFrom(inv)
    test.assert_type(transferResult, "table", "npc:takeAllFrom returns table")

    -- onGameMinuteChanged hook is dispatchable in Lua layer
    local minuteHookFired = false
    local minuteHookId = opengothic.events.register("onGameMinuteChanged", function(day, hour, minute)
        minuteHookFired = true
        return false
    end)
    opengothic._dispatchEvent("onGameMinuteChanged", 1, 12, 0)
    test.assert_eq(minuteHookFired, true, "onGameMinuteChanged dispatch reaches Lua handlers")
    opengothic.events.unregister("onGameMinuteChanged", minuteHookId)

    test.summary()
end)

print("[Test] Exposed Primitives test loaded - runs on world load")
