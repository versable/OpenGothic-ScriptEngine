# Tutorial: Creating a Simple Quest

This tutorial walks through a small quest with three phases:

1. Start the quest by talking to a specific NPC.
2. Progress the quest by picking up a target item.
3. Complete the quest by returning to the NPC with that item.

The script uses `opengothic.quest.*` convenience APIs and stores progress in `opengothic.storage` so save/load is handled correctly.

## Step 1: Setup Symbols and Create Your Script

Before writing quest logic, choose symbols that definitely exist in your current game/scripts.

Recommended first pass (Gothic 2 NoTR):

- Xardas NPC symbol: `NONE_100_XARDAS`
- Quest item symbol: `ITPO_HEALTH_01`

If you want a different NPC, use a short probe script first:

**File Location:** `Data/opengothic/scripts/xardas_probe.lua`

```lua
opengothic.events.register("onDialogStart", function(npc, player)
    local id = npc:instanceId()
    local symbol = nil
    opengothic.vm.enumerate("", function(sym)
        if sym.index == id then
            symbol = sym.name
            return false
        end
        return true
    end)

    print("[xardas_probe] dialog with: " .. npc:displayName()
        .. " | id=" .. tostring(id)
        .. " | symbol=" .. tostring(symbol))
    return false
end)
```

Talk to your target NPC once and copy the printed `symbol=...` value into `XARDAS_NPC_SYMBOL`.

### Probe Explanation

- `npc:instanceId()` gives the Daedalus symbol index for the currently speaking NPC.
- `opengothic.vm.enumerate("", ...)` walks known symbols; the probe matches by `sym.index`.
- The printed `symbol=...` is the string you should use in `npc:isInstance(...)` checks.

Now create the quest script file:

**File Location:** `Data/opengothic/scripts/my_first_quest.lua`

## Step 2: Define Quest Logic

Open `my_first_quest.lua` and add:

```lua
-- my_first_quest.lua

local QUEST_TOPIC_NAME = "An Apple a day keeps Xardas at bay..."
local XARDAS_NPC_SYMBOL = "NONE_100_XARDAS"
local XARDAS_ITEM_SYMBOL = "ITPO_HEALTH_01"
local STAGE_KEY = "my_first_quest.stage"

local missingScrollId = nil

local function notify(text)
    local ok = opengothic.ui.notify(text)
    if not ok then
        opengothic.printMessage(text)
    end
end

local function getStage()
    local stage = opengothic.storage[STAGE_KEY]
    if type(stage) ~= "number" then
        return 0
    end
    return stage
end

local function setStage(stage)
    opengothic.storage[STAGE_KEY] = stage
end

local function refreshSymbols()
    -- resolve() requires an active world/session
    missingScrollId = opengothic.resolve(XARDAS_ITEM_SYMBOL)
    print("[my_first_quest] configured item=" .. tostring(XARDAS_ITEM_SYMBOL)
        .. " resolvedId=" .. tostring(missingScrollId))
end

local function startQuest()
    if getStage() ~= 0 then
        return
    end

    notify("New Quest: An Apple a day keeps Xardas at bay...")
    opengothic.quest.create(QUEST_TOPIC_NAME, opengothic.quest.SECTION.MISSIONS)
    opengothic.quest.addEntry(QUEST_TOPIC_NAME, "Xardas looks like he argued with twenty demons and lost sleep to all of them. He wants an Essence of Healing, now.")
    opengothic.quest.setState(QUEST_TOPIC_NAME, opengothic.quest.STATUS.RUNNING)
    setStage(1)
end

local function progressQuest()
    if getStage() ~= 1 then
        return
    end

    notify("Quest Update: Medicine for the old necromancer")
    opengothic.quest.addEntry(QUEST_TOPIC_NAME, "I found an Essence of Healing near Xardas' place. If I bring it quickly, maybe he stops cursing apprentices for a minute.")
    setStage(2)
end

local function completeQuest()
    if getStage() ~= 2 then
        return
    end

    notify("Quest Complete: Xardas survives another day")
    opengothic.quest.addEntry(QUEST_TOPIC_NAME, "Xardas drank the potion without thanks, muttered that I am useful for once, and went back to brooding over ancient disasters.")
    opengothic.quest.setState(QUEST_TOPIC_NAME, opengothic.quest.STATUS.SUCCESS)
    setStage(3)
end

-- Re-resolve symbols whenever a world is loaded
opengothic.events.register("onWorldLoaded", function()
    refreshSymbols()
    return false
end)

-- New game: reset tutorial quest stage
opengothic.events.register("onStartGame", function(worldName)
    setStage(0)
    refreshSymbols()
    print("[my_first_quest] initialized in world " .. tostring(worldName))
    return false
end)

-- Load game: keep stage from storage, only refresh cached symbol IDs
opengothic.events.register("onLoadGame", function(saveName)
    refreshSymbols()
    print("[my_first_quest] loaded save " .. tostring(saveName) .. " at stage " .. tostring(getStage()))
    return false
end)

-- Start or complete the quest by talking to the configured NPC
opengothic.events.register("onDialogStart", function(npc, player)
    if not npc:isInstance(XARDAS_NPC_SYMBOL) then
        return false
    end

    local stage = getStage()
    if stage == 0 then
        startQuest()
    elseif stage == 2 then
        if opengothic.inventory.hasItem(player, XARDAS_ITEM_SYMBOL) then
            completeQuest()
        end
    end

    return false
end)

-- Progress quest when player picks up the target item
opengothic.events.register("onItemPickup", function(npc, item)
    if not npc:isPlayer() or getStage() ~= 1 then
        return false
    end

    if missingScrollId == nil then
        refreshSymbols()
    end

    if missingScrollId and item:clsId() == missingScrollId then
        progressQuest()
    end

    return false
end)

print("my_first_quest loaded")
```

