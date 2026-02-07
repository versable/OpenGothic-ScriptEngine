# Tutorial: Dialog Time Windows

This tutorial combines both time-gating approaches for dialog:

1. Block an NPC's **entire dialog** outside a time range.
2. Hide specific **dialog options** outside configured windows.

## Step 1: Create Your Script File

Create a new Lua file:

**File Location:** `Data/opengothic/scripts/dialog_time_windows.lua`

## Step 2: Add the Lua Code

Open `dialog_time_windows.lua` and add:

```lua
-- dialog_time_windows.lua

local NIGHT_NPC_SYMBOL = "NONE_100_XARDAS"

-- Entire dialog allowed only between 20:00 and 06:00
local DIALOG_WINDOW = {
    startH = 20, startM = 0,
    endH = 6, endM = 0
}

-- Per-option time windows
local OPTION_WINDOWS = {
    INFO_SECRETQUEST_START = {startH = 20, startM = 0, endH = 6, endM = 0},
    INFO_MERCHANT_RAREITEMS = {startH = 8, startM = 0, endH = 18, endM = 0},
    INFO_GUARD_RUMORS = {startH = 12, startM = 0, endH = 13, endM = 0},
}

local function notify(text)
    local ok = opengothic.ui.notify(text)
    if not ok then
        opengothic.printMessage(text)
    end
end

local function refreshSymbols()
    local npcId = opengothic.resolve(NIGHT_NPC_SYMBOL)
    print("[dialog_time_windows] npc=" .. tostring(NIGHT_NPC_SYMBOL) .. " -> " .. tostring(npcId))
end

opengothic.events.register("onWorldLoaded", function()
    refreshSymbols()
    return false
end)

-- Register option-level windows using the dialog convenience API.
for infoName, window in pairs(OPTION_WINDOWS) do
    local ok, err = opengothic.dialog.setOptionTimeWindow(
        infoName,
        window.startH, window.startM,
        window.endH, window.endM
    )
    if not ok then
        print("[dialog_time_windows] failed to register option window for " .. infoName .. ": " .. tostring(err))
    end
end

-- Block full dialog for one NPC outside the configured time window.
opengothic.events.register("onDialogStart", function(npc, player)
    if not npc:isInstance(NIGHT_NPC_SYMBOL) then
        return false
    end

    if not opengothic.dialog.canTalkTo(npc) then
        return true
    end

    local world = opengothic.world()
    if not world then
        return false
    end

    local hour, minute = world:time()
    local clock = string.format("%02d:%02d", hour, minute)

    if world:isTime(
        DIALOG_WINDOW.startH, DIALOG_WINDOW.startM,
        DIALOG_WINDOW.endH, DIALOG_WINDOW.endM
    ) then
        print("[dialog_time_windows] " .. clock .. " allow dialog for " .. NIGHT_NPC_SYMBOL)
        return false -- inside time window: allow dialog
    end

    print("[dialog_time_windows] " .. clock .. " block dialog for " .. NIGHT_NPC_SYMBOL)
    notify(npc:displayName() .. " is unavailable at this hour.")
    return true -- outside time window: block dialog
end)

print("dialog_time_windows loaded")
```

## Step 3: Explanation

This implementation uses the dialog convenience APIs plus one lifecycle hook:

- **`opengothic.dialog.setOptionTimeWindow(...)`**:
  - Registers time windows for specific `INFO_*` dialog options.
  - Option is visible inside the window and hidden outside it.
- **`onDialogStart`**:
  - Used for whole-dialog gating of a specific NPC.
  - Returning `true` blocks the dialog start.
- **`opengothic.dialog.canTalkTo(npc)`**:
  - Fast guard for invalid/non-talkable NPC states.

### Full-dialog gating

- Uses `npc:isInstance(...)` to target one NPC.
- Uses `world:isTime(...)` to allow/block by hour window.
- Shows a user-facing message when blocked.
- Prints current game time and allow/block decision to stdout for quick verification.

### Option-level gating

- Uses `OPTION_WINDOWS` config table keyed by `INFO_*` option names.
- Registers all option windows once at script load.
- No custom `onDialogOption` callback is required for basic time windows.

### Finding option names

If you need option symbol names for your own dialogs:

```lua
opengothic.events.register("onDialogOption", function(npc, player, infoName)
    print("Dialog option: " .. tostring(infoName))
    return false
end)
```

## Step 4: Run the Game

Test flow:

1. Start or load a game (the script only works with an active world/session).
2. Check logs for:
    - `dialog_time_windows loaded`
    - `[dialog_time_windows] npc=NONE_100_XARDAS -> ...`
3. Talk to `NONE_100_XARDAS` at night (20:00-06:00): dialog should open normally.
4. Talk to `NONE_100_XARDAS` outside that window (for example 12:00): dialog should be blocked with an unavailable message. Also check stdout:
    - `... allow dialog ...` inside window
    - `... block dialog ...` outside window
5. Check configured `INFO_*` options at different times:
    - inside their configured windows: option visible
    - outside their configured windows: option hidden
6. For quick testing, call `world:setDayTime(hour, minute)` from a temporary helper script or console flow, then retry the same dialog immediately.
7. Gothic 2 start-area tip: there is a bed close to Xardas' tower, so you can sleep to jump time quickly between allowed/disallowed windows.

If nothing changes:

- Confirm `NIGHT_NPC_SYMBOL` matches your target NPC symbol.
- Confirm your `INFO_*` names actually exist in your loaded script set.
- Use the `onDialogOption` probe shown above to log real option names.

## Related Tutorials

- [Creating a Simple Quest](./creating-a-quest.md)
- [Quickloot](./quickloot.md)
- [Cowardly Scavengers](./scavenger-flee.md)
