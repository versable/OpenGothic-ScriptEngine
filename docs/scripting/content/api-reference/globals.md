# Global Functions

These functions are available directly under the `opengothic` global table.

---

### `opengothic.world()`

Returns the current `World` object. The World object is your primary entry point for finding objects and changing global world state.

- **Returns**: A `World` object, or `nil` if no game session is active.

```lua
local world = opengothic.world()
if world then
    -- Do something with the world
    local time_h, time_m = world:time()
    print("The time is " .. time_h .. ":" .. time_m)
end
```

---

### `opengothic.player()`

A convenience function to get the player's `Npc` object.

- **Returns**: The player's `Npc` object, or `nil` if no game session is active.

```lua
local player = opengothic.player()
if player then
    print("Player's name is " .. player:displayName())
end
```

---

### `opengothic.printMessage(message)`

Shows a message through the game's in-world/message UI channel.

- `message` (string): The message to print.

```lua
local level = opengothic.player():level()
opengothic.printMessage("Player level is: " .. level)
```

For developer/test logs, prefer Lua's global `print(...)`, which writes to script log/stdout.

---

### `opengothic.printScreen(message, x, y, [time], [font])`

Displays a message at a specific screen position. Useful for custom HUD elements, damage numbers, or notifications.

- `message` (string): The text to display.
- `x` (number): Horizontal position (0-100 percent, or -1 to center).
- `y` (number): Vertical position (0-100 percent, or -1 to center).
- `time` (number, optional): Display duration in seconds. Default: 5.
- `font` (string, optional): Font name. Default: `"font_old_10_white.tga"`.

```lua
-- Display centered message for 3 seconds
opengothic.printScreen("Level Up!", -1, -1, 3)

-- Display at top-left corner
opengothic.printScreen("Score: 100", 5, 5, 2)

-- Display at bottom-center
opengothic.printScreen("Press E to interact", -1, 90, 4)
```

---

### `opengothic.resolve(symbolName)`

Finds the integer ID of a Daedalus symbol by its name. This is useful when you need to pass an instance ID to a function, such as when spawning an NPC or item.

This lookup requires an active game world/session (the symbol table comes from the loaded `GameScript`).

- `symbolName` (string): The name of the Daedalus instance (e.g., `"PC_HERO"`, `"ItMw_1H_Sword"`).
- **Returns**: The integer ID of the symbol, or `nil` if not found (or if no world is loaded).

```lua
local world = opengothic.world()
if world then
    local swordId = opengothic.resolve("ItMw_1H_Sword")
    if swordId then
        -- Now you can use this ID to add the item
        opengothic.player():inventory():addItem(swordId, 1)
    end
end
```

---

## Bridge Tables

The following global tables expose Daedalus interop and VM bridge APIs:

- `opengothic.daedalus`
- `opengothic.vm`

For full behavioral contracts, see [Daedalus Bridge Contracts](../daedalus-bridge/contracts.md).

---

### Raw Bridge APIs

The raw bridge APIs are:

- `opengothic.daedalus.get(...)`
- `opengothic.daedalus.set(...)`
- `opengothic.daedalus.call(...)`
- `opengothic.vm.callWithContext(...)`
- `opengothic.vm.registerExternal(...)`
- `opengothic.vm.getSymbol(...)`
- `opengothic.vm.enumerate(...)`

Use these for low-level interop and symbol access. They can raise Lua errors on invalid state/arguments. External-function passthrough is currently unsupported. For non-throwing behavior, prefer the safe wrappers below.

---

### `opengothic.daedalus.get(symbolName, [index])`

Reads a Daedalus symbol value from the active session.

- **Returns**: symbol value or `nil`.
- Safe to call without world; returns `nil` when unavailable.

---

### `opengothic.daedalus.set(symbolName, value, [index])`

Writes a Daedalus symbol value.

- Raises on invalid symbol/type/no-world cases.

---

### `opengothic.daedalus.call(functionName, ...)`

Calls a Daedalus function symbol.

- Returns function result for scalar return types.
- Raises on invalid symbol/no-world/unsupported argument type.
- Raises for external function symbols (current bridge limitation).

---

### `opengothic.vm.callWithContext(functionName, context, ...)`

Calls a Daedalus function with explicit context keys (`self`, `other`, `victim`, `item`).

- Returns function result for scalar return types.
- Raises on invalid symbol/no-world/call failure.
- Raises for external function symbols (current bridge limitation).

---

### `opengothic.vm.registerExternal(name, fn)`

Registers a Lua callback as a Daedalus external.

- Callback is currently invoked with no positional arguments.
- Numeric return value is passed back to Daedalus.

---

### `opengothic.vm.getSymbol(name)`

Returns metadata table for a Daedalus symbol, or `nil`.

---

### `opengothic.vm.enumerate(className, callback)`

Enumerates symbols, optionally filtered by parent class name.

- Callback gets `{ name, index }`.
- Return `false` from callback to stop.

---

### `opengothic.daedalus.tryCall(functionName, ...)`

Non-throwing wrapper around `opengothic.daedalus.call(...)`.

- **Returns**:
  - success: `true, result, nil`
  - failure: `false, nil, err`

```lua
local ok, result, err = opengothic.daedalus.tryCall("B_GivePlayerXP", 100)
if not ok then
    print("tryCall failed: " .. tostring(err))
end
```

---

### `opengothic.daedalus.trySet(symbolName, value, [index])`

Non-throwing wrapper around `opengothic.daedalus.set(...)`.

- **Returns**:
  - success: `true, nil`
  - failure: `false, err`

```lua
local ok, err = opengothic.daedalus.trySet("MY_QUEST_STATE", 2)
if not ok then
    print("trySet failed: " .. tostring(err))
end
```

---

### `opengothic.daedalus.exists(symbolName)`

Checks whether a Daedalus symbol exists in the current session symbol table.

- **Returns**: `boolean`

```lua
if opengothic.daedalus.exists("Npc_IsInState") then
    print("Symbol exists")
end
```

---

### `opengothic.vm.callContextSafe(functionName, context, ...)`

Non-throwing wrapper around `opengothic.vm.callWithContext(...)`.

- `context` supports keys: `self`, `other`, `victim`, `item`.
- **Returns**:
  - success: `true, result, nil`
  - failure: `false, nil, err`

```lua
local ok, result, err = opengothic.vm.callContextSafe("ZS_Attack", {
    self = attacker,
    other = target
})
```

---

### `opengothic.vm.callSelf(functionName, selfNpc, ...)`

Convenience wrapper for `callContextSafe` with `{ self = selfNpc }`.

- **Returns**:
  - success: `true, result, nil`
  - failure: `false, nil, err`

---

### `opengothic.vm.callSelfOther(functionName, selfNpc, otherNpc, ...)`

Convenience wrapper for `callContextSafe` with `{ self = selfNpc, other = otherNpc }`.

- **Returns**:
  - success: `true, result, nil`
  - failure: `false, nil, err`

---

For full contracts and edge-case behavior of raw/safe bridge APIs, see [Daedalus Bridge Contracts](../daedalus-bridge/contracts.md).