## Step 3: Explanation

This implementation uses quest hooks and save-safe state handling:

- **`opengothic.quest.*`**:
  - Used to create the topic, add entries, and set status.
- **`opengothic.storage`**:
  - Stores the quest stage under `my_first_quest.stage`, so progress survives save/load.
- **`refreshSymbols()`**:
  - Re-resolves IDs after world/session events because `opengothic.resolve(...)` requires an active world.
  - Prints the resolved item ID so setup mistakes are visible immediately.

### `onStartGame` and `onLoadGame`

- `onStartGame`: resets stage to `0` for a new playthrough.
- `onLoadGame`: keeps stored stage and only refreshes cached IDs.

### `onDialogStart`

- Starts the quest at stage `0`.
- Completes the quest at stage `2` if `opengothic.inventory.hasItem(...)` confirms the target item.

### `onItemPickup`

- Advances stage `1 -> 2` when the configured item is picked up by the player.

## Step 4: Run the Game

Test flow:

1. Start a new game.
2. Check logs for `my_first_quest loaded` and item symbol resolution from `refreshSymbols()`.
3. Talk to Xardas (`NONE_100_XARDAS` by default).
4. Verify:
    - `New Quest: An Apple a day keeps Xardas at bay...` appears.
    - A quest topic is created in the log.
5. Pick up the configured item (`ITPO_HEALTH_01` by default).
6. Verify:
    - `Quest Update: Medicine for the old necromancer` appears.
    - A second quest entry is added.
7. Talk to Xardas again.
8. Verify:
    - `Quest Complete: Xardas survives another day` appears.
    - Topic status switches to success.
9. Save and reload at different stages to confirm persistence.

If a stage does not advance:

- Confirm `XARDAS_NPC_SYMBOL` and `XARDAS_ITEM_SYMBOL` are valid in your script set.
- Keep `xardas_probe.lua` enabled until NPC symbol matching is correct.
- Watch `[my_first_quest] ... resolvedId=...` log output to catch unresolved item symbols (`nil`).

## Related Tutorials

- [Quickloot](./quickloot.md)
- [Dialog Time Windows](./dialog-time-windows.md)
- [Custom Damage Logic](./custom-damage.md)
