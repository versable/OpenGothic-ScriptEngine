# Daedalus Bridge Troubleshooting

This page maps common bridge failures to exact causes and fixes.

## Common Errors

| Error message | Likely cause | Fix |
|---|---|---|
| `daedalus.call: no world loaded` | Called at script load/menu time | Move call into runtime hook (`onWorldLoaded`, gameplay event) |
| `daedalus.set: no world loaded` | Same as above | Same fix as above |
| `vm.callWithContext: no world loaded` | Same as above | Same fix as above |
| `vm.registerExternal: no world loaded` | Registration executed before session world exists | Register at/after `onWorldLoaded` |
| `daedalus.call: function 'X' not found` | Symbol name typo or script pack mismatch | Check symbol name and loaded DAT/scripts |
| `daedalus.call: error calling 'X': external function is not supported by the current bridge ...` | Target symbol is external | Use existing convenience/primitives or wait for external-call bridge extension |
| `daedalus.set: symbol 'X' not found` | Unknown global symbol | Verify symbol name exists in current script set |
| `daedalus.set: cannot modify const symbol 'X'` | Attempt to write const | Remove write or use mutable variable |
| `daedalus.set: index N out of bounds` | Wrong array index | Validate `count`/index assumptions |
| `daedalus.set: expected ...` | Lua value type mismatch | Convert to expected scalar type |
| `daedalus.call: unsupported argument type at position ...` | Passed table/function/boolean to `daedalus.call` | Convert argument to scalar/userdata or wrap differently |
| `vm.callWithContext: error calling 'X': ...` | VM execution failure inside target function | Validate context (`self/other/...`) and argument set |

## Prefer Non-Throwing Wrapper Paths

When runtime errors should not abort script flow, use:

- `opengothic.daedalus.tryCall(...)`
- `opengothic.daedalus.trySet(...)`
- `opengothic.vm.callContextSafe(...)`
- `opengothic.vm.callSelf(...)`
- `opengothic.vm.callSelfOther(...)`

These wrappers convert bridge exceptions into return values (`ok, ..., err`) and are useful for migration-heavy mods.

## Silent/Non-Throwing Pitfalls

### `daedalus.get(...)` returns `nil`

`nil` may mean any of:

1. No world loaded.
2. Symbol not found.
3. Index out of range.
4. Instance symbol exists but no mapped live runtime object.

Add explicit guards:

```lua
local world = opengothic.world()
if not world then
    return
end

local value = opengothic.daedalus.get("MY_SYMBOL")
if value == nil then
    print("[bridge] missing or unavailable symbol")
end
```

### `vm.enumerate(...)` callback error does not throw

Callback errors are logged and enumeration stops. Wrap callback body if you need strict behavior:

```lua
opengothic.vm.enumerate("C_ITEM", function(symbol)
    local ok, err = pcall(function()
        -- your logic
    end)
    if not ok then
        print("[bridge] enumerate callback failed: " .. tostring(err))
    end
    return true
end)
```

## Debug Checklist for Bridge Calls

1. Is a world/session loaded?
2. Does symbol/function name exist in the current script set?
3. Are you passing bridge-compatible argument types?
4. Do you need explicit context (`self`, `other`, `victim`, `item`)?
5. Are you accidentally using bridge calls where an existing convenience wrapper is better?

## Migration Stability Checklist

1. Keep direct bridge calls centralized in one module.
2. Wrap direct calls with `pcall` where failure should not abort script flow.
3. Replace mapped legacy calls with convenience wrappers from [Migration Map](./migration-map.md).
4. Store persistent mod state in `opengothic.storage`, not Daedalus globals.
5. Re-test after save/load transitions because Daedalus VM is session-scoped.
