# opengothic.ui

The `opengothic.ui` table provides wrappers for common message output patterns.

---

### `opengothic.ui.notify(text)`

Shows a simple in-game message.

- `text` (string): Message text.
- **Returns**: `ok` (boolean), `err` (`nil` or string)

```lua
local ok, err = opengothic.ui.notify("Quest updated")
```

---

### `opengothic.ui.toast(text, [seconds], [style])`

Shows a positioned screen message via `opengothic.printScreen(...)`.

- `text` (string): Message text.
- `seconds` (number, optional): Duration in seconds.
- `style` (table, optional):
  - `x` (number): X position (0-100 or `-1` center)
  - `y` (number): Y position (0-100 or `-1` center)
  - `font` (string): Font name
- **Returns**: `ok` (boolean), `err` (`nil` or string)

```lua
opengothic.ui.toast("Level up!", 2.0, {
    x = -1,
    y = 82,
    font = "font_old_10_white.tga"
})
```

---

### `opengothic.ui.debug(text)`

Developer/test output helper. Writes to Lua `print(...)` (stdout/log), not in-world message UI.

- `text` (any): Value to print. `nil` is ignored.
- **Returns**: none

```lua
opengothic.ui.debug("Loot pass started")
opengothic.ui.debug({}) -- prints table tostring()
```
