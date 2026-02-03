-- Interactive Primitives Test Suite
-- Tests all Interactive methods using assertion helpers
-- Note: This test runs when player opens a container/mob

local test = opengothic.test

opengothic.events.register("onOpen", function(player, interactive)
    test.suite("Interactive Primitives")

    test.assert_not_nil(interactive, "interactive exists")
    test.assert_not_nil(player, "player exists")

    -- String properties
    test.assert_type(interactive:focusName(), "string", "focusName returns string")
    test.assert_type(interactive:schemeName(), "string", "schemeName returns string")
    print("  Interactive: " .. interactive:focusName() .. " (" .. interactive:schemeName() .. ")")

    -- State test
    test.assert_type(interactive:state(), "number", "state returns number")

    -- Boolean type checks
    test.assert_type(interactive:isContainer(), "boolean", "isContainer returns boolean")
    test.assert_type(interactive:isDoor(), "boolean", "isDoor returns boolean")
    test.assert_type(interactive:isLadder(), "boolean", "isLadder returns boolean")
    test.assert_type(interactive:isCracked(), "boolean", "isCracked returns boolean")

    -- Type-specific test
    if interactive:isContainer() then
        print("  Type: Container")
        local inv = interactive:inventory()
        test.assert_not_nil(inv, "container has inventory")
        if inv then
            local items = inv:items()
            test.assert_type(items, "table", "inventory:items() returns table")
            print("  Items in container: " .. #items)
        end
    elseif interactive:isDoor() then
        print("  Type: Door")
        test.assert_type(interactive:isTrueDoor(player), "boolean", "isTrueDoor returns boolean")
    elseif interactive:isLadder() then
        print("  Type: Ladder")
    else
        print("  Type: Other interactive")
    end

    -- needToLockpick test
    test.assert_type(interactive:needToLockpick(player), "boolean", "needToLockpick returns boolean")

    test.summary()

    return false -- Don't block the open action
end)

print("[Test] Interactive Primitives test loaded - runs on open")
