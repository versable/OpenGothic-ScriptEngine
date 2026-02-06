-- Dialog Option Helpers Test Suite
-- Tests high-level wrappers around onDialogOption

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Dialog Option Helpers")

    local player = opengothic.player()
    local world = opengothic.world()

    test.assert_type(opengothic.dialog.onOption, "function", "dialog.onOption exists")
    test.assert_type(opengothic.dialog.offOption, "function", "dialog.offOption exists")
    test.assert_type(opengothic.dialog.blockOption, "function", "dialog.blockOption exists")
    test.assert_type(opengothic.dialog.setOptionTimeWindow, "function", "dialog.setOptionTimeWindow exists")
    test.assert_type(opengothic.dialog.clearOptionTimeWindow, "function", "dialog.clearOptionTimeWindow exists")

    -- Invalid argument handling
    local handlerId, err = opengothic.dialog.onOption(nil)
    test.assert_eq(handlerId, nil, "onOption rejects nil callback")
    test.assert_type(err, "string", "onOption returns error string for invalid callback")
    test.assert_eq(opengothic.dialog.offOption(nil), false, "offOption rejects nil handler id")

    local ok, ruleErr = opengothic.dialog.blockOption(nil, true)
    test.assert_eq(ok, false, "blockOption rejects invalid info name")
    test.assert_type(ruleErr, "string", "blockOption returns error string for invalid info name")

    ok, ruleErr = opengothic.dialog.setOptionTimeWindow("INFO_TEST_WINDOW", 25, 0, 6, 0)
    test.assert_eq(ok, false, "setOptionTimeWindow rejects invalid hours")
    test.assert_type(ruleErr, "string", "setOptionTimeWindow returns error string for invalid window")

    -- onOption/offOption behavior
    local called = 0
    handlerId, err = opengothic.dialog.onOption(function(_npc, _player, infoName)
        if infoName == "INFO_TEST_ONOPTION" then
            called = called + 1
            return true
        end
        return false
    end)
    test.assert_type(handlerId, "number", "onOption returns handler id")
    test.assert_true(err == nil, "onOption returns nil error on success")

    local handled = opengothic._dispatchEvent("onDialogOption", player, player, "INFO_TEST_ONOPTION")
    test.assert_eq(handled, true, "onOption handler can block dialog option")
    test.assert_eq(called, 1, "onOption handler called once")

    test.assert_eq(opengothic.dialog.offOption(handlerId), true, "offOption unregisters handler")
    handled = opengothic._dispatchEvent("onDialogOption", player, player, "INFO_TEST_ONOPTION")
    test.assert_eq(handled, false, "removed onOption handler no longer blocks")

    -- blockOption behavior
    ok, ruleErr = opengothic.dialog.blockOption("INFO_TEST_BLOCKED", true)
    test.assert_eq(ok, true, "blockOption enables blocking")
    test.assert_true(ruleErr == nil, "blockOption returns nil error on success")
    handled = opengothic._dispatchEvent("onDialogOption", player, player, "INFO_TEST_BLOCKED")
    test.assert_eq(handled, true, "blocked option is hidden")

    ok, ruleErr = opengothic.dialog.blockOption("INFO_TEST_BLOCKED", false)
    test.assert_eq(ok, true, "blockOption can disable blocking")
    test.assert_true(ruleErr == nil, "unblock returns nil error on success")
    handled = opengothic._dispatchEvent("onDialogOption", player, player, "INFO_TEST_BLOCKED")
    test.assert_eq(handled, false, "unblocked option is visible")

    -- setOptionTimeWindow / clearOptionTimeWindow behavior
    if world ~= nil then
        ok, ruleErr = opengothic.dialog.setOptionTimeWindow("INFO_TEST_TIME", 20, 0, 23, 0)
        test.assert_eq(ok, true, "setOptionTimeWindow succeeds with valid window")
        test.assert_true(ruleErr == nil, "setOptionTimeWindow returns nil error on success")

        world:setDayTime(21, 0)
        handled = opengothic._dispatchEvent("onDialogOption", player, player, "INFO_TEST_TIME")
        test.assert_eq(handled, false, "option visible inside allowed time window")

        world:setDayTime(10, 0)
        handled = opengothic._dispatchEvent("onDialogOption", player, player, "INFO_TEST_TIME")
        test.assert_eq(handled, true, "option hidden outside allowed time window")

        ok, ruleErr = opengothic.dialog.clearOptionTimeWindow("INFO_TEST_TIME")
        test.assert_eq(ok, true, "clearOptionTimeWindow succeeds")
        test.assert_true(ruleErr == nil, "clearOptionTimeWindow returns nil error on success")
        handled = opengothic._dispatchEvent("onDialogOption", player, player, "INFO_TEST_TIME")
        test.assert_eq(handled, false, "option visible after clearing time window")
    else
        test.assert_true(true, "world is nil, skipped time window checks")
    end

    test.summary()
end)

print("[Test] Dialog Option Helpers test loaded - runs on world load")
