# World API

The `World` object is the main entry point for interacting with the game world as a whole. You can get the current world object by calling `opengothic.world()`.

---

## Primitives

These methods are implemented in C++ and provide direct access to world functionality. They use numeric IDs (from `opengothic.resolve()`) and do not call Daedalus functions.

### `world:time()`

Gets the current in-game time.

- **Returns**: `hour` (number), `minute` (number)

```lua
local h, m = opengothic.world():time()
opengothic.printMessage("Current time: " .. h .. ":" .. m)
```

---

### `world:isTime(startHour, startMin, endHour, endMin)`

Checks if the current game time is within a specified range. Correctly handles overnight ranges (for example, `20:00` to `06:00`).

- `startHour` (number): Start hour (0-23).
- `startMin` (number): Start minute (0-59).
- `endHour` (number): End hour (0-23).
- `endMin` (number): End minute (0-59).
- **Returns**: `boolean` - `true` if the current time is within the range (inclusive start, exclusive end).

```lua
local world = opengothic.world()

if world:isTime(22, 0, 6, 0) then
    print("It's nighttime!")
end
```

---

### `world:setDayTime(hour, minute)`

Sets the current in-game time.

- `hour` (number): The hour to set (0-23).
- `minute` (number): The minute to set (0-59).

```lua
-- Set the time to midnight
opengothic.world():setDayTime(0, 0)
```

---

### `world:day()`

Gets the current in-game day number.

- **Returns**: `day` (number)

---

### `world:addNpc(instanceId, waypoint)`

Spawns an NPC at a specific waypoint.

- `instanceId` (number): The instance ID of the NPC to spawn (use `opengothic.resolve()` to get this).
- `waypoint` (string): The name of the waypoint to spawn the NPC at (e.g., `"WP_START"`).
- **Returns**: The new `Npc` object, or `nil` if spawning failed.

```lua
local mudId = opengothic.resolve("PC_MUD")
if mudId then
    local newMud = opengothic.world():addNpc(mudId, "WP_CITY_CENTER")
    if newMud then
        newMud:setAttitude(opengothic.CONSTANTS.Attitude.ATT_FRIENDLY)
    end
end
```

---

### `world:addNpcAt(instanceId, x, y, z)`

Spawns an NPC at a specific world coordinate.

- `instanceId` (number): The instance ID of the NPC.
- `x`, `y`, `z` (numbers): The world coordinates.
- **Returns**: The new `Npc` object, or `nil` if spawning failed.

```lua
local playerPos = { opengothic.player():position() }
local wolfId = opengothic.resolve("WOLF")
-- Spawn a wolf 100 units in front of the player
opengothic.world():addNpcAt(wolfId, playerPos[1] + 100, playerPos[2], playerPos[3])
```

---

### `world:removeNpc(npc)`

Removes an NPC from the world.

- `npc` (Npc): The NPC object to remove.

---

### `world:addItem(instanceId, waypoint)`

Spawns an item in the world at a specific waypoint.

- `instanceId` (number): The instance ID of the item.
- `waypoint` (string): The name of the waypoint.
- **Returns**: The new `Item` object, or `nil` on failure.

---

### `world:addItemAt(instanceId, x, y, z)`

Spawns an item at a specific world coordinate.

- `instanceId` (number): The instance ID of the item.
- `x`, `y`, `z` (numbers): The world coordinates.
- **Returns**: The new `Item` object, or `nil` on failure.

---

### `world:removeItem(item)`

Removes an item from the world.

- `item` (Item): The item object to remove.

---

### `world:findNpc(instanceId, [n])`

Finds an NPC in the world by its instance ID.

- `instanceId` (number): The instance ID to search for.
- `n` (number, optional): The nth NPC to find (0-based). Defaults to 0. Useful if multiple NPCs have the same ID.
- **Returns**: An `Npc` object, or `nil` if not found.

```lua
local laresId = opengothic.resolve("PC_THIEF_LARES")
local lares = opengothic.world():findNpc(laresId)
if lares then
    -- Found him!
end
```

---

### `world:findItem(instanceId, [n])`

Finds an item in the world by its instance ID.

- `instanceId` (number): The instance ID to search for.
- `n` (number, optional): The nth item to find (0-based). Defaults to 0.
- **Returns**: An `Item` object, or `nil` if not found.

---

### `world:findInteractive(instanceId)`

Finds an interactive object (mobsi) in the world by its instance ID.

- `instanceId` (number): The instance ID to search for (e.g., `opengothic.resolve("MOBSI_CHEST_01")`).
- **Returns**: An `Interactive` object, or `nil` if not found.

---

### `world:player()`

Gets the player's `Npc` object. This is a shortcut for `opengothic.player()`.

- **Returns**: The player `Npc` object, or `nil`.

---

### `world:playSound(soundName)`

Plays a sound effect globally.

- `soundName` (string): The name of the sound effect file (without extension).

```lua
-- Play a magic sound
opengothic.world():playSound("MFX_HEAL_HEAL")
```

---

### `world:playEffect(effectName, x, y, z)`

Spawns a visual effect (VFX) at a specific world coordinate.

- `effectName` (string): The name of the effect (e.g., `"SpellFx_Lightning"`).
- `x`, `y`, `z` (numbers): The world coordinates to spawn the effect at.

---

### `world:spellDesc(spellId)`

