# opengothic.dialog

The `opengothic.dialog` table provides helpers for dialog checks and `onDialogOption` workflows.

---

### `opengothic.dialog.isOption(infoName)`

Checks whether `infoName` is a valid dialog option symbol.

- `infoName` (string): Candidate symbol name.
- **Returns**: `boolean`

Rules:
- Must be a non-empty string.
- Must start with `INFO_`.
- Must resolve via `opengothic.resolve(...)`.

```lua
if opengothic.dialog.isOption(infoName) then
    -- safe to apply option-specific logic
end
```

---

### `opengothic.dialog.canTalkTo(npc)`

Quick talkability guard.

- `npc` (Npc): NPC to check.
- **Returns**: `boolean`

Current checks:
- valid NPC
- player exists
- `npc` is not the player
- not dead
- not down
- not currently talking

---

### `opengothic.dialog.onOption(callback)`

Registers a safe wrapper for `onDialogOption`.

- `callback` (function): `callback(npc, player, infoName) -> boolean`
- **Returns**: `handlerId` (number or `nil`), `err` (`nil` or string)

If the callback throws, the wrapper logs the error with `print(...)` and returns `false`.

```lua
local handlerId, err = opengothic.dialog.onOption(function(npc, player, infoName)
    if infoName == "INFO_SECRETQUEST_START" then
        return true
    end
    return false
end)
```

---

### `opengothic.dialog.offOption(handlerId)`

Unregisters a handler previously returned by `dialog.onOption`.

- `handlerId` (number): Registration ID.
- **Returns**: `boolean`

---

### `opengothic.dialog.blockOption(infoName, [blocked])`

Adds or removes a static block rule for one dialog option.

- `infoName` (string): Dialog option symbol (`INFO_*`).
- `blocked` (boolean, optional): Defaults to `true`.
- **Returns**: `ok` (boolean), `err` (`nil` or string)

---

### `opengothic.dialog.setOptionTimeWindow(infoName, startH, startM, endH, endM)`

Adds or updates a time-window rule for one dialog option.

- `infoName` (string): Dialog option symbol (`INFO_*`).
- `startH`, `startM`, `endH`, `endM` (number): Window boundaries.
- **Returns**: `ok` (boolean), `err` (`nil` or string)

Behavior:
- Option is shown inside the window.
- Option is hidden outside the window.

---

### `opengothic.dialog.clearOptionTimeWindow(infoName)`

Removes a time-window rule for one dialog option.

- `infoName` (string): Dialog option symbol (`INFO_*`).
- **Returns**: `ok` (boolean), `err` (`nil` or string)
