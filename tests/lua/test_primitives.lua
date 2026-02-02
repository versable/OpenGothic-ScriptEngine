-- Test script for Lua primitives

local Attr = opengothic.CONSTANTS.Attribute

local function testNpc(npc)
    print("=== Testing Npc primitives ===")
    print("  displayName:", npc:displayName())
    print("  level:", npc:level())
    print("  hp:", npc:attribute(Attr.ATR_HITPOINTS))
    print("  maxHp:", npc:attribute(Attr.ATR_HITPOINTSMAX))
    print("  mana:", npc:attribute(Attr.ATR_MANA))
    print("  strength:", npc:attribute(Attr.ATR_STRENGTH))
    print("  dexterity:", npc:attribute(Attr.ATR_DEXTERITY))
    print("  experience:", npc:experience())
    print("  guild:", npc:guild())
    print("  isTalking:", npc:isTalking())
    print("  isPlayer:", npc:isPlayer())
    print("  isDead:", npc:isDead())
    print("  isUnconscious:", npc:isUnconscious())
end

local function testInventory(inv)
    print("=== Testing Inventory primitives ===")
    local items = inv:items()
    print("  item count:", #items)
    for i, item in ipairs(items) do
        if i <= 5 then
            print("    -", item:displayName(), "x"..item:count())
        end
    end
    if #items > 5 then
        print("    ... and", #items - 5, "more")
    end
end

local function testItem(item)
    print("=== Testing Item primitives ===")
    print("  displayName:", item:displayName())
    print("  description:", item:description())
    print("  count:", item:count())
    print("  cost:", item:cost())
    print("  clsId:", item:clsId())
    print("  isEquipped:", item:isEquipped())
    print("  isGold:", item:isGold())
    print("  isArmor:", item:isArmor())
end

local function testWorld(world)
    print("=== Testing World primitives ===")
    local hour, minute = world:time()
    print("  time:", hour..":"..string.format("%02d", minute))
end

opengothic.events.register("onItemPickup", function(npc, item)
    testNpc(npc)
    testItem(item)
    testInventory(npc:inventory())
    testWorld(npc:world())
    return false
end)

print("Primitives Test Script Loaded.")
