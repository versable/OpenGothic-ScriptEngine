# Tutorial: Cowardly Scavengers

This tutorial implements a state-driven scavenger behavior: scavengers initially run from the player, then switch to normal combat behavior after the player attacks them.

The script does this by combining perception hooks, a flee action with explicit target setup, and a simple retaliate-state toggle.

Behavior summary:

1. Flees from the player on threat perception.
2. Stops fleeing and allows default combat behavior after being hit by the player.
3. Prints detailed debug output so you can verify each state transition.

This version is intentionally simple and deterministic.

## Step 1: Create Your Script File

Create a new Lua file:

**File Location:** `Data/opengothic/scripts/cowardly_scavengers.lua`

## Step 2: Add the Lua Code

Open `cowardly_scavengers.lua` and add:

```lua
-- cowardly_scavengers.lua

local SCAVENGER_SYMBOLS = { "SCAVENGER", "YSCAVENGER" }
local FLEE_COOLDOWN_SECONDS = 0.8

local scavengerByInstance = {}
local retaliate = {}  -- key: tostring(npc), value: true after player hit
local nextFleeAt = {} -- key: tostring(npc), value: game runtime seconds
local now = 0.0

local THREAT_PERC = 2 -- PERC_ASSESSENEMY
if opengothic.CONSTANTS and opengothic.CONSTANTS.PercType then
    THREAT_PERC = opengothic.CONSTANTS.PercType.PERC_ASSESSENEMY or THREAT_PERC
end

local function keyOf(npc)
    return tostring(npc)
end

local function nameOf(npc)
    local ok, name = pcall(function()
        return npc:displayName()
    end)
    if ok and type(name) == "string" then
        return name
    end
    return "<?>"
end

local function instOf(npc)
    local ok, inst = pcall(function()
        return npc:instanceId()
    end)
    if ok and type(inst) == "number" then
        return inst
    end
    return nil
end

local function isPlayer(npc)
    local ok, value = pcall(function()
        return npc:isPlayer()
    end)
    return ok and value == true
end

local function resolveSymbols()
    scavengerByInstance = {}

    for _, symbol in ipairs(SCAVENGER_SYMBOLS) do
        local id = opengothic.resolve(symbol)
        if type(id) == "number" then
            scavengerByInstance[id] = symbol
            print("[cowardly_scavengers_v1] symbol " .. symbol .. " -> " .. tostring(id))
        else
            print("[cowardly_scavengers_v1] symbol " .. symbol .. " unresolved")
        end
    end
end

local function isScavenger(npc)
    local inst = instOf(npc)
    if inst == nil then
        return false
    end
    return scavengerByInstance[inst] ~= nil
end

opengothic.events.register("onUpdate", function(dt)
    if type(dt) == "number" then
        now = now + dt
    end
    return false
end)

opengothic.events.register("onWorldLoaded", function()
    resolveSymbols()
    retaliate = {}
    nextFleeAt = {}
    now = 0.0
    print("[cowardly_scavengers_v1] world loaded; behavior active")
    return false
end)

opengothic.events.register("onNpcPerception", function(npc, other, percType)
    if not isScavenger(npc) then
        return false
    end

    if other == nil or not isPlayer(other) then
        return false
    end

    local key = keyOf(npc)
    local inst = instOf(npc)
    local mode = retaliate[key] and "RETALIATE" or "COWARD"

    local okDist, dist = pcall(function()
        return npc:distanceTo(other)
    end)

    print("[cowardly_scavengers_v1] perception"
        .. " npc=" .. nameOf(npc)
        .. " inst=" .. tostring(inst)
        .. " percType=" .. tostring(percType)
        .. " dist=" .. tostring(okDist and dist or "ERR")
        .. " mode=" .. mode)

    if retaliate[key] then
        print("[cowardly_scavengers_v1] action=ALLOW_DEFAULT reason=retaliate")
        return false
    end

    if percType ~= THREAT_PERC then
        print("[cowardly_scavengers_v1] action=ALLOW_DEFAULT reason=not_threat")
        return false
    end

    local gate = nextFleeAt[key] or 0.0
    if now < gate then
        print("[cowardly_scavengers_v1] action=BLOCK_DEFAULT reason=cooldown")
        return true
    end

    local okCall, okResult, errResult = pcall(function()
        return opengothic.ai.fleeFrom(npc, other)
    end)

    print("[cowardly_scavengers_v1] fleeFrom"
        .. " callOk=" .. tostring(okCall)
        .. " result=" .. tostring(okResult)
        .. " err=" .. tostring(errResult))

    if okCall and okResult == true then
        nextFleeAt[key] = now + FLEE_COOLDOWN_SECONDS
        print("[cowardly_scavengers_v1] action=FLEE_BLOCK_DEFAULT next=" .. tostring(nextFleeAt[key]))
        return true
    end

    print("[cowardly_scavengers_v1] action=ALLOW_DEFAULT reason=flee_failed")
    return false
end)

opengothic.events.register("onNpcTakeDamage", function(victim, attacker, isSpell, spellId)
    if not isScavenger(victim) then
        return false
    end

    local key = keyOf(victim)
    local attackerIsPlayer = attacker ~= nil and isPlayer(attacker)

    print("[cowardly_scavengers_v1] damage"
        .. " victim=" .. nameOf(victim)
        .. " attacker=" .. (attacker and nameOf(attacker) or "nil")
        .. " attackerIsPlayer=" .. tostring(attackerIsPlayer)
        .. " isSpell=" .. tostring(isSpell)
        .. " spellId=" .. tostring(spellId))

    if attackerIsPlayer then
        retaliate[key] = true
        nextFleeAt[key] = 0.0
        print("[cowardly_scavengers_v1] mode=RETALIATE npc=" .. nameOf(victim))
    end

    return false
end)

opengothic.events.register("onNpcDeath", function(victim, killer, isDeath)
    if isScavenger(victim) then
        local key = keyOf(victim)
        retaliate[key] = nil
        nextFleeAt[key] = nil
        print("[cowardly_scavengers_v1] cleanup npc=" .. nameOf(victim) .. " isDeath=" .. tostring(isDeath))
    end
    return false
end)

print("cowardly_scavengers loaded")
```

