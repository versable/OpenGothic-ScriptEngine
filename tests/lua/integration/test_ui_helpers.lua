-- UI Helpers Test Suite
-- Tests high-level opengothic.ui convenience wrappers

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("UI Helpers")

    test.assert_type(opengothic.ui, "table", "opengothic.ui module exists")
    test.assert_type(opengothic.ui.notify, "function", "notify exists")
    test.assert_type(opengothic.ui.toast, "function", "toast exists")
    test.assert_type(opengothic.ui.debug, "function", "debug exists")

    -- Invalid argument handling
    local ok, err = opengothic.ui.notify(nil)
    test.assert_eq(ok, false, "notify rejects nil text")
    test.assert_type(err, "string", "notify returns error string")

    ok, err = opengothic.ui.toast(nil, 1)
    test.assert_eq(ok, false, "toast rejects nil text")
    test.assert_type(err, "string", "toast invalid text error string")

    ok, err = opengothic.ui.toast("Hello", "1")
    test.assert_eq(ok, false, "toast rejects non-number duration")
    test.assert_type(err, "string", "toast invalid duration error string")

    -- Valid usage
    ok, err = opengothic.ui.notify("UI helper notify smoke test")
    test.assert_eq(ok, true, "notify succeeds with string")
    test.assert_true(err == nil, "notify returns nil error on success")

    ok, err = opengothic.ui.toast("UI helper toast smoke test", 1.0)
    test.assert_eq(ok, true, "toast succeeds with defaults")
    test.assert_true(err == nil, "toast returns nil error on success")

    ok, err = opengothic.ui.toast("UI helper styled toast", 1.0, {
        x = -1,
        y = 80,
        font = "font_old_10_white.tga"
    })
    test.assert_eq(ok, true, "toast succeeds with style overrides")
    test.assert_true(err == nil, "styled toast returns nil error on success")

    local debugOk = pcall(function()
        opengothic.ui.debug("UI helper debug smoke test")
        opengothic.ui.debug(12345)
        opengothic.ui.debug(nil)
    end)
    test.assert_eq(debugOk, true, "debug never crashes for test inputs")

    test.summary()
end)

print("[Test] UI Helpers test loaded - runs on world load")
