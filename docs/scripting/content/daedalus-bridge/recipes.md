# Daedalus Bridge Recipes

These recipes assume you already know Daedalus and want robust Lua-side interop patterns.

## Recipe 1: Use Built-In Non-Throwing Bridge Wrappers

Prefer the built-in wrappers over ad-hoc `pcall` wrappers:

```lua
local ok, result, err = opengothic.daedalus.tryCall("B_GivePlayerXP", 250)
if not ok then
    print("[bridge] B_GivePlayerXP failed: " .. tostring(err))
end

local setOk, setErr = opengothic.daedalus.trySet("MY_QUEST_STATE", 2)
if not setOk then
    print("[bridge] MY_QUEST_STATE write failed: " .. tostring(setErr))
end
```

Use these helpers when partial failure is acceptable and you do not want script termination.

## Recipe 2: Call AI State with Explicit Context

```lua
local function forceAttack(attacker, target)
    if not attacker or not target then
        return false
    end

    local ok, _, err = opengothic.vm.callSelfOther("ZS_Attack", attacker, target)
    if not ok then
        print("[bridge] ZS_Attack failed: " .. tostring(err))
        return false
    end

    return true
end
```

Use this when a Daedalus function depends on `SELF`/`OTHER` semantics.

## Recipe 3: Register Lua External for Daedalus Trigger

Lua side:

```lua
opengothic.vm.registerExternal("Lua_OnSpecialLever", function()
    local player = opengothic.player()
    if not player then
        return 0
    end

    if player:level() >= 10 then
        opengothic.ui.notify("The mechanism responds.")
        return 1
    end

    opengothic.ui.notify("You are not experienced enough.")
    return 0
end)
```

Daedalus side declaration/call:

```dae
func int Lua_OnSpecialLever();

func void ON_LEVER_TRIGGER() {
    var int result;
    result = Lua_OnSpecialLever();
    if (result == 1) {
        // follow-up logic
    };
};
```

Current bridge limitation: Lua external callback is invoked without arguments.

## Recipe 4: Enumerate Symbol Class for Diagnostics

```lua
local count = 0

opengothic.vm.enumerate("C_NPC", function(symbol)
    count = count + 1
    if count <= 20 then
        print("[C_NPC] " .. symbol.name .. " (#" .. tostring(symbol.index) .. ")")
    end
    return true
end)

print("[C_NPC] total scanned: " .. tostring(count))
```

Use this for migration audits and symbol sanity checks.

## Recipe 5: Prefer Convenience API First, Bridge as Fallback

```lua
local function giveItemCompat(npc, itemSymbol, count)
    count = count or 1

    -- Preferred path
    local ok = npc:giveItem(itemSymbol, count)
    return ok
end
```

Use convenience APIs directly for gameplay helpers that no longer rely on raw external passthrough.

## Recipe 6: Session-Aware Symbol Cache

```lua
local HERO_ID = nil

local function refreshSymbolCache()
    HERO_ID = opengothic.resolve("PC_HERO")
end

opengothic.events.register("onWorldLoaded", function()
    refreshSymbolCache()
    return false
end)
```

For bridge-heavy mods, refresh cached symbol IDs on `onWorldLoaded` instead of assuming static IDs before session start.

## Recipe 7: Incremental Migration with `tryCall` + `callSelfOther`

```lua
local function runLegacyAttack(attacker, target)
    -- 1) Prefer higher-level wrapper when behavior is equivalent
    local ok, err = opengothic.ai.attackTarget(attacker, target)
    if ok then
        return true
    end

    -- 2) Fallback to exact Daedalus AI state with safe context wrapper
    local ctxOk, _, ctxErr = opengothic.vm.callSelfOther("ZS_Attack", attacker, target)
    if ctxOk then
        return true
    end

    -- 3) Optional legacy side-effect call without throwing
    local callOk, _, callErr = opengothic.daedalus.tryCall("MY_LEGACY_ATTACK_HELPER", attacker, target)
    if not callOk then
        print("[bridge] fallback failed: " .. tostring(ctxErr) .. " / " .. tostring(callErr))
        return false
    end

    return false
end
```

This pattern keeps migration robust:

1. convenience API first
2. context-safe bridge fallback
3. non-throwing legacy call path
