# Tutorial: Custom Damage Logic

This tutorial shows how to intercept incoming damage and replace it with custom logic.

Our script will:

1. Detect attacks coming from a specific attacker type.
2. Apply a custom poison-style damage profile.
3. Make a specific victim type extra vulnerable.

## Step 1: Create Your Script File

Create a new Lua file:

**File Location:** `Data/opengothic/scripts/custom_damage.lua`

## Step 2: Add the Lua Code

Open `custom_damage.lua` and add:

```lua
-- custom_damage.lua

local ATTACKER_SYMBOL = "PC_HERO"
local VULNERABLE_SYMBOL = "SHEEP"

local attackerId = nil
local vulnerableId = nil
local symbolByIndex = {}

local function notify(text)
    local ok = opengothic.ui.notify(text)
    if not ok then
        opengothic.printMessage(text)
    end
end

local function refreshSymbols()
    symbolByIndex = {}
    opengothic.vm.enumerate("", function(sym)
        symbolByIndex[sym.index] = sym.name
        return true
    end)

    attackerId = opengothic.resolve(ATTACKER_SYMBOL)
    vulnerableId = opengothic.resolve(VULNERABLE_SYMBOL)
    print("[custom_damage] attacker=" .. tostring(ATTACKER_SYMBOL) .. " -> " .. tostring(attackerId))
    print("[custom_damage] vulnerable=" .. tostring(VULNERABLE_SYMBOL) .. " -> " .. tostring(vulnerableId))
end

opengothic.events.register("onWorldLoaded", function()
    refreshSymbols()
    return false
end)

local function buildDamageTable(baseDamage)
    local dmg = {}
    for i = 0, opengothic.CONSTANTS.Protection.PROT_MAX - 1 do
        dmg[i] = 0
    end

    -- Split poison-like custom damage across magic and fire channels.
    dmg[opengothic.CONSTANTS.Protection.PROT_MAGIC] = math.floor(baseDamage / 2)
    dmg[opengothic.CONSTANTS.Protection.PROT_FIRE] = baseDamage - dmg[opengothic.CONSTANTS.Protection.PROT_MAGIC]
    return dmg
end

opengothic.events.register("onNpcTakeDamage", function(victim, attacker, incomingIsSpell, spellId)
    if attackerId == nil or vulnerableId == nil then
        refreshSymbols()
    end

    -- Not our attacker: keep default engine damage flow.
    if not attackerId or attacker:instanceId() ~= attackerId then
        return false
    end

    local baseDamage = 20

    -- Selected victim type takes triple poison damage.
    if vulnerableId and victim:instanceId() == vulnerableId then
        baseDamage = baseDamage * 3
    end

    local damageTable = buildDamageTable(baseDamage)

    local finalDamage, hasHit = opengothic.DamageCalculator.damageValue(
        attacker,
        victim,
        true, -- force custom spell-table path so our damageTable is always used
        damageTable
    )

    local attackerSymbol = symbolByIndex[attacker:instanceId()]
    local victimSymbol = symbolByIndex[victim:instanceId()]
    local fireProt = victim:protection(opengothic.CONSTANTS.Protection.PROT_FIRE)
    local magicProt = victim:protection(opengothic.CONSTANTS.Protection.PROT_MAGIC)
    print(
        "[custom_damage] attacker=" .. tostring(attackerSymbol)
        .. " victim=" .. tostring(victimSymbol)
        .. " incomingIsSpell=" .. tostring(incomingIsSpell)
        .. " forcedSpellPath=true"
        .. " base=" .. tostring(baseDamage)
        .. " fireProt=" .. tostring(fireProt)
        .. " magicProt=" .. tostring(magicProt)
        .. " final=" .. tostring(finalDamage)
        .. " hit=" .. tostring(hasHit)
    )

    if hasHit and finalDamage > 0 then
        victim:changeAttribute(opengothic.CONSTANTS.Attribute.ATR_HITPOINTS, -finalDamage, true)
        notify(victim:displayName() .. " took " .. tostring(finalDamage) .. " poison damage")
    else
        notify(victim:displayName() .. " resisted poison damage")
    end

    return true -- we handled this damage event; skip default damage path
end)

print("custom_damage loaded")
```

## Step 3: Explanation

This implementation uses event interception and the damage helper API:

- **`onNpcTakeDamage`**:
  - Called before default damage handling.
  - Returning `true` means your script fully handled damage and blocks default processing.
- **`opengothic.DamageCalculator.damageValue(...)`**:
  - Evaluates your custom per-type damage table against target protections.
  - Returns `finalDamage, hasHit`.
  - This tutorial forces the spell-table path (`isSpell=true`) so the custom table is applied for every incoming hit.
- **`victim:changeAttribute(...)`**:
  - Applies final HP change after your custom calculation.

### Symbol resolution

- IDs are resolved via `opengothic.resolve(...)`.
- `refreshSymbols()` is called on `onWorldLoaded` and lazily in the damage handler to stay robust across sessions.
- This tutorial uses concrete defaults that work in normal Gothic 2 starts:
  - `ATTACKER_SYMBOL = "PC_HERO"`
  - `VULNERABLE_SYMBOL = "SHEEP"`
- `refreshSymbols()` also builds an index-to-symbol map so debug output can print symbol names for attacker/victim.

### Custom damage table

- Damage table indices map to `opengothic.CONSTANTS.Protection`.
- This example splits custom damage between `PROT_MAGIC` and `PROT_FIRE`.
- Debug output includes victim `PROT_MAGIC`/`PROT_FIRE` values so final-damage results are explainable from logs.

### Blocking default damage

- This tutorial intentionally uses `return true` after custom application.
- If you return `false`, the default engine damage path will also run.

## Step 4: Run the Game

Test flow:

1. Start or load a game and check logs for:
    - `[custom_damage] attacker=PC_HERO -> ...`
    - `[custom_damage] vulnerable=SHEEP -> ...`
2. Attack any NPC as the player and verify poison messages appear.
3. Attack a `SHEEP` (near Xardas' tower start area) and verify damage is higher than normal targets.
4. Check stdout log lines for each hit:
    - attacker symbol
    - victim symbol
    - base damage
    - final damage
    - hit result
5. Temporarily change `return true` to `return false` to observe default+custom stacking behavior (for testing only).

## Related Tutorials

- [Cowardly Scavengers](./scavenger-flee.md)
- [Quickloot](./quickloot.md)
- [Creating a Simple Quest](./creating-a-quest.md)
