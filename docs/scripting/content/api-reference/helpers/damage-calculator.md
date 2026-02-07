# opengothic.DamageCalculator

The `opengothic.DamageCalculator` table exposes combat-damage methods.

---

### `opengothic.DamageCalculator.calculate(attacker, victim, isSpell, spellId)`

High-level helper that builds a damage array (including spell damage typing) and delegates to `damageValue(...)`.

- `attacker` (Npc): Attacker NPC.
- `victim` (Npc): Victim NPC.
- `isSpell` (boolean): Spell-vs-melee/ranged mode.
- `spellId` (number): Spell class ID for spell damage lookup.
- **Returns**: Result of `damageValue(...)`.

```lua
local value, hasHit = opengothic.DamageCalculator.calculate(attacker, victim, true, spellId)
print("Damage=" .. tostring(value) .. ", hit=" .. tostring(hasHit))
```

---

### `opengothic.DamageCalculator.damageTypeMask(npc)`

Low-level primitive helper for damage-type mask extraction.

- `npc` (Npc): NPC to inspect.
- **Returns**: `mask` (number)

---

### `opengothic.DamageCalculator.damageValue(attacker, victim, isSpell, damageTable)`

Low-level primitive damage evaluation helper.

- `attacker` (Npc): Attacker NPC.
- `victim` (Npc): Victim NPC.
- `isSpell` (boolean): Spell-vs-melee/ranged mode.
- `damageTable` (table): Damage values indexed by damage-type IDs.
- **Returns**: `value` (number), `hasHit` (boolean)

```lua
local dmg = {}
for i = 0, 7 do
    dmg[i] = 0
end
dmg[0] = 30

local value, hasHit = opengothic.DamageCalculator.damageValue(attacker, victim, false, dmg)
```
