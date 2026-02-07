# Inventory API

The `Inventory` object represents a collection of items. It can belong to an NPC or an interactive object like a chest. You can get an inventory object by calling `npc:inventory()` or `interactive:inventory()`.

---

### `inventory:items()`

Returns a list of all items in the inventory.

- **Returns**: `items` (table) - A list of `Item` objects.

```lua
local playerInv = opengothic.player():inventory()
local items = playerInv:items()

print("Player has " .. #items .. " item stacks.")

for i, item in ipairs(items) do
    if item:isGold() then
        print("Found " .. item:count() .. " ore!")
    end
end
```

---

### `inventory:itemCount(instanceId)`

Counts the total number of a specific item in the inventory.

- `instanceId` (number): The instance ID of the item to count.
- **Returns**: `count` (number).

```lua
local playerInv = opengothic.player():inventory()
local healingPotionId = opengothic.resolve("ITPO_HEAL")

local potionCount = playerInv:itemCount(healingPotionId)
print("Player has " .. potionCount .. " healing potions.")
```

---

### `inventory:addItem(itemId, count)`

Adds a new item stack to this inventory. This method allows you to add items directly to an inventory without using the Daedalus bridge.

- `itemId` (number): The instance ID of the item to add (use `opengothic.resolve()` to get this).
- `count` (number): The number of items to add.
- **Returns**: The newly added `Item` object, or `nil` if creation or adding failed.

**Note:** This method throws an error if no world is currently loaded (e.g., during script initialization). Use it only inside event handlers or after a world has been loaded.

```lua
local playerInv = opengothic.player():inventory()
local goldId = opengothic.resolve("ITMI_GOLD")

local newGold = playerInv:addItem(goldId, 100)
if newGold then
    opengothic.printMessage("Added 100 gold to inventory!")
end
```

---

### `inventory:transfer(sourceInventory, itemId, count, world)`

Transfers a number of items from a source inventory to this inventory. This is a **primitive** - a low-level building block that gives you direct control. For most cases, using **convenience methods** like `Npc:giveItem()` or `Npc:takeAllFrom()` is recommended. See [Architecture](../concepts/architecture.md) for more on the three-layer API design.

- `sourceInventory` (Inventory): The inventory to take items from.
- `itemId` (number): The instance ID of the item to transfer.
- `count` (number): The number of items to transfer.
- `world` (World): The current world object.
- **Returns**: `success` (boolean).

```lua
-- Low-level example: transfer 10 arrows from a chest to the player
local world = opengothic.world()
local chestId = opengothic.resolve("CHEST_01")
local chest = chestId and world:findInteractive(chestId)
local player = opengothic.player()

if world and chest and player then
    local chestInv = chest:inventory()
    local playerInv = player:inventory()
    local arrowId = opengothic.resolve("ITRW_ARROW")

    -- Transfers 10 arrows from chest to player
    playerInv:transfer(chestInv, arrowId, 10, world)
end
```

---

### `inventory:transferAll(sourceInventory, [world], [includeEquipped], [includeMission])`

Transfers all matching stacks from a source inventory into this inventory.

- `sourceInventory` (Inventory): Source inventory.
- `world` (World, optional): World context. Defaults to the active world.
- `includeEquipped` (boolean, optional): Include equipped stacks. Default: `false`.
- `includeMission` (boolean, optional): Include mission stacks. Default: `true`.
- **Returns**: `items` (table) - A list of moved stack records:
  - `id` (number): Item class ID.
  - `count` (number): Stack count.
  - `name` (string): Display name.

```lua
local player = opengothic.player()
local world = opengothic.world()
local chest = world and world:findInteractive(opengothic.resolve("CHEST_01"))

if player and chest then
    local moved = player:inventory():transferAll(chest:inventory(), world, false, true)
    print("Moved stacks: " .. tostring(#moved))
end
```

---
See also: [`opengothic.inventory`](./helpers/inventory.md) for namespace-level transfer wrappers.
