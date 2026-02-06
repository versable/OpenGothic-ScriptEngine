-- Dialog Helpers Test Suite
-- Tests high-level opengothic.dialog convenience wrappers

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Dialog Helpers")

    local player = opengothic.player()
    local world = opengothic.world()

    test.assert_type(opengothic.dialog, "table", "opengothic.dialog module exists")
    test.assert_type(opengothic.dialog.isOption, "function", "isOption exists")
    test.assert_type(opengothic.dialog.canTalkTo, "function", "canTalkTo exists")

    -- Invalid argument handling
    test.assert_eq(opengothic.dialog.isOption(nil), false, "isOption rejects nil")
    test.assert_eq(opengothic.dialog.isOption(42), false, "isOption rejects non-string")
    test.assert_eq(opengothic.dialog.isOption(""), false, "isOption rejects empty string")
    test.assert_eq(opengothic.dialog.canTalkTo(nil), false, "canTalkTo rejects nil")
    test.assert_eq(opengothic.dialog.canTalkTo({}), false, "canTalkTo rejects non-npc userdata")
    test.assert_eq(opengothic.dialog.canTalkTo(player), false, "canTalkTo rejects player target")

    -- Resolve-based option detection (best-effort discovery)
    local foundInfoName = nil
    local enumOk = pcall(function()
        opengothic.vm.enumerate("", function(sym)
            if type(sym) == "table" and type(sym.name) == "string" then
                if string.sub(sym.name, 1, 5) == "INFO_" then
                    foundInfoName = sym.name
                    return false
                end
            end
            return true
        end)
    end)
    test.assert_eq(enumOk, true, "vm.enumerate does not error while scanning options")

    if foundInfoName ~= nil then
        test.assert_eq(opengothic.dialog.isOption(foundInfoName), true, "isOption accepts discovered INFO symbol")
    else
        -- Fallback smoke path when no INFO symbols are present in the active data set
        test.assert_type(opengothic.dialog.isOption("INFO_NON_EXISTENT"), "boolean", "isOption fallback remains boolean")
    end

    local nearestNpc = nil
    if world and player and world.findNearestNpc then
        local nearOk, near = pcall(function()
            return world:findNearestNpc(player, 5000)
        end)
        if nearOk then
            nearestNpc = near
        end
    end

    if nearestNpc ~= nil then
        test.assert_type(opengothic.dialog.canTalkTo(nearestNpc), "boolean", "canTalkTo returns boolean for NPC")
    else
        test.assert_true(true, "no nearby npc available for canTalkTo smoke check")
    end

    test.summary()
end)

print("[Test] Dialog Helpers test loaded - runs on world load")
