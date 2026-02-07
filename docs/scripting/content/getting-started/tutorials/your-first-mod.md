# Tutorial: Your First Mod (Health Monitor)

This tutorial builds a minimal health monitor script.
When the player takes damage, the script logs current and maximum HP to stdout.

This script will:

1. Register one damage event handler.
2. Filter to player-only hits.
3. Log HP values with `print(...)`.

## Step 1: Create Your Script File

Create a new Lua file:

**File Location:** `Data/opengothic/scripts/health_monitor.lua`

## Step 2: Add the Lua Code

Open `health_monitor.lua` and add:

```lua
-- health_monitor.lua

opengothic.events.register("onNpcTakeDamage", function(victim, attacker, isSpell, spellId)
    if not victim:isPlayer() then
        return false
    end

    local currentHp = victim:attribute(opengothic.CONSTANTS.Attribute.ATR_HITPOINTS)
    local maxHp = victim:attribute(opengothic.CONSTANTS.Attribute.ATR_HITPOINTSMAX)

    print("[health_monitor] player HP: " .. tostring(currentHp) .. "/" .. tostring(maxHp))
    return false -- keep default game damage behavior
end)

print("health_monitor loaded")
```

## Step 3: Explanation

This implementation uses one event hook and one object API call group:

- **`onNpcTakeDamage`**:
  - Triggered before default damage resolution.
  - Returning `false` keeps default engine behavior unchanged.
- **`victim:attribute(...)`**:
  - Reads current and max HP from `opengothic.CONSTANTS.Attribute`.
- **`print(...)`**:
  - Writes developer/test output to stdout instead of in-game UI.

## Step 4: Run the Game

Test flow:

1. Launch OpenGothic and load/start a game.
2. Confirm `health_monitor loaded` appears in script output.
3. Take damage from an enemy.
4. Confirm the HP log line updates on each hit.

## Related Tutorials

- [Creating a Simple Quest](./creating-a-quest.md)
- [Quickloot](./quickloot.md)
- [Dialog Time Windows](./dialog-time-windows.md)
