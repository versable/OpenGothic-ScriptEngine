# Tutorial: Auto Pickup

This tutorial implements automatic pickup of nearby world items for the player.

Our script will:

1. Periodically scan for the nearest item around the player.
2. Filter pickup candidates (for example mission-item exclusion).
3. Move the item into player inventory and remove it from the world.
4. Start and stop cleanly across world/session lifecycle events.

## Step 1: Create Your Script File

Create a new Lua file:

**File Location:** `Data/opengothic/scripts/auto_pickup.lua`

## Step 2: Add the Lua Code

Open `auto_pickup.lua` and add:

```lua
-- auto_pickup.lua

local PICKUP_RANGE = 180
local PICKUP_INTERVAL_SECONDS = 0.25
local ALLOW_MISSION_ITEMS = false
local SHOW_PICKUP_MESSAGES = true

local pickupTimerId = nil

local function notify(text)
    local ok = opengothic.ui.notify(text)
    if not ok then
        opengothic.printMessage(text)
    end
end

local function isPickupCandidate(item)
    if not ALLOW_MISSION_ITEMS and item:isMission() then
        return false
    end
    return true
end

local function stopAutoPickupTimer()
    if pickupTimerId then
        opengothic.timer.cancel(pickupTimerId)
        pickupTimerId = nil
    end
end

local function tryAutoPickup()
    local player = opengothic.player()
    if not player then
        return
    end

    local world = player:world()
    local inventory = player:inventory()
    if not world or not inventory then
        return
    end

    local item = opengothic.worldutil.findNearestItem(player, PICKUP_RANGE, isPickupCandidate)
    if not item then
        return
    end

    local itemId = item:clsId()
    local count = item:count()
    if count <= 0 then
        return
    end

    local added = inventory:addItem(itemId, count)
    if not added then
        print("[auto_pickup] addItem failed for item id " .. tostring(itemId))
        return
    end

    world:removeItem(item)

    if SHOW_PICKUP_MESSAGES then
        notify("Auto-picked " .. tostring(added:count()) .. "x " .. added:displayName())
    end
end

local function startAutoPickupTimer()
    stopAutoPickupTimer()

    local timerId, err = opengothic.timer.every(PICKUP_INTERVAL_SECONDS, function(_)
        local ok, tickErr = pcall(tryAutoPickup)
        if not ok then
            print("[auto_pickup] tick failed: " .. tostring(tickErr))
        end
    end)

    if not timerId then
        print("[auto_pickup] timer start failed: " .. tostring(err))
        return
    end

    pickupTimerId = timerId
end

opengothic.events.register("onWorldLoaded", function()
    startAutoPickupTimer()
    return false
end)

opengothic.events.register("onSessionExit", function()
    stopAutoPickupTimer()
    return false
end)

print("auto_pickup loaded")
```

## Step 3: Explanation

This implementation combines timer and world utility helpers:

- **`opengothic.timer.every(...)`**:
  - Runs a periodic check without requiring an `onUpdate` handler in your script.
- **`opengothic.worldutil.findNearestItem(...)`**:
  - Finds the nearest item near the player.
  - Uses a predicate (`isPickupCandidate`) to filter items.
- **`inventory:addItem(...)` + `world:removeItem(...)`**:
  - Adds to inventory first, then removes the world instance to avoid accidental loss on add failure.

### Candidate filtering

- `ALLOW_MISSION_ITEMS = false` prevents auto-looting mission items by default.
- Extend `isPickupCandidate(item)` if your mod needs stricter rules (for example only gold or only ammo).

### Lifecycle safety

- `onWorldLoaded`: restarts timer for the active world.
- `onSessionExit`: cancels timer to avoid stale periodic callbacks across sessions.

## Step 4: Run the Game

Test flow:

1. Start or load a game.
2. Approach a dropped world item.
3. Verify item is automatically moved to inventory within a short interval.
4. Toggle `ALLOW_MISSION_ITEMS` and confirm mission-item behavior changes.

If pickup is too aggressive, increase `PICKUP_INTERVAL_SECONDS` or reduce `PICKUP_RANGE`.

## Related Tutorials

- [Quickloot](./quickloot.md)
- [Your First Mod](./your-first-mod.md)
- [Cowardly Scavengers](./scavenger-flee.md)