## Step 3: Explanation

This script uses a strict two-mode model:

- `COWARD`: scavenger tries to flee from player on threat perception.
- `RETALIATE`: after the player hits the scavenger, script stops blocking default combat behavior.

### Why `fleeFrom` is used

`opengothic.ai.fleeFrom(npc, target)` performs the required setup for reliable fleeing:

1. Set target.
2. Trigger flee.
3. Keep behavior script-side safe with `ok, err` return values.

### Why cooldown exists

`onNpcPerception` can fire repeatedly for the same NPC.
The cooldown prevents action spam while preserving deterministic behavior.

### Blocking behavior rule

The hook returns `true` only when the script successfully handled fleeing.
For all other paths it returns `false` so default engine behavior remains available.

## Step 4: Run the Game

Test flow:

1. Start or load a game with scavengers nearby.
2. Verify startup logs:
    - `symbol SCAVENGER -> ...`
    - `symbol YSCAVENGER -> ...`
    - `world loaded; behavior active`
3. Approach a scavenger without attacking:
    - expect `mode=COWARD`
    - expect `action=FLEE_BLOCK_DEFAULT`
4. Hit the scavenger once:
    - expect `mode=RETALIATE npc=...`
5. Stay near after the hit:
    - expect `action=ALLOW_DEFAULT reason=retaliate`
    - scavenger should use default combat behavior

If behavior does not match logs, keep the stdout lines and debug one transition at a time.

## Related Tutorials

- [Auto Pickup](./auto-pickup.md)
- [Dialog Time Windows](./dialog-time-windows.md)
- [Custom Damage Logic](./custom-damage.md)
