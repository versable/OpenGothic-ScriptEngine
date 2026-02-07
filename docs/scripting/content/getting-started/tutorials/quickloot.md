# Tutorial: Implementing Quickloot

This tutorial implements a common modding feature: **quickloot**.
When the player opens containers, ransacks NPCs, or picks up world items, loot is transferred immediately without opening the usual UI.

This script quickloots:

1. Containers and chests.
2. Unconscious/dead NPC inventories.
3. Individual world items.

## Step 1: Create Your Script File

Create a new Lua file for your script.

**File Location:** `Data/opengothic/scripts/quickloot.lua`

## Step 2: Add the Lua Code

Open `quickloot.lua` and paste the following code:

```lua
-- quickloot.lua

local function notify(text)
    local ok = opengothic.ui.notify(text)
    if not ok then
        opengothic.printMessage(text)
    end
end

local function quicklootInventory(player, sourceInventory, verb)
    local items, err = opengothic.inventory.transferAll(player, sourceInventory, {
        includeEquipped = false,
        includeMission = true
    })

    if err then
        print("[quickloot] transfer failed: " .. err)
        return false
    end

    if #items == 0 then
        return false
    end

    for _, itemRecord in ipairs(items) do
        notify(verb .. " " .. itemRecord.count .. "x " .. itemRecord.name)
    end

    return true
end

-- Containers/chests
opengothic.events.register("onOpen", function(player, interactive)
    if not player:isPlayer() then
        return false
    end

    if not interactive:isContainer() then
        return false
    end

    if interactive:needToLockpick(player) then
        return false
    end

    local sourceInventory = interactive:inventory()
    if not sourceInventory then
        return false
    end

    if quicklootInventory(player, sourceInventory, "Looted") then
        return true -- hide open/ransack UI when loot was transferred
    end

    return false
end)

-- Corpses/unconscious NPCs
opengothic.events.register("onRansack", function(player, target)
    if not player:isPlayer() then
        return false
    end

    local sourceInventory = target:inventory()
    if not sourceInventory then
        return false
    end

    if quicklootInventory(player, sourceInventory, "Ransacked") then
        return true -- hide ransack UI when loot was transferred
    end

    return false
end)

-- Individual world items
opengothic.events.register("onItemPickup", function(npc, item)
    if not npc:isPlayer() then
        return false
    end

    local world = npc:world()
    local inventory = npc:inventory()
    if not world or not inventory then
        return false
    end

    local itemId = item:clsId()
    local count = item:count()
    if count <= 0 then
        return false
    end

    -- Add first to avoid item loss if add fails. Then remove world instance.
    local added = inventory:addItem(itemId, count)
    if not added then
        return false
    end

    world:removeItem(item)
    notify("Picked up " .. added:count() .. "x " .. added:displayName())
    return true -- block default pickup flow
end)

print("quickloot loaded")
```

## Step 3: Explanation

This implementation uses the newer convenience APIs where they help:

- **`opengothic.inventory.transferAll(...)`**:
  - Used for both `onOpen` and `onRansack`.
  - Returns moved item records plus an error code (`items, err`), so failure handling is explicit.
- **`opengothic.ui.notify(...)`**:
  - Used for player-facing loot messages.
  - Wrapped with fallback to `opengothic.printMessage(...)`.

### `onOpen` (containers)

- Guards to player-only and container-only interactions.
- Keeps lockpicking gameplay intact via `interactive:needToLockpick(player)`.
- Returns `true` only when items were actually transferred.

### `onRansack` (NPC inventories)

- Reuses the same transfer helper for corpses/unconscious targets.
- Returns `true` only when there was loot to transfer.

### `onItemPickup` (world items)

- Player-only guard.
- Adds to inventory first, then removes world item instance.
- Returns `true` to block the default pickup flow after manual transfer.

## Step 4: Run the Game

Test these scenarios:

1. Open an unlocked container with items: loot should transfer instantly.
2. Ransack an unconscious/dead NPC: loot should transfer instantly.
3. Pick up a world item: item should go directly to inventory with a message.

If you want less message spam, replace per-item notifications with a single summary line in `quicklootInventory(...)`.

## Related Tutorials

- [Creating a Simple Quest](./creating-a-quest.md)
- [Custom Damage Logic](./custom-damage.md)
- [Dialog Time Windows](./dialog-time-windows.md)
