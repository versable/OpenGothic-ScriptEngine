# opengothic.worldutil

The `opengothic.worldutil` table provides convenience wrappers built on world, NPC, and item primitives.

---

### `opengothic.worldutil.findNearestNpc(originNpc, range, [predicate])`

Finds the nearest NPC around `originNpc`, with optional filtering.

- `originNpc` (Npc): Origin NPC.
- `range` (number): Search radius in world units.
- `predicate` (function, optional): Filter callback `predicate(npc) -> boolean`.
- **Returns**: `Npc` or `nil`.

Behavior:
- Without `predicate`, it uses the primitive `world:findNearestNpc(...)`.
- With `predicate`, it scans `world:findNpcsNear(...)` and returns the nearest passing NPC.
- The origin NPC is excluded.

```lua
local player = opengothic.player()

local nearestAny = opengothic.worldutil.findNearestNpc(player, 2000)
local nearestHostile = opengothic.worldutil.findNearestNpc(player, 2000, function(npc)
    return npc:attitude() == opengothic.CONSTANTS.Attitude.ATT_HOSTILE
end)
```

---

### `opengothic.worldutil.findNearestItem(originNpc, range, [predicate])`

Finds the nearest item around `originNpc`, with optional filtering.

- `originNpc` (Npc): Origin NPC.
- `range` (number): Search radius in world units.
- `predicate` (function, optional): Filter callback `predicate(item) -> boolean`.
- **Returns**: `Item` or `nil`.

Behavior:
- Without `predicate`, it uses the primitive `world:findNearestItem(...)`.
- With `predicate`, it scans `world:detectItemsNear(...)` and returns the nearest passing item.

```lua
local player = opengothic.player()

local nearestAny = opengothic.worldutil.findNearestItem(player, 2000)
local nearestGold = opengothic.worldutil.findNearestItem(player, 2000, function(item)
    return item:isGold()
end)
```
