# Daedalus Bridge Contracts

This page documents current bridge behavior exactly as implemented.

If you are porting old Daedalus/Ikarus/LeGo code, treat this as the source of truth for return values, error behavior, and edge cases.

## Error Model

- Bridge functions that call `luaL_error(...)` raise a Lua error.
- Use `pcall(...)` if you want non-fatal behavior in scripts.
- Bridge functions that return `nil`/`false` do so intentionally and do not raise.
- Calls to external function symbols currently raise an explicit unsupported-bridge error.

## `opengothic.daedalus.*`

| Function | World required | Return | Failure behavior | Notes |
|---|---|---|---|---|
| `daedalus.get(name, index?)` | No | symbol value or `nil` | returns `nil` | `nil` for no world, unknown symbol, or out-of-bounds index |
| `daedalus.set(name, value, index?)` | Yes | no return | raises Lua error | only `int`, `float`, `string` symbols are writable |
| `daedalus.call(funcName, ...)` | Yes | function return (`int`/`float`/`string`) or no return | raises Lua error | external function symbols are currently unsupported |
| `daedalus.tryCall(funcName, ...)` | Yes | `ok, result, err` | returns `ok=false` + `err` string | non-throwing wrapper around `daedalus.call` |
| `daedalus.trySet(name, value, index?)` | Yes | `ok, err` | returns `ok=false` + `err` string | non-throwing wrapper around `daedalus.set` |
| `daedalus.exists(name)` | No | `boolean` | returns `false` | checks symbol existence through `vm.getSymbol` |

### `daedalus.get` value mapping

`daedalus.get` maps Daedalus symbols to Lua values as follows:

| Daedalus symbol type | Lua value |
|---|---|
| `INT` | `number` (integer) |
| `FLOAT` | `number` |
| `STRING` | `string` |
| `INSTANCE` (`C_NPC`) | `Npc` userdata (if resolvable in world), else `nil` |
| `INSTANCE` (`C_ITEM`) | `Item` userdata (if resolvable in world), else `nil` |
| `INSTANCE` (other class) | symbol index as `number` |
| `FUNCTION` | `number` (symbol int value) |
| unsupported/unknown | `nil` |

### `daedalus.call` argument mapping

| Lua argument | Daedalus stack push |
|---|---|
| integer-like `number` | `int` |
| non-integer `number` | `float` |
| `string` | `string` |
| `Npc` userdata | instance pointer |
| `Item` userdata | instance pointer |
| `nil` | `0` |
| other userdata | `0` |
| `table`/`boolean`/`function`/thread | raises `unsupported argument type` |

Arguments are pushed in reverse Lua order before the VM call.

### `daedalus.tryCall` / `daedalus.trySet` contracts

- These wrappers convert thrown bridge errors into return values.
- They do not raise on normal invalid usage.
- `tryCall` return shape:
  - success: `true, result, nil`
  - failure: `false, nil, "error_text"`
- `trySet` return shape:
  - success: `true, nil`
  - failure: `false, "error_text"`
- `trySet` validates:
  - non-empty symbol name
  - numeric `index` when provided

## `opengothic.vm.*`

| Function | World required | Return | Failure behavior | Notes |
|---|---|---|---|---|
| `vm.callWithContext(funcName, context, ...)` | Yes | function return (`int`/`float`/`string`) or no return | raises Lua error | context table required at arg 2; external symbols are currently unsupported |
| `vm.callContextSafe(funcName, context, ...)` | Yes | `ok, result, err` | returns `ok=false` + `err` string | non-throwing wrapper around `vm.callWithContext` |
| `vm.callSelf(funcName, selfNpc, ...)` | Yes | `ok, result, err` | returns `ok=false` + `err` string | shorthand for `{ self = npc }` context |
| `vm.callSelfOther(funcName, selfNpc, otherNpc, ...)` | Yes | `ok, result, err` | returns `ok=false` + `err` string | shorthand for `{ self = npc, other = npc }` context |
| `vm.registerExternal(name, luaFn)` | Yes | no return | raises Lua error | callback currently called with no args, integer return consumed |
| `vm.getSymbol(name)` | No | symbol table or `nil` | returns `nil` | `nil` for no world or unknown symbol |
| `vm.enumerate(className, callback)` | No | no return | does not throw on callback errors | callback errors are logged and iteration stops |

### `vm.callWithContext` context handling

- Recognized keys: `self`, `other`, `victim`, `item`.
- `self`/`other`/`victim` expect `Npc` userdata.
- `item` expects `Item` userdata.
- Invalid or missing keys are ignored.
- Previous VM context is restored after the call and also restored on exceptions.

### `vm.callWithContext` argument mapping

| Lua argument | Daedalus stack push |
|---|---|
| integer-like `number` | `int` |
| non-integer `number` | `float` |
| `string` | `string` |
| `Npc` userdata | instance pointer |
| `Item` userdata | instance pointer |
| any unsupported type (including `nil`) | `0` |

Unlike `daedalus.call`, unsupported non-userdata types are coerced to `0` instead of raising.

### `vm.callContextSafe` / `vm.callSelf*` contracts

- `vm.callContextSafe` validates:
  - non-empty function name
  - context must be a table
- `vm.callSelf` and `vm.callSelfOther` validate NPC userdata inputs before call.
- Return shape for all three wrappers:
  - success: `true, result, nil`
  - failure: `false, nil, "error_text"`

### `vm.getSymbol` return schema

On success:

```lua
{
  name = "SYMBOL_NAME",
  index = 123,
  count = 1,
  isConst = true,
  type = "int" | "float" | "string" | "class" | "function" | "prototype" | "instance" | "void",
  value = ... -- only for scalar int/float/string symbols
}
```

### `vm.enumerate` callback schema

For each visited symbol, callback receives:

```lua
{
  name = "SYMBOL_NAME",
  index = 123
}
```

- Return `false` from callback to stop enumeration.
- Any other return value continues enumeration.

## Lifecycle Contracts

- Daedalus VM is session-scoped. It is recreated on new game/load.
- Lua externals registered via `vm.registerExternal(...)` are automatically re-registered when the world is loaded again.
- Do not use Daedalus globals for persistence across game restarts. Use `opengothic.storage` for save-bound state.

See [VM Lifecycles](../concepts/lifecycles.md) for lifecycle background.
