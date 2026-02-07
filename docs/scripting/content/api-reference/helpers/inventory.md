# opengothic.inventory

The `opengothic.inventory` table provides high-level wrappers around inventory transfer primitives.

---

### `opengothic.inventory.hasItem(owner, itemRef, [minCount])`

Checks whether an owner has at least `minCount` of an item.

- `owner` (Npc or Inventory): NPC owner or direct inventory object.
- `itemRef` (string or number): Item symbol name (`"ITFO_APPLE"`) or numeric item class ID.
- `minCount` (number, optional): Minimum amount required. Default `1`.
- **Returns**: `boolean`

```lua
local player = opengothic.player()

if opengothic.inventory.hasItem(player, "ITKE_KEY_01") then
    print("Player has the key")
end

local inv = player:inventory()
local appleId = opengothic.resolve("ITFO_APPLE")
if opengothic.inventory.hasItem(inv, appleId, 2) then
    print("At least two apples in inventory")
end
```

---

### `opengothic.inventory.transferAll(dstNpc, srcInv, [opts])`

Transfers all matching items from `srcInv` to `dstNpc:inventory()`.

- `dstNpc` (Npc): Destination NPC.
- `srcInv` (Inventory): Source inventory.
- `opts` (table, optional):
  - `includeEquipped` (boolean, optional): Default `false`.
  - `includeMission` (boolean, optional): Default `true`.
- **Returns**: `items` (table), `err` (`nil` or string)

```lua
local items, err = opengothic.inventory.transferAll(opengothic.player(), chest:inventory(), {
    includeEquipped = false,
    includeMission = true
})
if err then
    print("Transfer failed: " .. err)
end
```

---

### `opengothic.inventory.transferByPredicate(dstNpc, srcInv, predicate, [opts])`

Transfers only items for which `predicate(item)` returns `true`.

- `dstNpc` (Npc): Destination NPC.
- `srcInv` (Inventory): Source inventory.
- `predicate` (function): Item filter callback.
- `opts` (table, optional): Same as `transferAll`.
- **Returns**: `items` (table), `err` (`nil` or string)

```lua
local items, err = opengothic.inventory.transferByPredicate(
    opengothic.player(),
    chest:inventory(),
    function(item)
        return item:isGold() or item:isRune()
    end,
    { includeMission = false }
)
if not err then
    print("Filtered transfer stacks: " .. tostring(#items))
end
```
