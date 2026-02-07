# Item API

The `Item` object represents an item instance, either in the world or in an inventory.

---

### `item:displayName()`
- **Returns**: `name` (string) - The item's display name.

### `item:description()`
- **Returns**: `description` (string) - The item's description text.

### `item:position()`
- **Returns**: `x`, `y`, `z` (number, number, number) - Current world position. For inventory items this is usually `0, 0, 0`.

### `item:clsId()`
- **Returns**: `id` (number) - The item's class/instance ID. This is the same ID you get from `opengothic.resolve()`.

---

## Counts and Cost

### `item:count()`
- **Returns**: `count` (number) - The number of items in this stack. For non-stackable items, this is 1.

### `item:setCount(n)`
- `n` (number): The new stack count for the item.

### `item:cost()`
- **Returns**: `value` (number) - The base value of the item in ore.

### `item:sellCost()`
- **Returns**: `value` (number) - The value of the item when sold by the player.

### `item:weight()`
- **Returns**: `weight` (number) - The weight of a single item in the stack.

---

## Item Properties (Booleans)

### `item:isEquipped()`
- **Returns**: `boolean` - `true` if currently equipped by the owner.

### `item:isMission()`
- **Returns**: `boolean` - `true` if this is a mission item.

### `item:isGold()`
- **Returns**: `boolean` - `true` if this is ore/gold.

### `item:isMulti()`
- **Returns**: `boolean` - `true` if this item is stackable.

### `item:is2H()`
- **Returns**: `boolean` - `true` if this is a two-handed weapon.

### `item:isCrossbow()`
- **Returns**: `boolean` - `true` if this is a crossbow.

### `item:isRing()`
- **Returns**: `boolean` - `true` if this is a ring.

### `item:isArmor()`
- **Returns**: `boolean` - `true` if this is armor.

### `item:isSpell()`
- **Returns**: `boolean` - `true` if this is a spell scroll.

### `item:isRune()`
- **Returns**: `boolean` - `true` if this is a rune.

### `item:isSpellOrRune()`
- **Returns**: `boolean` - `true` if this is a spell or rune.

### `item:isSpellShoot()`
- **Returns**: `boolean` - `true` if this is a projectile spell item.

```lua
local item = opengothic.player():activeWeapon()
if item and item:is2H() then
    print("Player is using a two-handed weapon.")
end
```

---

## Combat Properties

### `item:damage()`
- **Returns**: `damage` (number) - The item's total damage value.

### `item:damageType()`
- **Returns**: `type` (number) - The bitmask of damage types. Use with `opengothic.CONSTANTS.Protection`.

### `item:protection(protectionId)`
- `protectionId` (number): The ID of the protection type (`PROT_EDGE`, `PROT_FIRE`, etc.).
- **Returns**: `value` (number) - The protection this item provides against a damage type.

```lua
-- Find equipped armor by iterating the player's inventory
local player = opengothic.player()
local inv = player:inventory()
for _, item in ipairs(inv:items()) do
    if item:isArmor() and item:isEquipped() then
        local fireProt = item:protection(opengothic.CONSTANTS.Protection.PROT_FIRE)
        print("Armor fire protection: " .. fireProt)
        break
    end
end
```

### `item:range()`
- **Returns**: `range` (number) - The range of the weapon.

---

### `item:flags()`
- **Returns**: `flags` (number) - The raw item flags bitmask.
