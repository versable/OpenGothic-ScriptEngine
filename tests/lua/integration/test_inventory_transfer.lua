-- Inventory Transfer Integration Test
-- Tests transferring items between inventories

local test = opengothic.test

-- This test runs when opening a container
opengothic.events.register("onOpen", function(player, interactive)
    if not interactive:isContainer() then
        return false
    end

    test.suite("Inventory Transfer")

    local srcInv = interactive:inventory()
    local dstInv = player:inventory()
    local world = player:world()

    test.assert_not_nil(srcInv, "source inventory exists")
    test.assert_not_nil(dstInv, "destination inventory exists")

    local srcItems = srcInv:items()
    print("  Container has " .. #srcItems .. " item types")

    if #srcItems == 0 then
        print("[SKIP] Container is empty, nothing to transfer")
        return false
    end

    -- Test itemCount on both inventories
    local firstItem = srcItems[1]
    local itemClsId = firstItem:clsId()
    local itemName = firstItem:displayName()
    local srcCountBefore = srcInv:itemCount(itemClsId)
    local dstCountBefore = dstInv:itemCount(itemClsId)

    print("  Testing transfer of: " .. itemName)
    print("  Source count before: " .. srcCountBefore)
    print("  Dest count before: " .. dstCountBefore)

    -- Transfer 1 item (or all if only 1)
    local transferCount = math.min(srcCountBefore, 1)
    if transferCount > 0 then
        local success = dstInv:transfer(srcInv, itemClsId, transferCount, world)
        test.assert_true(success, "transfer returns true")

        -- Verify counts changed
        local srcCountAfter = srcInv:itemCount(itemClsId)
        local dstCountAfter = dstInv:itemCount(itemClsId)

        print("  Source count after: " .. srcCountAfter)
        print("  Dest count after: " .. dstCountAfter)

        test.assert_eq(srcCountAfter, srcCountBefore - transferCount, "source count decreased")
        test.assert_eq(dstCountAfter, dstCountBefore + transferCount, "dest count increased")
    end

    test.summary()

    return false -- Don't block the open action
end)

print("[Test] Inventory Transfer test loaded - runs when opening container")