Gets information about a spell.

- `spellId` (number): The ID of the spell (from `item.clsId()` on a rune or scroll).
- **Returns**: A table with spell properties, or `nil`.
  - `damagePerLevel` (number)
  - `damageType` (number)
  - `spellType` (number)
  - `timePerMana` (number)

```lua
local fireboltId = opengothic.resolve("ITRU_FIREBOLT")
local spellInfo = opengothic.world():spellDesc(fireboltId)
if spellInfo then
    print("Firebolt damage type: " .. spellInfo.damageType)
end
```

---

### `world:findNpcsInRange(x, y, z, range)`

Finds all NPCs within a specified radius of a world coordinate.

- `x`, `y`, `z` (numbers): The center point coordinates.
- `range` (number): The search radius in world units.
- **Returns**: A table (array) of `Npc` objects found within range. Returns an empty table if none found. Note: Any NPC at the exact center point (distance = 0) is included.

```lua
local world = opengothic.world()
local px, py, pz = opengothic.player():position()

-- Find all NPCs within 1000 units of the player
local nearbyNpcs = world:findNpcsInRange(px, py, pz, 1000)
for _, npc in ipairs(nearbyNpcs) do
    print("Nearby: " .. npc:displayName())
end
```

---

### `world:findNpcsNear(npc, range)`

Finds all NPCs within a specified radius of another NPC.

- `npc` (Npc): The NPC to search around.
- `range` (number): The search radius in world units.
- **Returns**: A table (array) of `Npc` objects found within range. Note: The center `npc` itself is included in the results (distance = 0).

```lua
local world = opengothic.world()
local player = opengothic.player()

local nearby = world:findNpcsNear(player, 500)
for _, npc in ipairs(nearby) do
    if not npc:isPlayer() then
        print("Found: " .. npc:displayName())
    end
end
```

---

### `world:findNearestNpc(originNpc, range)`

Finds the nearest NPC around an origin NPC.

- `originNpc` (Npc): The NPC used as search center.
- `range` (number): Search radius in world units.
- **Returns**: The nearest `Npc`, or `nil` if none is found.

Notes:
- The `originNpc` itself is excluded from results.
- Returns `nil` for invalid inputs (for example `range <= 0`).

```lua
local world = opengothic.world()
local player = opengothic.player()
local nearest = world:findNearestNpc(player, 1500)
if nearest then
    print("Nearest NPC: " .. nearest:displayName())
end
```

---

### `world:detectItemsInRange(x, y, z, range)`

Finds all world items within a specified radius of a world coordinate.

- `x`, `y`, `z` (numbers): The center point coordinates.
- `range` (number): The search radius in world units.
- **Returns**: A table (array) of `Item` objects found within range. Returns an empty table for invalid inputs or when no item is found.

```lua
local world = opengothic.world()
local px, py, pz = opengothic.player():position()

local nearbyItems = world:detectItemsInRange(px, py, pz, 1000)
for _, item in ipairs(nearbyItems) do
    print("Nearby item: " .. item:displayName())
end
```

---

### `world:detectItemsNear(originNpc, range)`

Finds all world items around an origin NPC.

- `originNpc` (Npc): The NPC used as search center.
- `range` (number): Search radius in world units.
- **Returns**: A table (array) of `Item` objects found within range. Returns an empty table for invalid inputs.

```lua
local world = opengothic.world()
local player = opengothic.player()

for _, item in ipairs(world:detectItemsNear(player, 800)) do
    print("Detected item: " .. item:displayName())
end
```

---

### `world:findNearestItem(originNpc, range)`

Finds the nearest world item around an origin NPC.

- `originNpc` (Npc): The NPC used as search center.
- `range` (number): Search radius in world units.
- **Returns**: The nearest `Item`, or `nil` if none is found.

Notes:
- Returns `nil` for invalid inputs (for example `range <= 0`).

```lua
local world = opengothic.world()
local player = opengothic.player()
local nearestItem = world:findNearestItem(player, 1200)
if nearestItem then
    print("Nearest item: " .. nearestItem:displayName())
end
```

---

## Convenience Methods

These methods are defined in `bootstrap.lua` and wrap world primitives with symbol-name resolution.

### `world:insertNpc(npcName, waypoint)`

Spawns an NPC at a waypoint using a symbol name instead of a resolved ID.

- `npcName` (string): The name of the NPC instance (e.g., `"PC_HERO"`).
- `waypoint` (string): The name of the waypoint.
- **Returns**: The new `Npc` object, or `nil` if spawning failed.

```lua
local world = opengothic.world()
local newNpc = world:insertNpc("GRD_200_GUARD", "WP_CITY_GATE")
if newNpc then
    print("Spawned guard at city gate")
end
```

---

### `world:insertItem(itemName, waypoint)`

Spawns an item at a waypoint using a symbol name instead of a resolved ID.

- `itemName` (string): The name of the item instance (e.g., `"ITMI_GOLD"`).
- `waypoint` (string): The name of the waypoint.
- **Returns**: The new `Item` object, or `nil` if spawning failed.

```lua
local world = opengothic.world()
local goldPile = world:insertItem("ITMI_GOLD", "WP_TREASURE_SPOT")
```

---
See also: [`opengothic.worldutil`](./helpers/world.md) for namespace-level world query wrappers.
