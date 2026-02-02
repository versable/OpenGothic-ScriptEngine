-- Test script for item-related hooks
-- All hooks return false to allow default behavior

opengothic.events.register("onItemPickup", function(npc, item)
    print("onItemPickup", npc:displayName(), item:displayName(), "x"..item:count())
    return false
end)

opengothic.events.register("onUseItem", function(npc, item)
    print("onUseItem", npc:displayName(), item:displayName())
    return false
end)

opengothic.events.register("onEquip", function(npc, item)
    print("onEquip", npc:displayName(), item:displayName())
    return false
end)

opengothic.events.register("onUnequip", function(npc, item)
    print("onUnequip", npc:displayName(), item:displayName())
    return false
end)

opengothic.events.register("onDropItem", function(npc, itemId, count)
    print("onDropItem", npc:displayName(), "itemId:"..itemId, "x"..count)
    return false
end)

print("Item Hooks Test Script Loaded.")
