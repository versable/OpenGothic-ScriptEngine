-- Inventory/World Utility Helpers Test Suite
-- Tests high-level opengothic.inventory and opengothic.worldutil wrappers

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Inventory/World Helpers")

    local player = opengothic.player()
    local inv = player and player:inventory() or nil

    test.assert_type(opengothic.inventory, "table", "opengothic.inventory module exists")
    test.assert_type(opengothic.inventory.hasItem, "function", "hasItem exists")
    test.assert_type(opengothic.inventory.transferAll, "function", "transferAll exists")
    test.assert_type(opengothic.inventory.transferByPredicate, "function", "transferByPredicate exists")
    test.assert_type(opengothic.worldutil, "table", "opengothic.worldutil module exists")
    test.assert_type(opengothic.worldutil.findNearestNpc, "function", "findNearestNpc exists")
    test.assert_type(opengothic.worldutil.findNearestItem, "function", "findNearestItem exists")

    -- Invalid argument handling
    local moved, err = opengothic.inventory.transferAll(nil, nil)
    test.assert_type(moved, "table", "transferAll invalid args returns table")
    test.assert_eq(#moved, 0, "transferAll invalid args returns empty list")
    test.assert_type(err, "string", "transferAll invalid args returns error")

    local hasItem = opengothic.inventory.hasItem(nil, "ITFO_APPLE")
    test.assert_eq(hasItem, false, "hasItem rejects nil owner")

    hasItem = opengothic.inventory.hasItem(player, nil)
    test.assert_eq(hasItem, false, "hasItem rejects nil item reference")

    hasItem = opengothic.inventory.hasItem(player, "ITFO_APPLE", 0)
    test.assert_eq(hasItem, false, "hasItem rejects invalid minimum count")

    moved, err = opengothic.inventory.transferByPredicate(player, inv, "not_a_function")
    test.assert_type(moved, "table", "transferByPredicate invalid predicate returns table")
    test.assert_eq(#moved, 0, "transferByPredicate invalid predicate returns empty list")
    test.assert_type(err, "string", "transferByPredicate invalid predicate returns error")

    local nearest = opengothic.worldutil.findNearestNpc(nil, 100)
    test.assert_true(nearest == nil, "findNearestNpc rejects nil origin")

    nearest = opengothic.worldutil.findNearestNpc(player, -1)
    test.assert_true(nearest == nil, "findNearestNpc rejects negative range")

    local nearestItem = opengothic.worldutil.findNearestItem(nil, 100)
    test.assert_true(nearestItem == nil, "findNearestItem rejects nil origin")

    nearestItem = opengothic.worldutil.findNearestItem(player, -1)
    test.assert_true(nearestItem == nil, "findNearestItem rejects negative range")

    -- Valid path: predicate always false -> deterministic no-transfer smoke test
    moved, err = opengothic.inventory.transferByPredicate(player, inv, function(_)
        return false
    end)
    test.assert_type(moved, "table", "transferByPredicate returns moved-items list")
    test.assert_eq(#moved, 0, "always-false predicate results in no transfer")
    test.assert_true(err == nil, "transferByPredicate success returns nil error")

    hasItem = opengothic.inventory.hasItem(player, "ITFO_APPLE")
    test.assert_type(hasItem, "boolean", "hasItem supports symbol-name lookup")

    hasItem = opengothic.inventory.hasItem(inv, opengothic.resolve("ITFO_APPLE"))
    test.assert_type(hasItem, "boolean", "hasItem supports inventory + numeric id")

    hasItem = player:hasItem("ITFO_APPLE")
    test.assert_type(hasItem, "boolean", "Npc:hasItem delegates to inventory.hasItem")

    -- Valid path for nearest lookup; result may be nil depending on nearby NPCs
    nearest = opengothic.worldutil.findNearestNpc(player, 5000)
    test.assert_true(nearest == nil or type(nearest) == "userdata", "findNearestNpc returns nil or Npc userdata")

    local noNearest = opengothic.worldutil.findNearestNpc(player, 5000, function(_)
        return false
    end)
    test.assert_true(noNearest == nil, "predicate can filter out all candidates")

    nearestItem = opengothic.worldutil.findNearestItem(player, 5000)
    test.assert_true(nearestItem == nil or type(nearestItem) == "userdata", "findNearestItem returns nil or Item userdata")

    local noNearestItem = opengothic.worldutil.findNearestItem(player, 5000, function(_)
        return false
    end)
    test.assert_true(noNearestItem == nil, "predicate can filter out all item candidates")

    test.summary()
end)

print("[Test] Inventory/World Helpers test loaded - runs on world load")
