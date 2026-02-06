-- Daedalus Bridge Helpers Test Suite
-- Tests non-throwing convenience wrappers over opengothic.daedalus/opengothic.vm

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Daedalus Bridge Helpers")

    local player = opengothic.player()

    test.assert_type(opengothic.daedalus, "table", "opengothic.daedalus table exists")
    test.assert_type(opengothic.vm, "table", "opengothic.vm table exists")

    test.assert_type(opengothic.daedalus.tryCall, "function", "daedalus.tryCall exists")
    test.assert_type(opengothic.daedalus.trySet, "function", "daedalus.trySet exists")
    test.assert_type(opengothic.daedalus.exists, "function", "daedalus.exists exists")

    test.assert_type(opengothic.vm.callContextSafe, "function", "vm.callContextSafe exists")
    test.assert_type(opengothic.vm.callSelf, "function", "vm.callSelf exists")
    test.assert_type(opengothic.vm.callSelfOther, "function", "vm.callSelfOther exists")

    local ok, result, err = opengothic.daedalus.tryCall(nil)
    test.assert_eq(ok, false, "tryCall rejects invalid function name")
    test.assert_eq(result, nil, "tryCall invalid args returns nil result")
    test.assert_type(err, "string", "tryCall invalid args returns error string")

    ok, result, err = opengothic.daedalus.tryCall("__OG_DOES_NOT_EXIST__")
    test.assert_eq(ok, false, "tryCall returns false for missing function")
    test.assert_eq(result, nil, "tryCall missing function returns nil result")
    test.assert_type(err, "string", "tryCall missing function returns error string")

    local setOk, setErr = opengothic.daedalus.trySet(nil, 1)
    test.assert_eq(setOk, false, "trySet rejects invalid symbol name")
    test.assert_type(setErr, "string", "trySet invalid symbol returns error string")

    setOk, setErr = opengothic.daedalus.trySet("PLAYER_CHAPTER", 1, "x")
    test.assert_eq(setOk, false, "trySet rejects invalid index type")
    test.assert_type(setErr, "string", "trySet invalid index returns error string")

    setOk, setErr = opengothic.daedalus.trySet("__OG_DOES_NOT_EXIST__", 1)
    test.assert_eq(setOk, false, "trySet returns false for missing symbol")
    test.assert_type(setErr, "string", "trySet missing symbol returns error string")

    test.assert_eq(opengothic.daedalus.exists(nil), false, "exists rejects nil")
    test.assert_eq(opengothic.daedalus.exists(""), false, "exists rejects empty name")

    local knownSymbol = nil
    if opengothic.vm.getSymbol("Npc_IsInState") ~= nil then
        knownSymbol = "Npc_IsInState"
    elseif opengothic.vm.getSymbol("SELF") ~= nil then
        knownSymbol = "SELF"
    end

    if knownSymbol ~= nil then
        test.assert_eq(opengothic.daedalus.exists(knownSymbol), true, "exists reports known symbol")
    else
        test.assert_true(true, "no known symbol discovered for exists positive-path check")
    end

    local ctxOk, ctxResult, ctxErr = opengothic.vm.callContextSafe(nil, {})
    test.assert_eq(ctxOk, false, "callContextSafe rejects invalid function name")
    test.assert_eq(ctxResult, nil, "callContextSafe invalid args returns nil result")
    test.assert_type(ctxErr, "string", "callContextSafe invalid args returns error string")

    ctxOk, ctxResult, ctxErr = opengothic.vm.callContextSafe("Npc_IsInState", nil)
    test.assert_eq(ctxOk, false, "callContextSafe rejects invalid context")
    test.assert_eq(ctxResult, nil, "callContextSafe invalid context returns nil result")
    test.assert_type(ctxErr, "string", "callContextSafe invalid context returns error string")

    ctxOk, ctxResult, ctxErr = opengothic.vm.callContextSafe("__OG_DOES_NOT_EXIST__", {})
    test.assert_eq(ctxOk, false, "callContextSafe returns false for missing function")
    test.assert_eq(ctxResult, nil, "callContextSafe missing function returns nil result")
    test.assert_type(ctxErr, "string", "callContextSafe missing function returns error string")

    local selfOk, selfResult, selfErr = opengothic.vm.callSelf("Npc_IsInState", nil)
    test.assert_eq(selfOk, false, "callSelf rejects nil self")
    test.assert_eq(selfResult, nil, "callSelf invalid self returns nil result")
    test.assert_type(selfErr, "string", "callSelf invalid self returns error string")

    local selfOtherOk, selfOtherResult, selfOtherErr = opengothic.vm.callSelfOther("Npc_IsInState", player, nil)
    test.assert_eq(selfOtherOk, false, "callSelfOther rejects invalid other")
    test.assert_eq(selfOtherResult, nil, "callSelfOther invalid other returns nil result")
    test.assert_type(selfOtherErr, "string", "callSelfOther invalid other returns error string")

    if player ~= nil then
        selfOk, selfResult, selfErr = opengothic.vm.callSelf("__OG_DOES_NOT_EXIST__", player)
        test.assert_eq(selfOk, false, "callSelf returns false for missing function")
        test.assert_eq(selfResult, nil, "callSelf missing function returns nil result")
        test.assert_type(selfErr, "string", "callSelf missing function returns error string")

        selfOtherOk, selfOtherResult, selfOtherErr = opengothic.vm.callSelfOther("__OG_DOES_NOT_EXIST__", player, player)
        test.assert_eq(selfOtherOk, false, "callSelfOther returns false for missing function")
        test.assert_eq(selfOtherResult, nil, "callSelfOther missing function returns nil result")
        test.assert_type(selfOtherErr, "string", "callSelfOther missing function returns error string")
    else
        test.assert_true(true, "no player available for callSelf/callSelfOther error-path checks")
    end

    if player ~= nil and opengothic.vm.getSymbol("Npc_IsInState") ~= nil then
        local probeOk, probeResult, probeErr = opengothic.daedalus.tryCall("Npc_IsInState", player, 0)
        test.assert_type(probeOk, "boolean", "tryCall returns boolean on probe call")
        if probeOk then
            test.assert_type(probeResult, "number", "tryCall returns numeric result for Npc_IsInState")
            test.assert_eq(probeErr, nil, "tryCall success returns nil error")
        else
            test.assert_type(probeErr, "string", "tryCall probe failure returns error string")
        end
    else
        test.assert_true(true, "Npc_IsInState probe skipped (symbol or player unavailable)")
    end

    test.summary()
end)

print("[Test] Daedalus Bridge Helpers test loaded - runs on world load")
