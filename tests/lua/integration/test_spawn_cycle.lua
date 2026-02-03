-- Spawn Cycle Integration Test
-- Tests spawn, modify, and remove cycle for NPCs and Items

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Spawn Cycle")

    local world = opengothic.world()
    local player = opengothic.player()
    local px, py, pz = player:position()

    -- Try to resolve a scavenger NPC type (common in Gothic games)
    local scavengerNames = {"YOURSCAVENGER", "SCAVENGER", "YOURWOLF", "WOLF", "YOURKEILER", "KEILER"}
    local npcType = nil
    local npcTypeName = nil

    for _, name in ipairs(scavengerNames) do
        local id = opengothic.resolve(name)
        if id then
            npcType = id
            npcTypeName = name
            break
        end
    end

    if not npcType then
        print("[SKIP] Spawn Cycle: No known creature types found")
        print("  Tried: " .. table.concat(scavengerNames, ", "))
        return
    end

    print("  Using NPC type: " .. npcTypeName .. " (ID: " .. npcType .. ")")

    -- Spawn NPC at position offset from player
    local spawnX = px + 500
    local spawnY = py
    local spawnZ = pz + 500

    local npc = world:addNpcAt(npcType, spawnX, spawnY, spawnZ)
    test.assert_not_nil(npc, "NPC spawned successfully")

    if npc then
        -- Verify spawned NPC properties
        test.assert_eq(npc:isDead(), false, "spawned NPC is alive")
        test.assert_type(npc:displayName(), "string", "spawned NPC has displayName")
        test.assert_type(npc:instanceId(), "number", "spawned NPC has instanceId")

        local nx, ny, nz = npc:position()
        test.assert_true(nx ~= 0 or ny ~= 0 or nz ~= 0, "NPC has valid position")
        print("  Spawned at: " .. nx .. ", " .. ny .. ", " .. nz)

        -- Test attribute access on spawned NPC
        local Attr = opengothic.CONSTANTS.Attribute
        local hp = npc:attribute(Attr.ATR_HITPOINTS)
        test.assert_type(hp, "number", "spawned NPC has HP attribute")
        test.assert_true(hp > 0, "spawned NPC HP > 0")
        print("  HP: " .. hp)

        -- Remove the NPC
        world:removeNpc(npc)
        print("  NPC removed")
        -- Note: Cannot easily verify removal without additional API
    end

    test.summary()
end)

-- Item spawn test
opengothic.events.register("onWorldLoaded", function()
    test.suite("Item Spawn Cycle")

    local world = opengothic.world()
    local player = opengothic.player()
    local px, py, pz = player:position()

    -- Try to resolve common item types
    local itemNames = {"ITFO_APPLE", "ITMI_GOLD", "ITFO_BREAD", "ITAT_GOLDCOIN"}
    local itemType = nil
    local itemTypeName = nil

    for _, name in ipairs(itemNames) do
        local id = opengothic.resolve(name)
        if id then
            itemType = id
            itemTypeName = name
            break
        end
    end

    if not itemType then
        print("[SKIP] Item Spawn Cycle: No known item types found")
        return
    end

    print("  Using item type: " .. itemTypeName .. " (ID: " .. itemType .. ")")

    -- Spawn item near player
    local item = world:addItemAt(itemType, px + 100, py + 50, pz + 100)
    test.assert_not_nil(item, "Item spawned successfully")

    if item then
        test.assert_type(item:displayName(), "string", "spawned item has displayName")
        test.assert_type(item:count(), "number", "spawned item has count")
        print("  Spawned: " .. item:displayName() .. " x" .. item:count())

        -- Remove item
        world:removeItem(item)
        print("  Item removed")
    end

    test.summary()
end)

print("[Test] Spawn Cycle test loaded - runs on world load")
