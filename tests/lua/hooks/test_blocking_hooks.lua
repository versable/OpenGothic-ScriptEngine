-- Blocking Hooks Test Suite
-- Tests that returning true from hooks actually blocks behavior
-- Manual testing required: trigger actions and verify blocking

local test = opengothic.test
local blockState = {
    dropItem = false,
    useItem = false,
    equip = false,
    pickup = false
}

-- Expose control functions for console testing
opengothic.testBlocking = {
    -- Enable blocking for next action
    blockNextDrop = function()
        blockState.dropItem = true
        print("[Test] Will block next item drop")
    end,
    blockNextUse = function()
        blockState.useItem = true
        print("[Test] Will block next item use")
    end,
    blockNextEquip = function()
        blockState.equip = true
        print("[Test] Will block next equip")
    end,
    blockNextPickup = function()
        blockState.pickup = true
        print("[Test] Will block next item pickup")
    end,
    -- Status
    status = function()
        print("Blocking state:")
        print("  dropItem: " .. tostring(blockState.dropItem))
        print("  useItem: " .. tostring(blockState.useItem))
        print("  equip: " .. tostring(blockState.equip))
        print("  pickup: " .. tostring(blockState.pickup))
    end
}

-- Hook: onDropItem
opengothic.events.register("onDropItem", function(npc, itemId, count)
    if blockState.dropItem then
        print("[Test] BLOCKING item drop - itemId: " .. itemId .. ", count: " .. count)
        blockState.dropItem = false
        return true -- Block the drop
    end
    return false
end)

-- Hook: onUseItem
opengothic.events.register("onUseItem", function(npc, item)
    if blockState.useItem then
        print("[Test] BLOCKING item use - " .. item:displayName())
        blockState.useItem = false
        return true -- Block the use
    end
    return false
end)

-- Hook: onEquip
opengothic.events.register("onEquip", function(npc, item)
    if blockState.equip then
        print("[Test] BLOCKING equip - " .. item:displayName())
        blockState.equip = false
        return true -- Block equip
    end
    return false
end)

-- Hook: onItemPickup
opengothic.events.register("onItemPickup", function(npc, item)
    if blockState.pickup then
        print("[Test] BLOCKING item pickup - " .. item:displayName())
        blockState.pickup = false
        return true -- Block pickup
    end
    return false
end)

print("[Test] Blocking Hooks test loaded")
print("  Use opengothic.testBlocking.blockNextDrop() etc. from console")
print("  Then perform action to verify blocking works")
