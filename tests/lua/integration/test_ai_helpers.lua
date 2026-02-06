-- AI Helpers Test Suite
-- Tests high-level opengothic.ai convenience wrappers

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("AI Helpers")

    local player = opengothic.player()

    test.assert_type(opengothic.ai, "table", "opengothic.ai module exists")
    test.assert_type(opengothic.ai.attackTarget, "function", "attackTarget exists")
    test.assert_type(opengothic.ai.flee, "function", "flee exists")
    test.assert_type(opengothic.ai.reset, "function", "reset exists")
    test.assert_type(opengothic.ai.isCombatReady, "function", "isCombatReady exists")

    -- Invalid argument handling
    local ok, err = opengothic.ai.attackTarget(nil, nil)
    test.assert_eq(ok, false, "attackTarget rejects nil args")
    test.assert_type(err, "string", "attackTarget returns error string")

    ok, err = opengothic.ai.attackTarget(player, nil)
    test.assert_eq(ok, false, "attackTarget rejects nil target")
    test.assert_type(err, "string", "attackTarget invalid target error string")

    ok, err = opengothic.ai.flee(nil)
    test.assert_eq(ok, false, "flee rejects nil npc")
    test.assert_type(err, "string", "flee returns error string")

    ok, err = opengothic.ai.reset(nil)
    test.assert_eq(ok, false, "reset rejects nil npc")
    test.assert_type(err, "string", "reset returns error string")

    -- Valid path with minimal side effects
    ok, err = opengothic.ai.reset(player)
    test.assert_eq(ok, true, "reset succeeds for player")
    test.assert_true(err == nil, "reset returns nil error on success")

    test.assert_type(opengothic.ai.isCombatReady(nil), "boolean", "isCombatReady handles nil safely")
    test.assert_type(opengothic.ai.isCombatReady(player), "boolean", "isCombatReady returns boolean for player")

    test.summary()
end)

print("[Test] AI Helpers test loaded - runs on world load")
