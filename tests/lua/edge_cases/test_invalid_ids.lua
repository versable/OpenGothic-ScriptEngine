-- Invalid IDs Test Suite
-- Tests behavior with edge case ID values

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Invalid IDs")

    local world = opengothic.player:world()
    local player = world:player()

    -- Test very large IDs
    local largeId = 2147483647 -- INT_MAX
    test.assert_eq(world:findNpc(largeId), nil, "findNpc handles INT_MAX")
    test.assert_eq(world:findItem(largeId), nil, "findItem handles INT_MAX")
    test.assert_eq(world:findInteractive(largeId), nil, "findInteractive handles INT_MAX")

    -- Test zero ID
    local zeroNpc = world:findNpc(0)
    -- Zero might be valid in some cases (first symbol), so just check type
    test.assert_true(zeroNpc == nil or type(zeroNpc) == "userdata", "findNpc(0) returns nil or valid npc")

    local zeroItem = world:findItem(0)
    test.assert_true(zeroItem == nil or type(zeroItem) == "userdata", "findItem(0) returns nil or valid item")

    -- Test with player attribute access with invalid attribute IDs
    local Attr = opengothic.CONSTANTS.Attribute
    local invalidAttr = player:attribute(999)
    test.assert_eq(invalidAttr, 0, "attribute with invalid ID returns 0")

    -- Test protection with invalid ID
    local invalidProt = player:protection(999)
    test.assert_eq(invalidProt, 0, "protection with invalid ID returns 0")

    -- Test talent with invalid ID
    local invalidTalent = player:talentSkill(999)
    test.assert_eq(invalidTalent, 0, "talentSkill with invalid ID returns 0")

    local invalidTalentVal = player:talentValue(999)
    test.assert_eq(invalidTalentVal, 0, "talentValue with invalid ID returns 0")

    local invalidHitChance = player:hitChance(999)
    test.assert_eq(invalidHitChance, 0, "hitChance with invalid ID returns 0")

    -- Test inventory itemCount with boundary values
    local inv = player:inventory()
    test.assert_eq(inv:itemCount(0), 0, "itemCount(0) handles edge case")
    test.assert_eq(inv:itemCount(largeId), 0, "itemCount(INT_MAX) handles edge case")

    test.summary()
end)

print("[Test] Invalid IDs test loaded - runs on world load")
