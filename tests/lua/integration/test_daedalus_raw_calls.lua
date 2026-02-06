-- Daedalus Raw Calls Test Suite
-- Validates direct opengothic.daedalus.call/set/get and vm.callWithContext behavior.

local test = opengothic.test
local topicSeq = 0

local function hasSymbol(name)
    return opengothic.vm.getSymbol(name) ~= nil
end

local function skip(message)
    print("[SKIP] Daedalus Raw Calls: " .. message)
    test.assert_true(true, message)
end

opengothic.events.register("onWorldLoaded", function()
    test.suite("Daedalus Raw Calls")

    local player = opengothic.player()

    test.assert_type(opengothic.daedalus, "table", "opengothic.daedalus table exists")
    test.assert_type(opengothic.vm, "table", "opengothic.vm table exists")
    test.assert_type(opengothic.daedalus.call, "function", "daedalus.call exists")
    test.assert_type(opengothic.daedalus.get, "function", "daedalus.get exists")
    test.assert_type(opengothic.daedalus.set, "function", "daedalus.set exists")
    test.assert_type(opengothic.vm.callWithContext, "function", "vm.callWithContext exists")

    local ok, err = pcall(opengothic.daedalus.call, nil)
    test.assert_eq(ok, false, "daedalus.call rejects invalid function name")
    test.assert_type(err, "string", "daedalus.call invalid name returns error string")

    ok, err = pcall(opengothic.daedalus.call, "__OG_DOES_NOT_EXIST__")
    test.assert_eq(ok, false, "daedalus.call fails for missing function")
    test.assert_type(err, "string", "missing function returns error string")

    if hasSymbol("Npc_IsInState") and player ~= nil then
        ok, err = pcall(opengothic.daedalus.call, "Npc_IsInState", player, {})
        test.assert_eq(ok, false, "daedalus.call rejects unsupported argument types")
        test.assert_type(err, "string", "unsupported argument error is string")

        local callOk, result = pcall(opengothic.daedalus.call, "Npc_IsInState", player, 0)
        test.assert_eq(callOk, true, "daedalus.call supports userdata + int arguments")
        test.assert_type(result, "number", "Npc_IsInState raw call returns number")
    else
        skip("Npc_IsInState probe skipped (symbol or player unavailable)")
    end

    if hasSymbol("Npc_GetStateTime") and player ~= nil then
        local callOk, result = pcall(opengothic.daedalus.call, "Npc_GetStateTime", player)
        test.assert_eq(callOk, true, "daedalus.call supports userdata-only argument call")
        test.assert_type(result, "number", "Npc_GetStateTime raw call returns number")
    else
        skip("Npc_GetStateTime probe skipped (symbol or player unavailable)")
    end

    if hasSymbol("Hlp_StrCmp") then
        local callOk, result = pcall(opengothic.daedalus.call, "Hlp_StrCmp", "abc", "abc")
        test.assert_eq(callOk, true, "daedalus.call supports string argument marshalling")
        test.assert_type(result, "number", "Hlp_StrCmp raw call returns number")
    else
        skip("Hlp_StrCmp probe skipped (symbol unavailable)")
    end

    if hasSymbol("PLAYER_CHAPTER") then
        local oldChapter = opengothic.daedalus.get("PLAYER_CHAPTER")
        test.assert_type(oldChapter, "number", "daedalus.get returns chapter number")

        local setOk, setErr = pcall(opengothic.daedalus.set, "PLAYER_CHAPTER", oldChapter)
        test.assert_eq(setOk, true, "daedalus.set accepts writing existing numeric symbol")
        test.assert_true(setErr == nil, "daedalus.set no error on same-value write")

        local chapterAfter = opengothic.daedalus.get("PLAYER_CHAPTER")
        test.assert_eq(chapterAfter, oldChapter, "daedalus.set write is observable via daedalus.get")

        setOk, setErr = pcall(opengothic.daedalus.set, "PLAYER_CHAPTER", "bad_type")
        test.assert_eq(setOk, false, "daedalus.set rejects invalid type")
        test.assert_type(setErr, "string", "invalid type write returns error string")
    else
        skip("PLAYER_CHAPTER set/get probe skipped (symbol unavailable)")
    end

    if hasSymbol("Npc_IsInState") and hasSymbol("SELF") and player ~= nil then
        local beforeSelf = opengothic.daedalus.get("SELF")
        local ctxOk, ctxResult = pcall(opengothic.vm.callWithContext, "Npc_IsInState", { self = player }, player, 0)
        test.assert_eq(ctxOk, true, "vm.callWithContext supports explicit self context")
        test.assert_type(ctxResult, "number", "vm.callWithContext returns numeric result")

        local afterSelf = opengothic.daedalus.get("SELF")
        test.assert_eq(afterSelf, beforeSelf, "vm.callWithContext restores SELF after call")
    else
        skip("vm.callWithContext probe skipped (symbols or player unavailable)")
    end

    if hasSymbol("Log_CreateTopic") and hasSymbol("Log_SetTopicStatus") and hasSymbol("Log_AddEntry") then
        topicSeq = topicSeq + 1
        local topicName = "OG_LUA_RAW_CALLS_" .. tostring(topicSeq)

        local createOk, createErr = pcall(opengothic.daedalus.call, "Log_CreateTopic", topicName, 1)
        test.assert_eq(createOk, true, "daedalus.call supports Log_CreateTopic(topic, section)")
        test.assert_true(createErr == nil, "Log_CreateTopic does not return Lua error")

        local addOk, addErr = pcall(opengothic.daedalus.call, "Log_AddEntry", topicName, "raw-call entry")
        test.assert_eq(addOk, true, "daedalus.call supports Log_AddEntry(topic, text)")
        test.assert_true(addErr == nil, "Log_AddEntry does not return Lua error")

        local statusOk, statusErr = pcall(opengothic.daedalus.call, "Log_SetTopicStatus", topicName, 1)
        test.assert_eq(statusOk, true, "daedalus.call supports Log_SetTopicStatus(topic, status)")
        test.assert_true(statusErr == nil, "Log_SetTopicStatus does not return Lua error")
    else
        skip("Log_* probes skipped (symbols unavailable)")
    end

    test.summary()
end)

print("[Test] Daedalus Raw Calls test loaded - runs on world load")
