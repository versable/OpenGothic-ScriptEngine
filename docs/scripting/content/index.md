# OpenGothic Scripting Documentation

This documentation covers the OpenGothic Lua scripting API end to end:

- gameplay scripting
- reusable helper modules
- event-driven systems
- Daedalus interoperability for legacy and hybrid mods

The goal is practical modding without C++ rebuild workflows, while still keeping low-level control available where needed.

## What You Can Build

With the current scripting API, you can implement:

- quest and journal logic
- dialog gating and option rules
- custom combat/damage behaviors
- AI reaction systems
- inventory/world interaction utilities
- mixed Lua + Daedalus workflows for gradual migration

For concrete patterns, use the tutorials under **Getting Started -> Tutorials**.

## API Model

The API is intentionally layered:

1. **Primitives**
   Engine-exposed object methods and global functions for direct control.
2. **Daedalus bridge**
   `opengothic.daedalus.*` and `opengothic.vm.*` for calling existing Daedalus symbols/functions and advanced VM interop.
3. **Convenience wrappers**
   Higher-level APIs in `bootstrap.lua` that reduce repetitive glue code.

See [Core Concept: Architecture](./concepts/architecture.md) for the complete model and tradeoffs.

## How to Navigate This Documentation

### If You Are New to OpenGothic Lua

Use this sequence:

1. [Getting Started](./getting-started/index.md)
2. [Your First Mod](./getting-started/tutorials/your-first-mod.md)
3. Continue with tutorials such as:
   - [Quickloot](./getting-started/tutorials/quickloot.md)
   - [Creating a Simple Quest](./getting-started/tutorials/creating-a-quest.md)
   - [Dialog Time Windows](./getting-started/tutorials/dialog-time-windows.md)

Then move to [Core Concepts](./concepts/architecture.md) before building larger systems.

### If You Are an Experienced Daedalus Modder

Start directly with bridge documentation:

1. [Daedalus Bridge Overview](./daedalus-bridge/index.md)
2. [Daedalus Bridge Contracts](./daedalus-bridge/contracts.md)
3. [Daedalus Bridge Internals](./daedalus-bridge/internals.md)
4. [Daedalus Migration Map](./daedalus-bridge/migration-map.md)
5. [Daedalus Bridge Recipes](./daedalus-bridge/recipes.md)

This path is designed for modders familiar with Daedalus/Ikarus/LeGo style workflows who need precise bridge behavior and safe migration patterns.

## Documentation Sections

- **Getting Started**
  Setup assumptions, script load model, output channels, and hands-on tutorials.
- **Core Concepts**
  Architecture, event semantics, lifecycle behavior, and design constraints.
- **API Reference**
  Detailed method-by-method reference for objects, constants, and helper namespaces.
- **Daedalus Bridge**
  Strict contracts, internals, migration guidance, and troubleshooting.

## Practical Rules of Thumb

1. Prefer convenience APIs when they already match your target behavior.
2. Use direct bridge calls when no wrapper exists or exact legacy semantics are required.
3. Call bridge functions only when a world/session is active (except `daedalus.get`, which returns `nil` safely when no world is loaded).
4. Use `opengothic.storage` for save-bound persistent state; do not rely on Daedalus globals for persistence across sessions.
5. Use `print(...)` (or `opengothic.ui.debug(...)`) for developer/test output and UI notify methods for player-facing messages.

## Start Here

- New project: [Getting Started](./getting-started/index.md)
- Existing Daedalus-heavy mod: [Daedalus Bridge Overview](./daedalus-bridge/index.md)
