# Events Reference

The event system allows Lua scripts to respond to game events. Register handlers using `opengothic.events.register()`.

## Registering Event Handlers

```lua
local handlerId = opengothic.events.register("eventName", function(...)
    -- Handle the event
    return false
end)
```

- `opengothic.events.register(eventName, callback) -> handlerId|nil`
- `opengothic.events.unregister(eventName, handlerId) -> boolean`

Multiple handlers can be registered for the same event. They are called in registration order until one returns `true`.

### Handler Return Semantics

- **Blocking hooks:** returning `true` blocks the default engine behavior.
- **Notification hooks:** the engine ignores the final handled state, but returning `true` still stops later Lua handlers in the same event chain.
- For composable observer-style handlers, return `false`/`nil`.

---

## Lifecycle Events

### `onStartGame`

Called when a new game is started.

- `worldName` (string): The name of the starting world.

```lua
opengothic.events.register("onStartGame", function(worldName)
    print("Starting new game in " .. worldName)
end)
```

---

### `onLoadGame`

Called when a saved game is loaded.

- `savegameName` (string): The name of the save file.

```lua
opengothic.events.register("onLoadGame", function(savegameName)
    print("Loading save: " .. savegameName)
end)
```

---

### `onSaveGame`

Called when the game is saved.

- `slotName` (string): The save slot identifier.
- `userName` (string): The user-provided save name.

```lua
opengothic.events.register("onSaveGame", function(slotName, userName)
    print("Saving to " .. slotName .. ": " .. userName)
end)
```

---

### `onWorldLoaded`

Called after the world has finished loading. This is the ideal place to initialize world-dependent systems.

**Parameters:** None

```lua
opengothic.events.register("onWorldLoaded", function()
    print("World loaded!")
end)
```

---

### `onStartLoading`

Called when a loading screen begins.

**Parameters:** None

---

### `onSessionExit`

Called when the game session is ending (returning to menu).

**Parameters:** None

---

### `onSettingsChanged`

Called when game settings are modified.

**Parameters:** None

---

### `onUpdate`

Called once per script update tick while a game session is active.

- `dt` (number): Delta time in seconds.

```lua
opengothic.events.register("onUpdate", function(dt)
    return false
end)
```

---

### `onGameMinuteChanged`

Called when in-game time advances to a new minute.

- `day` (number): Current in-game day.
- `hour` (number): Current in-game hour.
- `minute` (number): Current in-game minute.

```lua
opengothic.events.register("onGameMinuteChanged", function(day, hour, minute)
    print("Game time: day " .. day .. ", " .. hour .. ":" .. minute)
    return false
end)
```

---

## NPC Events

### `onNpcSpawn`

Called when an NPC is spawned into the world.

- `npc` (Npc): The spawned NPC.

```lua
opengothic.events.register("onNpcSpawn", function(npc)
    print(npc:displayName() .. " has spawned")
end)
```

---

### `onNpcRemove`

Called when an NPC is removed from the world.

- `npc` (Npc): The NPC being removed.

---

### `onNpcTakeDamage`

Called when an NPC takes damage. Return `true` to prevent the default damage.

- `victim` (Npc): The NPC taking damage.
- `attacker` (Npc): The NPC dealing damage.
- `isSpell` (boolean): Whether the damage is from a spell.
- `spellId` (number): The spell ID if `isSpell` is true.

```lua
opengothic.events.register("onNpcTakeDamage", function(victim, attacker, isSpell, spellId)
    if victim:isPlayer() then
        print("Player took damage from " .. attacker:displayName())
    end
end)
```

---

### `onNpcDeath`

Called when an NPC dies or is knocked unconscious. Return `true` to block the default death/KO handling.

- `victim` (Npc): The NPC that died/fell unconscious.
- `killer` (Npc or nil): The NPC responsible (may be nil).
- `isDeath` (boolean): `true` if death, `false` if unconscious.

```lua
opengothic.events.register("onNpcDeath", function(victim, killer, isDeath)
    if isDeath then
        print(victim:displayName() .. " has died")
    else
        print(victim:displayName() .. " was knocked out")
    end
end)
```

---

### `onNpcPerception`

Called when an NPC perceives something. Use `opengothic.CONSTANTS.PercType` to identify perception types. Return `true` to block default handling.

- `npc` (Npc): The NPC doing the perceiving.
- `other` (Npc): The perceived NPC.
- `percType` (number): The type of perception (see PercType constants).

```lua
opengothic.events.register("onNpcPerception", function(npc, other, percType)
    local PercType = opengothic.CONSTANTS.PercType
    if percType == PercType.PERC_ASSESSENEMY then
        print(npc:displayName() .. " spotted an enemy: " .. other:displayName())
    end
end)
```

---

## Item Events

### `onItemPickup`

Called when an NPC picks up an item. Return `true` to block the pickup.

- `npc` (Npc): The NPC picking up the item.
- `item` (Item): The item being picked up.

