# Getting Started with Lua Modding

This section is the shortest path from zero setup to a working OpenGothic Lua script.

## What You Need

- OpenGothic running locally.
- Write access to your game directory.
- Basic Lua syntax knowledge (`local`, functions, tables).

## Script Location and Load Model

OpenGothic loads all `.lua` files from:

`Data/opengothic/scripts/`

The directory is scanned recursively.

```text
Gothic2/
└── Data/
    └── opengothic/
        └── scripts/
            ├── health_monitor.lua
            └── my_mod/
                ├── init.lua
                └── helpers.lua
```

All files share one Lua VM/global environment. Use `local` by default and prefix persistent storage keys to avoid collisions with other mods.

## API Surface at a Glance

The scripting API is exposed through the global `opengothic` table.

- Event hooks: `opengothic.events.register(...)`
- World and player access: `opengothic.world()`, `opengothic.player()`
- Constants and enums: `opengothic.CONSTANTS`
- Save-safe mod state: `opengothic.storage`
- Convenience namespaces: `opengothic.quest`, `opengothic.dialog`, `opengothic.ai`, `opengothic.timer`, `opengothic.ui`

See the [API Reference](../api-reference/index.md) for full details.

## Output Channels

Use output channels intentionally:

- `print(...)` or `opengothic.ui.debug(...)` for developer/test logs (stdout).
- `opengothic.ui.notify(...)` or `opengothic.printMessage(...)` for player-facing in-game messages.

```lua
print("[my_mod] loaded")
opengothic.ui.notify("Quest updated")
```

## Learning Path

1. Start with [Your First Mod](./tutorials/your-first-mod.md).
2. Continue with [Quickloot](./tutorials/quickloot.md), [Auto Pickup](./tutorials/auto-pickup.md), or [Creating a Simple Quest](./tutorials/creating-a-quest.md).
3. Then move to [Core Concepts](../concepts/architecture.md) to choose between primitives and convenience APIs.
