# Core Concept: Events and Hooks

The event system is the heart of the scripting engine. It is the primary mechanism through which your mods can react to and modify game behavior.

For a deeper understanding of how events work internally (registration order, blocking mechanics), see the [Architecture](./architecture.md) documentation.

## What are Events?

As the game runs, the engine "fires" events at key moments. These are also often called "hooks". For example:

- An NPC is about to take damage (`onNpcTakeDamage`)
- The player opens a container (`onOpen`)
- A spell is cast (`onSpellCast`)
- A new game session begins (`onLoadGame`)

Your scripts can "listen" for these events and execute code when they happen.

## Registering an Event Handler

To listen for an event, you need to register a *handler* function. A handler is simply a Lua function that the engine will call when the event occurs. You do this using `opengothic.events.register`.

`opengothic.events.register(eventName, handlerFunction) -> handlerId|nil`

- `eventName` (string): The name of the event you want to listen for. A full list is available in the [Events Reference](../api-reference/events.md).
- `handlerFunction` (function): The function to be called when the event fires.
- **Return value:** A numeric `handlerId` on success, or `nil` on invalid arguments.

```lua
-- Define a function
local function myHandler(some, arguments)
    -- Your code here
end

-- Register it to an event
local handlerId = opengothic.events.register("onSomeEvent", myHandler)
```

## Unregistering a Handler

Use `opengothic.events.unregister(eventName, handlerId)` when you no longer need a handler.

```lua
local handlerId = opengothic.events.register("onUpdate", function(dt)
    print("Tick: " .. tostring(dt))
    return false
end)

-- Later:
opengothic.events.unregister("onUpdate", handlerId)
```

### Handler Arguments

Each event provides a specific set of arguments to its handler function. For example, `onNpcTakeDamage` provides the `victim`, `attacker`, `isSpell`, and `spellId`.

```lua
local function onDamage(victim, attacker, isSpell, spellId)
    local message = victim:displayName() .. " was hit by " .. attacker:displayName()
    opengothic.printMessage(message)
    return false
end

opengothic.events.register("onNpcTakeDamage", onDamage)
```
You must check the [API Reference](../api-reference/index.md) to know the exact arguments for each event.

## Controlling Game Logic with Return Values

Many event hooks allow you to change the game's behavior based on what your handler function returns.

There are two main types of hooks:

### 1. Interception Hooks

These hooks fire **before** the default game logic runs. They allow you to completely override or prevent that logic.

- **Return `false` or `nil`**: The default game logic will proceed as normal. This is the most common return value.
- **Return `true`**: The default game logic is **stopped**. The game assumes your script has handled the event completely.

**Example: Creating an invincible NPC**

This script listens for `onNpcTakeDamage`. If the victim is the NPC named "BOB", the handler returns `true`. This tells the game engine "stop processing this damage event," effectively making Bob immune to all damage. For any other NPC, it returns `false`, and they take damage as usual.

```lua
-- Resolve the Daedalus symbol for PC_BOB once at script load time
-- opengothic.resolve() returns a numeric symbol index, or nil if not found
local PC_BOB_ID = opengothic.resolve("PC_BOB")

local function makeBobInvincible(victim, attacker, isSpell, spellId)
    -- Compare the victim's instance ID to the resolved symbol index
    -- (Note: This assumes you created an NPC with instance name PC_BOB)
    if PC_BOB_ID and victim:instanceId() == PC_BOB_ID then
        opengothic.printMessage("Bob is invincible!")
        return true -- Block the damage
    end

    -- For everyone else, let the damage happen
    return false
end

opengothic.events.register("onNpcTakeDamage", makeBobInvincible)
```

### 2. Notification Hooks

These hooks are for observation only. They fire **after** something has already happened.

The engine ignores the final "handled" state for these events, but Lua callback chaining still stops when a handler returns `true`. In practice, observation handlers should return `false`/`nil` unless they intentionally want to stop later Lua handlers.

Examples include:
- `onNpcSpawn`: Fires after an NPC has been spawned into the world.
- `onSessionExit`: Fires when the player is returning to the main menu.

Recommendation: return `false` from notification handlers to keep them composable with other mods.
