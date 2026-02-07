# Npc API

The `Npc` object represents a character in the world, including the player, allies, and enemies. It is one of the most important objects in the API.

---

### `npc:displayName()`

- **Returns**: `name` (string) - The NPC's display name.

### `npc:instanceId()`

- **Returns**: `id` (number) - The NPC's Daedalus instance symbol ID. Note: This is the **type** identifier, not a unique ID. All NPCs spawned from the same Daedalus instance (e.g., all "SCAVENGER" NPCs) share the same `instanceId()`.

### `npc:world()`

- **Returns**: `World` - The `World` object this NPC belongs to.

### `npc:inventory()`

- **Returns**: `Inventory` - The NPC's `Inventory` object.

---

## Attributes and Stats

### `npc:attribute(attributeId)`

Gets the value of a primary attribute. Use `opengothic.CONSTANTS.Attribute`.

- `attributeId` (number): The ID of the attribute.
- **Returns**: `value` (number).

```lua
local player = opengothic.player()
local str = player:attribute(opengothic.CONSTANTS.Attribute.ATR_STRENGTH)
print("Player strength: " .. str)
```

### `npc:changeAttribute(attributeId, delta, allowUnconscious)`

Changes an attribute by a relative amount.

- `attributeId` (number): The ID of the attribute.
- `delta` (number): The amount to add or subtract.
- `allowUnconscious` (boolean): If `true`, a negative change to health can cause unconsciousness but not death.

```lua
-- Give the player 10 mana
local player = opengothic.player()
player:changeAttribute(opengothic.CONSTANTS.Attribute.ATR_MANA, 10, true)
```

### `npc:setHealth(value)`

Sets the NPC's health to an absolute value.

- `value` (number): The new health value.

### `npc:level()`
- **Returns**: `level` (number)

### `npc:experience()`
- **Returns**: `experience` (number)

### `npc:learningPoints()`
- **Returns**: `lp` (number)

### `npc:guild()`
- **Returns**: `guildId` (number) - The NPC's guild ID.

### `npc:protection(protectionId)`

Gets the NPC's protection value against a specific damage type. Use `opengothic.CONSTANTS.Protection`.

- `protectionId` (number): The ID of the protection type.
- **Returns**: `value` (number).

```lua
local fireProtection = opengothic.player():protection(opengothic.CONSTANTS.Protection.PROT_FIRE)
print("Player fire protection: " .. fireProtection)
```

### `npc:talentSkill(talentId)`

Gets the NPC's skill level in a talent (e.g., One-Handed). Use `opengothic.CONSTANTS.Talent`.

- `talentId` (number): The ID of the talent.
- **Returns**: `level` (number).

### `npc:setTalentSkill(talentId, level)`

Sets the NPC's talent skill level.

- `talentId` (number): The ID of the talent.
- `level` (number): The new skill level.

### `npc:talentValue(talentId)`

Gets the NPC's raw value in a talent (e.g., Lockpicking value).

- `talentId` (number): The ID of the talent.
- **Returns**: `value` (number).

### `npc:hitChance(talentId)`

Gets the NPC's hit chance for a given weapon talent.

- `talentId` (number): The ID of the talent (`TALENT_1H`, `TALENT_2H`, etc.).
- **Returns**: `chance` (number) - The hit chance from 0 to 100.

---

## State and Position

### `npc:isDead()`
- **Returns**: `boolean` - `true` if the NPC is dead.

### `npc:isUnconscious()`
- **Returns**: `boolean` - `true` if the NPC is unconscious.

### `npc:isDown()`
- **Returns**: `boolean` - `true` if the NPC is dead OR unconscious.

### `npc:isPlayer()`
- **Returns**: `boolean` - `true` if this NPC is the player.

### `npc:isTalking()`
- **Returns**: `boolean` - `true` if the NPC is currently in talking state.

### `npc:bodyState()`
- **Returns**: `state` (number) - The raw body state bitmask. Use `hasState` for easier checks.

### `npc:hasState(stateId)`
- `stateId` (number): The body state to check. Use `opengothic.CONSTANTS.BodyState`.
- **Returns**: `boolean` - `true` if the NPC has the specified state.

```lua
if someNpc:hasState(opengothic.CONSTANTS.BodyState.BS_SNEAK) then
    print(someNpc:displayName() .. " is sneaking!")
end
```

### `npc:position()`

- **Returns**: `x` (number), `y` (number), `z` (number) - The NPC's world coordinates.

### `npc:setPosition(x, y, z)`

Teleports the NPC to the specified world coordinates.

- `x`, `y`, `z` (numbers): The destination coordinates.

### `npc:rotationY()`
- **Returns**: `angle` (number) - The NPC's rotation on the Y-axis (facing direction).

### `npc:rotation()`
- **Returns**: `angle` (number) - The NPC's full rotation value.

### `npc:setDirectionY(angle)`
- `angle` (number): The new Y-axis rotation.

### `npc:walkMode()`
- **Returns**: `mode` (number) - The raw walk mode bitmask.

### `npc:setWalkMode(mode)`
- `mode` (number): The new walk mode. Use `opengothic.CONSTANTS.WalkBit`.

```lua
-- Force an NPC to run
local world = opengothic.world()
local npcId = opengothic.resolve("VLK_400_BUDDLER")
local someNpc = npcId and world and world:findNpc(npcId)
if someNpc then
    someNpc:setWalkMode(opengothic.CONSTANTS.WalkBit.WM_Run)
end
```

---

## Social and AI

### `npc:attitude()`
- **Returns**: `attitude` (number) - The NPC's attitude towards the player. See `opengothic.CONSTANTS.Attitude`.

### `npc:setAttitude(attitude)`
- `attitude` (number): The new attitude.

