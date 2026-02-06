-- Timer Helpers Test Suite
-- Tests high-level opengothic.timer wrappers and onUpdate dispatch

local test = opengothic.test

local state = {
    started = false,
    done = false,
    elapsed = 0,
    updateTicks = 0,
    afterFired = 0,
    everyFired = 0,
    canceledFired = false
}

opengothic.events.register("onWorldLoaded", function()
    test.suite("Timer Helpers")

    test.assert_type(opengothic.timer, "table", "opengothic.timer module exists")
    test.assert_type(opengothic.timer.after, "function", "after exists")
    test.assert_type(opengothic.timer.every, "function", "every exists")
    test.assert_type(opengothic.timer.cancel, "function", "cancel exists")
    test.assert_type(opengothic.timer.everyGameMinute, "function", "everyGameMinute exists")

    -- Invalid argument handling
    local id, err = opengothic.timer.after(-1, function() end)
    test.assert_true(id == nil, "after rejects invalid seconds")
    test.assert_type(err, "string", "after returns error string")

    id, err = opengothic.timer.every(0, function() end)
    test.assert_true(id == nil, "every rejects non-positive seconds")
    test.assert_type(err, "string", "every returns error string")

    id, err = opengothic.timer.everyGameMinute(nil)
    test.assert_true(id == nil, "everyGameMinute rejects non-function")
    test.assert_type(err, "string", "everyGameMinute returns error string")

    test.assert_eq(opengothic.timer.cancel("x"), false, "cancel rejects non-number id")

    -- Valid path setup
    local afterId
    afterId, err = opengothic.timer.after(0.05, function()
        state.afterFired = state.afterFired + 1
    end)
    test.assert_type(afterId, "number", "after returns timer id")
    test.assert_true(err == nil, "after success returns nil error")

    local everyId
    everyId, err = opengothic.timer.every(0.02, function(taskId)
        state.everyFired = state.everyFired + 1
        if state.everyFired >= 3 then
            opengothic.timer.cancel(taskId)
        end
    end)
    test.assert_type(everyId, "number", "every returns timer id")
    test.assert_true(err == nil, "every success returns nil error")

    local cancelId
    cancelId, err = opengothic.timer.after(1.0, function()
        state.canceledFired = true
    end)
    test.assert_type(cancelId, "number", "cancel-target timer returns id")
    test.assert_eq(opengothic.timer.cancel(cancelId), true, "cancel succeeds for active timer")

    local minuteId
    minuteId, err = opengothic.timer.everyGameMinute(function() end)
    test.assert_type(minuteId, "number", "everyGameMinute returns timer id")
    test.assert_true(err == nil, "everyGameMinute success returns nil error")
    test.assert_eq(opengothic.timer.cancel(minuteId), true, "cancel succeeds for game-minute timer")

    state.started = true
end)

opengothic.events.register("onUpdate", function(dt)
    if not state.started or state.done then
        return false
    end

    state.updateTicks = state.updateTicks + 1
    if type(dt) == "number" and dt > 0 then
        state.elapsed = state.elapsed + dt
    end

    if state.elapsed >= 0.5 or state.updateTicks >= 120 then
        test.assert_true(state.updateTicks > 0, "onUpdate dispatch reached Lua")
        test.assert_eq(state.afterFired, 1, "after fires exactly once")
        test.assert_true(state.everyFired >= 3, "every fires repeatedly until canceled")
        test.assert_eq(state.canceledFired, false, "canceled timer callback does not fire")
        state.done = true
        test.summary()
    end

    return false
end)

print("[Test] Timer Helpers test loaded - runs on world load")
