# Daedalus Bridge Internals

This page explains how the bridge works internally so experienced Daedalus modders can reason about behavior and failure modes.

## Execution Flow

Bridge calls follow this pattern:

1. Validate Lua-side arguments.
2. Acquire active world and Daedalus VM.
3. Resolve target symbol/function.
4. Marshal Lua arguments onto Daedalus VM stack.
5. Execute VM call.
6. Marshal return value back to Lua.

The two direct call paths are:

- `opengothic.daedalus.call(...)`
- `opengothic.vm.callWithContext(...)`

Some convenience APIs (notably `opengothic.quest.*`) use dedicated engine hooks.

External passthrough status:

- Function symbols marked as external are currently rejected by both call paths.
- This requires an external-call bridge extension before raw external calls can be supported.

## Argument Marshalling Details

### `daedalus.call`

- Supports `number`, `string`, `Npc` userdata, `Item` userdata, and `nil`.
- `nil` is marshalled as integer `0`.
- Unknown userdata is marshalled as `0`.
- `table`, `boolean`, and `function` arguments are rejected with a Lua error.
- Arguments are pushed in reverse order before the VM call.

### `vm.callWithContext`

- Same numeric/string/userdata conversions as above.
- Unsupported argument types are coerced to `0` instead of raising.
- This is intentionally permissive for mixed/legacy call sites.

## Context Pointer Handling

`vm.callWithContext` can set Daedalus global context pointers:

- `SELF`
- `OTHER`
- `VICTIM`
- `ITEM`

via keys in the Lua context table:

```lua
opengothic.vm.callWithContext("ZS_Attack", {
    self = attackerNpc,
    other = targetNpc
})
```

Internal guarantees:

- Previous context pointers are captured before mutation.
- Pointers are restored after successful calls.
- Pointers are also restored when the VM call throws.

This allows reentrant Lua logic without leaking call context into later script execution.

## Symbol Access Model

`daedalus.get` resolves a symbol by name and reads by optional index.

Important instance behavior:

- `C_NPC` instance symbols resolve to live `Npc` userdata only if corresponding world object exists.
- `C_ITEM` instance symbols resolve to live `Item` userdata only if corresponding world object exists.
- Other instance classes are returned as raw symbol index integers.

This is why some instance lookups produce `nil` even for valid symbol names: symbol exists, but no live runtime object is currently mapped.

## Externals: Lua -> Daedalus -> Lua

`vm.registerExternal(name, luaFn)` registers a Lua callback under a Daedalus external name.

Current implementation details:

- Lua callback is invoked with no positional arguments.
- Callback return value is read as integer (non-number becomes `0`).
- Registered externals are retained in script engine state.
- On world/session load, externals are automatically re-registered in the new Daedalus VM.

Practical implication:

- You can register once in script init and rely on session reload re-registration.
- Lua externals remain callable from Daedalus side.
- Raw Lua->Daedalus external passthrough is currently unavailable.

## Enumeration and Introspection

`vm.getSymbol(name)` gives focused symbol metadata.

`vm.enumerate(className, callback)` scans symbol table entries:

- Empty `className` enumerates all symbols.
- Non-empty `className` filters by exact parent class name.
- Callback errors are logged and stop iteration; they are not rethrown as Lua errors.

## Bridge Boundary

The bridge is intentionally conservative:

- No direct memory-address primitives.
- No pointer arithmetic APIs.
- No implicit persistence in Daedalus globals between sessions.

If you need mod state persistence, use `opengothic.storage`.

If you need high-level behaviors, prefer existing convenience wrappers first:

- `opengothic.quest.*`
- `opengothic.dialog.*`
- `opengothic.ai.*`
- `opengothic.timer.*`
- `opengothic.inventory.*`