---

## Equipment and Spells

### `npc:item(instanceId)`

Gets a specific item from the NPC's inventory.

- `instanceId` (number): The instance ID of the item.
- **Returns**: `Item` object, or `nil` if not found.

### `npc:activeWeapon()`
- **Returns**: `Item` object representing the currently equipped weapon, or `nil`.

### `npc:activeSpell()`
- **Returns**: `spellId` (number) - The class ID of the active spell, or -1 if none.

---

## AI Control

### `npc:distanceTo(other)`

Calculates the distance to another NPC.

- `other` (Npc): The other NPC.
- **Returns**: `distance` (number) - Distance in world units, or -1 if either NPC is invalid.

```lua
local player = opengothic.player()
local dist = npc:distanceTo(player)
if dist < 1000 then
    print("NPC is close to player")
end
```

---

### `npc:flee()`

Makes the NPC flee from their current threat. Pushes a flee action to the AI queue.

```lua
-- Make NPC run away
npc:flee()
```

---

### `npc:setTarget(other)`

Sets the NPC's combat target.

- `other` (Npc or nil): The target NPC, or `nil` to clear the target.

```lua
npc:setTarget(player)
npc:attack()
```

---

### `npc:target()`

Gets the NPC's current combat target.

- **Returns**: `Npc` or `nil`.

```lua
local target = npc:target()
if target then
    print(npc:displayName() .. " targets " .. target:displayName())
end
```

---

### `npc:setPerceptionTime(timeMs)`

Sets the NPC perception polling interval in milliseconds.

- `timeMs` (number): Interval in milliseconds. Values below 0 are clamped to 0.

```lua
-- Poll perceptions more frequently while debugging
npc:setPerceptionTime(500)
```

---

### `npc:attack()`

Makes the NPC attack their current target. Pushes an attack action to the AI queue.

```lua
npc:setTarget(enemy)
npc:attack()
```

---

### `npc:clearAI()`

Clears all pending AI actions from the NPC's action queue.

```lua
-- Stop current behavior and start fresh
npc:clearAI()
npc:flee()
```

---

## Convenience Methods

These methods are defined in `bootstrap.lua` and compose available scripting primitives.

### `npc:callDaedalus(funcName, ...)`

Calls a Daedalus function with this NPC set as the `self` context.

- `funcName` (string): The name of the Daedalus function to call.
- `...`: Additional arguments to pass to the function.
- **Returns**: The return value from the Daedalus function.

```lua
local player = opengothic.player()
player:callDaedalus("B_GivePlayerXP", 100)
```

---

### `npc:giveItem(itemName, count)`

Adds items to the NPC inventory.

- `itemName` (string): The name of the item instance (e.g., `"ITMI_GOLD"`).
- `count` (number, optional): The number of items to give. Defaults to 1.
- **Returns**: `boolean` - `true` if successful.

```lua
local player = opengothic.player()
player:giveItem("ITMI_GOLD", 100)
player:giveItem("ITAR_MIL_L")
```

---

### `npc:hasItem(itemName, minCount)`

Checks if the NPC has at least a specified number of an item.

- `itemName` (string): The name of the item instance.
- `minCount` (number, optional): The minimum count required. Defaults to 1.
- **Returns**: `boolean` - `true` if the NPC has at least `minCount` of the item.

Notes:
- Delegates to `opengothic.inventory.hasItem(...)` convenience helper.

```lua
local player = opengothic.player()
if player:hasItem("ITKE_KEY_01") then
    print("Player has the key!")
end

if player:hasItem("ITMI_GOLD", 100) then
    print("Player has enough gold")
end
```

---

## Convenience and Utility Methods

These methods are available on `Npc` for common scripting tasks.

### `npc:takeAllFrom(srcInventory, [world], [includeEquipped], [includeMission])`

Transfers item stacks from `srcInventory` to this NPC's inventory.

- `srcInventory` (Inventory): Source inventory.
- `world` (World, optional): World context for transfer. Defaults to the NPC's world.
- `includeEquipped` (boolean, optional): Include equipped items. Default: `false`.
- `includeMission` (boolean, optional): Include mission items. Default: `true`.
- **Returns**: `transferredItems` (table) - A list of transferred stack records with:
  - `id` (number): Item class ID.
  - `count` (number): Transferred stack count.
  - `name` (string): Display name.

```lua
local player = opengothic.player()
local chest = world:findInteractive(opengothic.resolve("CHEST_01")) -- Assuming 'world' is available
if chest then
    local transferred = player:takeAllFrom(chest:inventory())
    for _, itemRecord in ipairs(transferred) do
        print("Took " .. itemRecord.count .. "x " .. itemRecord.name .. " (id=" .. itemRecord.id .. ")")
    end
end
```

---

### `npc:isInstance(instanceName)`

Checks if this NPC matches a specific instance name. Uses `npc:instanceId()` and `opengothic.resolve()` primitives internally.

- `instanceName` (string): The name of the instance to check (e.g., `"PC_HERO"`, `"GRD_200_GUARD"`).
- **Returns**: `boolean` - `true` if this NPC's instance ID matches the resolved name.

```lua
opengothic.events.register("onDialogStart", function(npc, player)
    if npc:isInstance("YOURSPECIALNPC") then
        -- This is a specific NPC
        opengothic.printMessage("You found the special NPC!")
    end
    return false
end)
```

---

### `npc:isSneaking()`

Checks if this NPC is currently sneaking. Uses `npc:walkMode()` primitive internally.

- **Returns**: `boolean` - `true` if the NPC is sneaking.

```lua
local player = opengothic.player()
if player:isSneaking() then
    print("Player is sneaking")
end
```