```lua
opengothic.events.register("onItemPickup", function(npc, item)
    if npc:isPlayer() then
        print("Picked up: " .. item:displayName())
    end
end)
```

---

### `onDropItem`

Called when an NPC drops an item. Return `true` to block the drop.

- `npc` (Npc): The NPC dropping the item.
- `itemId` (number): The instance ID of the item.
- `count` (number): The number of items dropped.

---

### `onUseItem`

Called when an NPC uses an item (potions, food, etc.). Return `true` to block default item use.

- `npc` (Npc): The NPC using the item.
- `item` (Item): The item being used.

```lua
opengothic.events.register("onUseItem", function(npc, item)
    print(npc:displayName() .. " used " .. item:displayName())
end)
```

---

### `onEquip`

Called when an NPC equips an item. Return `true` to block equip.

- `npc` (Npc): The NPC equipping the item.
- `item` (Item): The item being equipped.

---

### `onUnequip`

Called when an NPC unequips an item. Return `true` to block unequip.

- `npc` (Npc): The NPC unequipping the item.
- `item` (Item): The item being unequipped.

---

## Combat Events

### `onDrawWeapon`

Called when an NPC draws a weapon. Return `true` to block draw.

- `npc` (Npc): The NPC drawing the weapon.
- `weaponType` (number): The type of weapon drawn.

---

### `onCloseWeapon`

Called when an NPC sheathes their weapon. Return `true` to block sheath/close.

- `npc` (Npc): The NPC sheathing the weapon.

---

### `onSpellCast`

Called when a spell is cast. Return `true` to block the cast.

- `caster` (Npc): The NPC casting the spell.
- `target` (Npc or nil): The target of the spell (may be nil).
- `spellId` (number): The ID of the spell being cast.

```lua
opengothic.events.register("onSpellCast", function(caster, target, spellId)
    print(caster:displayName() .. " cast spell " .. spellId)
end)
```

---

## Interaction Events

### `onOpen`

Called when an NPC opens an interactive object (chest, door, etc.). Return `true` to block open.

- `npc` (Npc): The NPC opening the object.
- `interactive` (Interactive): The object being opened.

---

### `onRansack`

Called when the player loots an unconscious/dead NPC. Return `true` to block ransacking.

- `player` (Npc): The player doing the looting.
- `target` (Npc): The NPC being looted.

---

### `onDialogStart`

Called when a dialog begins. Return `true` to block dialog start.

- `npc` (Npc): The NPC being talked to.
- `player` (Npc): The player character.

```lua
opengothic.events.register("onDialogStart", function(npc, player)
    print("Starting dialog with " .. npc:displayName())
end)
```

---

### `onDialogOption`

Called for each dialog option before it is shown. Return `true` to hide the option.

- `npc` (Npc): The NPC being talked to.
- `player` (Npc): The player character.
- `infoName` (string): The Daedalus symbol name of the dialog option (e.g., `"INFO_DIEGO_HALLO"`).

```lua
-- Hide a specific quest dialog option outside of night hours
opengothic.events.register("onDialogOption", function(npc, player, infoName)
    if not opengothic.dialog.isOption(infoName) then
        return false
    end

    if infoName == "INFO_NIGHTQUEST_START" then
        local world = opengothic.world()
        if not world:isTime(20, 0, 6, 0) then
            return true  -- Hide this option
        end
    end
    return false  -- Show option
end)
```

**Use cases:**
- Time-gated quest options
- Conditional dialog based on player state
- Dynamic dialog filtering based on game progress

See the [Dialog Time Windows Tutorial](../getting-started/tutorials/dialog-time-windows.md) for complete examples.

---

### `onMobInteract`

Called when an NPC interacts with a mob (usable object). Return `true` to block interaction.

- `npc` (Npc): The NPC interacting.
- `mob` (Interactive): The interactive object.

---

### `onTrade`

Called during a trade transaction. Return `true` to block the trade action.

- `buyer` (Npc): The NPC buying.
- `seller` (Npc): The NPC selling.
- `itemId` (number): The instance ID of the item.
- `count` (number): The number of items.
- `isBuying` (boolean): `true` if player is buying, `false` if selling.

```lua
opengothic.events.register("onTrade", function(buyer, seller, itemId, count, isBuying)
    if isBuying then
        print("Bought " .. count .. " items")
    else
        print("Sold " .. count .. " items")
    end
end)
```

---

## Movement Events

### `onJump`

Called when an NPC jumps. Return `true` to block jump.

- `npc` (Npc): The NPC that jumped.

---

### `onSwimStart`

Called when an NPC starts swimming.

- `npc` (Npc): The NPC that started swimming.

---

### `onSwimEnd`

Called when an NPC stops swimming.

- `npc` (Npc): The NPC that stopped swimming.

---

### `onDiveStart`

Called when an NPC starts diving underwater.

- `npc` (Npc): The NPC that started diving.

---

### `onDiveEnd`

Called when an NPC surfaces from diving.

- `npc` (Npc): The NPC that surfaced.
