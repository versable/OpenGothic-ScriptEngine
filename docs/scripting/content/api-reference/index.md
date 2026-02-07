# API Reference

Welcome to the OpenGothic Lua API Reference. This section provides a detailed breakdown of all the objects, functions, and constants available to you as a modder.

## The `opengothic` Table

All API functionality is exposed through the single global table `opengothic`. This includes sub-tables for events, constants, and the Daedalus bridge, as well as functions for accessing global game objects like the `World` and the `Player`.

## API Layers

The API is organized into three layers:

1. **Primitives** (direct C++ methods).
2. **Daedalus bridge** (`opengothic.daedalus.*`, `opengothic.vm.*`).
3. **Convenience methods** (Lua wrappers in `bootstrap.lua`).

Most modders use convenience methods first, then bridge/primitives for specialized behavior. See [Architecture](../concepts/architecture.md) for details.

## Game Objects

The most common way you will interact with the game is through "game objects." When you get a handle to an NPC, an item, or the world itself, you are given a Lua `userdata` object with a set of methods you can call.

For example, when you get the player object, you can call methods on it like `player:displayName()` or `player:inventory()`.

```lua
local player = opengothic.player()

-- Call the displayName() method on the Npc object
local name = player:displayName()

-- Call the inventory() method to get another object
local inv = player:inventory()
```

These objects are the building blocks of your mods. The following pages document the methods available for each object type.

## API Ownership Map

Each API symbol has exactly one canonical documentation page.

- `world:*`, `npc:*`, `item:*`, `inventory:*`, `interactive:*` live only on object pages.
- `opengothic.<namespace>.*` tables live only on namespace pages under `api-reference/helpers/`.
- Object pages may include a short "See also" link to a namespace page, but must not duplicate namespace method docs.

## Structure

### Core

- **[Globals](./globals.md):** `opengothic.*` global entry points.
- **[Events](./events.md):** Hook names and callback signatures.
- **[Storage](./storage.md):** Persistent save-backed key/value data.
- **[Constants](./constants.md):** Built-in enums and constants.

### Objects

- **[World](./world.md):** `world:*`.
- **[Npc](./npc.md):** `npc:*`.
- **[Item](./item.md):** `item:*`.
- **[Inventory](./inventory.md):** `inventory:*`.
- **[Interactive](./interactive.md):** `interactive:*`.

### Namespaces

- **[Quest](./helpers/quest.md):** `opengothic.quest.*`.
- **[Dialog](./helpers/dialog.md):** `opengothic.dialog.*`.
- **[AI](./helpers/ai.md):** `opengothic.ai.*`.
- **[Timer](./helpers/timer.md):** `opengothic.timer.*`.
- **[UI](./helpers/ui.md):** `opengothic.ui.*`.
- **[Inventory](./helpers/inventory.md):** `opengothic.inventory.*`.
- **[World](./helpers/world.md):** `opengothic.worldutil.*`.
- **[Damage Calculator](./helpers/damage-calculator.md):** `opengothic.DamageCalculator.*`.
- **[Sound](./helpers/sound.md):** `opengothic.sound.*`.
- **[Effect](./helpers/effect.md):** `opengothic.effect.*`.
