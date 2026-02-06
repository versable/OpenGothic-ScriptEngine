-- VM Context Restore Test Suite
-- Verifies vm.callWithContext restores SELF context back to its previous value.

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("VM Context Restore")

    local player = opengothic.player()
    test.assert_not_nil(player, "player exists")

    -- Skip gracefully on script setups that do not expose these symbols/functions.
    if not opengothic.vm.getSymbol("SELF") then
        print("[SKIP] VM Context Restore: SELF symbol not available")
        return
    end

    if not opengothic.vm.getSymbol("Npc_IsInState") then
        print("[SKIP] VM Context Restore: Npc_IsInState function not available")
        return
    end

    local beforeSelf = opengothic.daedalus.get("SELF")
    if beforeSelf ~= nil then
        print("[SKIP] VM Context Restore: SELF already set before test, cannot verify nil restoration path")
        return
    end

    local ok, err = pcall(function()
        -- The call itself is not important; the regression is context leakage after it returns.
        opengothic.vm.callWithContext("Npc_IsInState", { self = player }, player, 0)
    end)
    test.assert_true(ok, "vm.callWithContext call succeeded")
    if not ok then
        print("[INFO] vm.callWithContext error: " .. tostring(err))
        test.summary()
        return
    end

    local afterSelf = opengothic.daedalus.get("SELF")
    test.assert_eq(afterSelf, nil, "SELF context restored to nil after vm.callWithContext")

    test.summary()
end)

print("[Test] VM Context Restore test loaded - runs on world load")
