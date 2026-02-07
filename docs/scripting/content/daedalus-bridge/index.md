# Daedalus Bridge Overview

This section is targeted at modders already fluent in Daedalus/Ikarus/LeGo workflows.

The goal is not to hide Daedalus internals. The goal is to document exactly how OpenGothic bridges Lua <-> Daedalus, where behavior differs from legacy tools, and which high-level APIs you should prefer today.

Use this together with:

- [Core Concept: API Architecture](../concepts/architecture.md)
- [Core Concept: VM Lifecycles](../concepts/lifecycles.md)

## What the Bridge Is

OpenGothic exposes two bridge layers:

- `opengothic.daedalus.*`:
  - Direct symbol/function access (`get`, `set`, `call`).
- `opengothic.vm.*`:
  - Advanced VM interop (`callWithContext`, `registerExternal`, `getSymbol`, `enumerate`).

Additionally, non-throwing bridge ergonomics wrappers are available for migration-safe usage:

- `opengothic.daedalus.tryCall`, `trySet`, `exists`
- `opengothic.vm.callContextSafe`, `callSelf`, `callSelfOther`

Both require an active game world for most operations (exception: `daedalus.get` returns `nil` when no world is loaded).

## What to Prefer First

Experienced Daedalus users should still prefer existing convenience APIs when they already model the use case:

- Quests: `opengothic.quest.*`
- Dialog rules: `opengothic.dialog.*`
- AI actions: `opengothic.ai.*`
- Timers: `opengothic.timer.*`
- Inventory/world helpers: `opengothic.inventory.*`, `opengothic.worldutil.*`
- World/NPC wrappers: `Npc:giveItem`, `Npc:hasItem`, `World:insertNpc`, `World:insertItem`

Quest note: `opengothic.quest.*` uses dedicated engine quest hooks (`_quest*`).

Direct bridge calls are best when:

1. You need an unwrapped Daedalus function now.
2. You are porting existing Daedalus logic incrementally.
3. You need symbol-level introspection/debugging.

Current limitation:

- External-function passthrough is unavailable in the bridge (`daedalus.call` / `vm.callWithContext`).
- Use convenience wrappers/primitives for affected functionality until external-call bridge extension is available.

## Documentation Map

1. [Contracts](./contracts.md): exact argument, return, and error behavior.
2. [Internals](./internals.md): marshalling rules, context handling, lifecycle details.
3. [Migration Map](./migration-map.md): Daedalus calls mapped to existing OpenGothic APIs.
4. [Recipes](./recipes.md): practical patterns for mixed Lua/Daedalus mods.
5. [Troubleshooting](./troubleshooting.md): common runtime failures and fixes.

## Community Context

The bridge documentation assumes common Gothic modding patterns seen across DE/PL/RU communities:

- Base Daedalus is often extended with Ikarus/LeGo/Union or zParserExtender-style tooling.
- Advanced modders value low-level control, but stability and compatibility are recurring pain points.
- The practical migration path is usually:
  - keep critical Daedalus behavior,
  - bridge safely from Lua,
  - replace with higher-level OpenGothic helpers where behavior matches.

Reference reading:

- Gothic Modding Community Extenders: <https://gothic-modding-community.github.io/gmc/extenders/>
- GMC Ikarus page: <https://gothic-modding-community.github.io/gmc/extenders/ikarus/>
- GMC LeGo page: <https://gothic-modding-community.github.io/gmc/extenders/lego/>
- Polish scripts docs (GMC): <https://gothic-modding-community.github.io/gmc/pl/zengin/scripts/>
- World of Gothic wiki (DE): <https://wiki.worldofgothic.de/doku.php?id=editing:skripte>
- zParserExtender background (RU, World of Players): <https://worldofplayers.ru/threads/41999/>
