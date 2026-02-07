# Daedalus Migration Map

This map is for incremental migration: keep behavior, reduce boilerplate, and prefer maintained wrappers where available.

## Daedalus Call -> Existing OpenGothic API

| Legacy Daedalus call | Preferred OpenGothic API | Notes |
|---|---|---|
| `Log_CreateTopic(topic, section)` | `opengothic.quest.create(topic, section)` | Uses dedicated `_questCreateTopic` hook; section defaults to `MISSIONS` |
| `Log_SetTopicStatus(topic, status)` | `opengothic.quest.setState(topic, status)` | Uses dedicated `_questSetTopicStatus` hook; accepts numeric or string status |
| `Log_AddEntry(topic, text)` | `opengothic.quest.addEntry(topic, text)` | Uses dedicated `_questAddEntry` hook |
| `Npc_KnowsInfo(player, infoId)` | No direct helper | Use explicit quest/dialog state in Lua for availability checks |
| `CreateInvItems(npc, itemId, count)` | `npc:giveItem(itemSymbol, count)` | Uses item symbol names |
| `Npc_RemoveInvItems(npc, itemId, count)` | No direct helper | Use inventory primitives (`transfer`, `transferAll`, `itemCount`) |
| `Npc_HasItems(npc, itemId)` | `npc:hasItem(itemSymbol, minCount)` | Boolean helper |
| `EquipItem(npc, itemId)` | No direct helper | Use exposed equipment-related primitives where available |
| `Wld_InsertNpc(npcId, wp)` | `world:insertNpc(npcSymbol, waypoint)` | Returns inserted NPC if found |
| `Wld_InsertItem(itemId, wp)` | `world:insertItem(itemSymbol, waypoint)` | Returns inserted Item if found |
| `Snd_Play(name)` | `opengothic.sound.play(name)` | World-level sound helper |
| `Snd_Play3D(npc, name)` | No direct helper | Pending external-call bridge extension |
| `Wld_PlayEffect(name, src, dst, ...)` | `opengothic.effect.play(name, src, dst)` | Plays at source position |
| `Wld_StopEffect(name)` | No direct helper | Pending effect-stop primitive/helper |

## Legacy Utility Pattern -> Current Convenience API

| Common legacy pattern | Current OpenGothic helper | Why prefer it |
|---|---|---|
| Dialog option filtering chains | `opengothic.dialog.onOption`, `blockOption`, `setOptionTimeWindow` | Built-in validation and hook lifecycle handling |
| Manual attack/flee command chains | `opengothic.ai.attackTarget`, `flee`, `reset`, `isCombatReady` | Safer error handling and clearer intent |
| Hand-rolled update-loop timers | `opengothic.timer.after`, `every`, `everyGameMinute`, `cancel` | Deterministic scheduler wrapper with isolated callback errors |
| Manual inventory transfer loops | `opengothic.inventory.transferAll`, `transferByPredicate` | Standardized filtering and result shape |
| Repeated nearest-NPC scan code | `opengothic.worldutil.findNearestNpc` | Reusable and predicate-aware |

## When Direct Bridge Calls Are Still Correct

Use `opengothic.daedalus.call(...)` or `opengothic.vm.callWithContext(...)` when:

1. No convenience wrapper exists yet.
2. You must preserve exact non-external Daedalus function behavior.
3. You are doing staged migration and need temporary interop.

Do not plan on raw bridge calls for external symbols yet; those paths are currently unsupported.

## Prefer Safe Bridge Wrappers for Transitional Code

For migration phases where direct bridge calls are still required, prefer:

- `opengothic.daedalus.tryCall(...)` instead of raw `daedalus.call(...)` in non-critical paths.
- `opengothic.daedalus.trySet(...)` instead of raw `daedalus.set(...)` when failure should not abort script execution.
- `opengothic.vm.callContextSafe(...)` / `callSelf(...)` / `callSelfOther(...)` for context-driven calls.

This keeps legacy interop robust while you gradually move to high-level wrappers.

## Porting Strategy

1. Keep direct bridge calls in one module (`bridge_compat.lua` style).
2. Replace mapped calls with wrappers from this page.
3. Add `pcall` around remaining direct bridge sites.
4. Move persistent state to `opengothic.storage`.
5. Keep only unavoidable `daedalus.call` uses.

## Related Pages

- [Contracts](./contracts.md)
- [Internals](./internals.md)
- [Recipes](./recipes.md)
- [API Reference: Globals](../api-reference/globals.md)
