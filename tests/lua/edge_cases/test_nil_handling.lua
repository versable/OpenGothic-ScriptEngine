-- Nil Handling Test Suite
-- Tests that API correctly returns nil for invalid inputs

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Nil Handling")

    local world = opengothic.player:world()

    -- Invalid symbol lookup
    local badId = opengothic.resolve("NONEXISTENT_NPC_12345")
    test.assert_eq(badId, nil, "resolve returns nil for invalid name")

    local anotherBad = opengothic.resolve("THIS_SYMBOL_DOES_NOT_EXIST")
    test.assert_eq(anotherBad, nil, "resolve returns nil for another invalid name")

    -- Find non-existent NPC
    local badNpc = world:findNpc(999999999)
    test.assert_eq(badNpc, nil, "findNpc returns nil for invalid ID")

    -- Find non-existent item
    local badItem = world:findItem(999999999)
    test.assert_eq(badItem, nil, "findItem returns nil for invalid ID")

    -- Find non-existent interactive
    local badInteractive = world:findInteractive(999999999)
    test.assert_eq(badInteractive, nil, "findInteractive returns nil for invalid ID")

    -- Test multiple invalid symbol resolutions
    local symbols = {"FAKE_NPC_1", "FAKE_ITEM_2", "NOT_A_SYMBOL", ""}
    for _, sym in ipairs(symbols) do
        local result = opengothic.resolve(sym)
        test.assert_eq(result, nil, "resolve('" .. sym .. "') returns nil")
    end

    test.summary()
end)

print("[Test] Nil Handling test loaded - runs on world load")
