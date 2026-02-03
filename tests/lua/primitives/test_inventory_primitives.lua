-- Inventory Primitives Test Suite
-- Tests Inventory methods using assertion helpers

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Inventory Primitives")

    local world = opengothic.player:world()
    local player = world:player()
    local inv = player:inventory()

    test.assert_not_nil(inv, "inventory exists")

    -- items() test
    local items = inv:items()
    test.assert_type(items, "table", "items() returns table")
    print("  Player inventory has " .. #items .. " item stacks")

    -- itemCount test (need a valid item ID)
    -- Test with a known item if we have any
    if #items > 0 then
        local firstItem = items[1]
        local clsId = firstItem:clsId()
        local countFromMethod = inv:itemCount(clsId)
        test.assert_type(countFromMethod, "number", "itemCount returns number")
        print("  itemCount for clsId " .. clsId .. ": " .. countFromMethod)
    end

    -- itemCount with invalid ID should return 0
    local invalidCount = inv:itemCount(999999999)
    test.assert_eq(invalidCount, 0, "itemCount returns 0 for invalid item ID")

    test.summary()
end)

print("[Test] Inventory Primitives test loaded - runs on world load")
