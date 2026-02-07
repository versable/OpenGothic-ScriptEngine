# Core Concept: VM Lifecycles

A "Virtual Machine" (VM) is the environment where scripts are executed. OpenGothic uses two different VMs that run in parallel: the Lua VM for mods, and the Daedalus VM for the original game's scripts. Understanding their lifecycles is critical for writing stable, bug-free mods.

## The Lua VM (`ScriptEngine`)

The Lua VM is where all your `.lua` mod scripts live and run.

- **Created:** The Lua VM is created **once** when the OpenGothic application first launches, even before you see the main menu.
- **Persists:** It stays alive for the entire duration of the application. It persists across new games, loaded games, and trips back to the main menu.
- **Destroyed:** The Lua VM is only destroyed when you quit the OpenGothic application entirely.

### Implications for Modders

- **Global state is persistent.** If you define a global variable in your Lua script, its value will persist until the game is closed. This can be powerful, but also dangerous if you're not careful.

- **Lifecycle hooks are reliable.** Because the VM is always running, you can reliably use hooks like `onStartGame` or `onLoadGame` to initialize or reset your mod's state for a new session.

- **Event registrations are persistent too.** A handler registered at script load stays registered across sessions until you explicitly unregister it.

```lua
local sessionCounter = 0

-- This function will be called every time a save is loaded
local function onGameLoaded(savegameName)
    sessionCounter = sessionCounter + 1
    opengothic.printMessage("This is session number " .. sessionCounter)
end

-- This registration only happens once when the script is first loaded
opengothic.events.register("onLoadGame", onGameLoaded)
```

In the example above, the `sessionCounter` will correctly increment every time you load a game, because it's a global variable in the persistent Lua VM.

`onUpdate` and `onGameMinuteChanged` are only fired while a game session is active. They do not run on the main menu.

## The Daedalus VM (`GameScript`)

The Daedalus VM runs the original game's compiled `.dat` scripts. The Lua scripting engine provides a [bridge](../daedalus-bridge/index.md) to interact with it.

- **Created:** A new Daedalus VM is created **every time a game session begins**. This happens when you start a new game or load a save file.
- **Persists:** It only stays alive for the current game session.
- **Destroyed:** The Daedalus VM is destroyed whenever the session ends (e.g., loading another save, or returning to the main menu).

### Implications for Modders

- **Daedalus state is NOT persistent.** Any changes you make to Daedalus global variables will be lost when the session ends. You cannot use Daedalus globals to store information across saves (that's what save files are for).

- **`opengothic.daedalus.*` only works in-game.** You can only use the Daedalus bridge functions when a world is loaded. When called from the main menu (before any world is loaded):
    - `daedalus.get()` returns `nil` gracefully (safe to call)
    - `daedalus.set()`, `daedalus.call()`, and `vm.registerExternal()` will throw a Lua error

- **Lua externals must be re-registered.** If you use `opengothic.vm.registerExternal` to create a Lua callback for Daedalus, the engine automatically handles re-registering it every time a new Daedalus VM is created. You do not need to worry about this, but it's good to know why it's necessary.

## Persisting Data Across Sessions

While Lua globals persist during the application lifetime, they are lost when the game closes. For data that must survive across game restarts, use `opengothic.storage`:

| Storage Type | Lifetime | Survives Restart? | Use Case |
|-------------|----------|-------------------|----------|
| Lua globals | Application | No | Session counters, caches |
| `opengothic.storage` | Save file | Yes | Quest progress, mod state |
| Daedalus globals | Session | No | Temporary game flags |

See the [Storage API](../api-reference/storage.md) for usage details and best practices.

At the start of each new game/load session, the engine clears `opengothic.storage` before optionally applying data from `game/lua`. This prevents stale values from leaking between sessions.

## Summary

| Feature | Lua VM | Daedalus VM |
| :--- | :--- | :--- |
| **Creation** | On application start | On new game / load game |
| **Persistence**| Entire application lifetime | Single game session |
| **Destruction** | On application exit | On session end (exit to menu) |
| **State** | Globals persist across saves | Globals are reset each session |
| **Modding Implication** | Good for persistent mod logic | Good for temporary game state |
