-- Nil Hook Args Test Suite
-- Verifies nullable hook arguments are passed as Lua nil (not userdata)

local test = opengothic.test

local pendingDeathCheck = false

opengothic.events.register("onNpcDeath", function(victim, killer, isDeath)
    if pendingDeathCheck then
        test.assert_eq(killer, nil, "onNpcDeath killer is nil when no killer exists")
        test.assert_eq(isDeath, true, "forced 0 HP results in death event")
        pendingDeathCheck = false
    end
    return false
end)

opengothic.events.register("onWorldLoaded", function()
    test.suite("Nil Hook Args")

    local world = opengothic.world()
    local player = opengothic.player()
    test.assert_not_nil(world, "world exists")
    test.assert_not_nil(player, "player exists")

    local px, py, pz = player:position()

    -- Pick a common spawnable NPC type, same strategy used by other integration tests.
    local npcNames = {"YOURSCAVENGER", "SCAVENGER", "YOURWOLF", "WOLF", "YOURKEILER", "KEILER"}
    local npcType = nil

    for _, name in ipairs(npcNames) do
        local id = opengothic.resolve(name)
        if id then
            npcType = id
            break
        end
    end

    if not npcType then
        print("[SKIP] Nil Hook Args: no known creature instance available")
        return
    end

    local npc = world:addNpcAt(npcType, px + 350, py, pz + 350)
    test.assert_not_nil(npc, "test NPC spawned")
    if not npc then
        test.summary()
        return
    end

    pendingDeathCheck = true
    npc:setHealth(0)

    test.assert_eq(pendingDeathCheck, false, "onNpcDeath fired for forced test NPC death")
    test.summary()
end)

print("[Test] Nil Hook Args test loaded - runs on world load")
