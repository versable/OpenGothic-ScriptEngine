# opengothic.ai

The `opengothic.ai` table provides high-level wrappers around NPC AI primitives.

All action helpers follow this return pattern:
- success: `true, nil`
- failure: `false, "error_code"`

---

### `opengothic.ai.attackTarget(npc, target)`

Sets `target` and pushes an attack action for `npc`.

- `npc` (Npc): Attacker.
- `target` (Npc): Target NPC.
- **Returns**: `ok` (boolean), `err` (`nil` or string)

```lua
local ok, err = opengothic.ai.attackTarget(guard, player)
if not ok then
    print("attackTarget failed: " .. err)
end
```

---

### `opengothic.ai.flee(npc)`

Pushes a flee action for `npc`.

- `npc` (Npc): NPC that should flee.
- **Returns**: `ok` (boolean), `err` (`nil` or string)

```lua
local ok, err = opengothic.ai.flee(npc)
```

---

### `opengothic.ai.fleeFrom(npc, target, opts)`

Sets `target` and pushes a flee action for `npc` as one atomic helper.

Optional `opts` fields:
- `setRunMode` (boolean, default `true`): If enabled, tries to set run walk mode before fleeing.
- `runMode` (number, optional): Explicit walk mode override.

- `npc` (Npc): NPC that should flee.
- `target` (Npc): NPC to flee from.
- `opts` (table|nil): Optional behavior flags.
- **Returns**: `ok` (boolean), `err` (`nil` or string)

```lua
local ok, err = opengothic.ai.fleeFrom(scavenger, opengothic.player())
if not ok then
    print("fleeFrom failed: " .. err)
end
```

---

### `opengothic.ai.reset(npc)`

Clears queued AI and target state for `npc`.

- `npc` (Npc): NPC to reset.
- **Returns**: `ok` (boolean), `err` (`nil` or string)

```lua
local ok, err = opengothic.ai.reset(npc)
```

---

### `opengothic.ai.isCombatReady(npc)`

Checks if an NPC appears combat-ready.

Current checks:
- valid NPC userdata
- not dead
- not down

- `npc` (Npc): NPC to test.
- **Returns**: `boolean`

```lua
if opengothic.ai.isCombatReady(enemy) then
    opengothic.ai.attackTarget(enemy, opengothic.player())
end
```
