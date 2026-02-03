-- World Primitives Test Suite
-- Tests all World methods using assertion helpers

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("World Primitives")

    local world = opengothic.player:world()
    test.assert_not_nil(world, "world exists")

    -- Time tests
    local hour, minute = world:time()
    test.assert_type(hour, "number", "time hour is number")
    test.assert_type(minute, "number", "time minute is number")
    test.assert_true(hour >= 0 and hour < 24, "hour in valid range 0-23")
    test.assert_true(minute >= 0 and minute < 60, "minute in valid range 0-59")

    -- Day test
    local day = world:day()
    test.assert_type(day, "number", "day returns number")
    test.assert_true(day >= 0, "day >= 0")

    -- Player access
    local player = world:player()
    test.assert_not_nil(player, "world:player() returns player")
    test.assert_true(player:isPlayer(), "returned npc is player")

    -- Symbol resolution and find tests
    local heroId = opengothic.resolve("PC_HERO")
    if heroId then
        test.assert_type(heroId, "number", "resolve returns number for valid symbol")
        print("  PC_HERO resolved to ID: " .. heroId)
    else
        print("  [INFO] PC_HERO not found - game-specific")
    end

    -- Test findNpc with invalid ID
    local invalidNpc = world:findNpc(999999999)
    test.assert_eq(invalidNpc, nil, "findNpc returns nil for invalid ID")

    -- Test findItem with invalid ID
    local invalidItem = world:findItem(999999999)
    test.assert_eq(invalidItem, nil, "findItem returns nil for invalid ID")

    -- Test findInteractive with invalid ID
    local invalidInteractive = world:findInteractive(999999999)
    test.assert_eq(invalidInteractive, nil, "findInteractive returns nil for invalid ID")

    -- Test resolve with invalid name
    local invalidSymbol = opengothic.resolve("NONEXISTENT_SYMBOL_12345")
    test.assert_eq(invalidSymbol, nil, "resolve returns nil for invalid symbol")

    test.summary()
end)

print("[Test] World Primitives test loaded - runs on world load")
