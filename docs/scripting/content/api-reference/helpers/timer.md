# opengothic.timer

The `opengothic.timer` table provides script-side scheduling helpers.

Callbacks run under `pcall`, so one failing timer does not break other timers.

---

### `opengothic.timer.after(seconds, fn)`

Schedules a one-shot callback after `seconds`.

- `seconds` (number): Delay in real-time seconds (`>= 0`).
- `fn` (function): Callback `fn(timerId)`.
- **Returns**: `timerId` (number or `nil`), `err` (`nil` or string)

```lua
local id, err = opengothic.timer.after(2.0, function(timerId)
    print("One-shot timer fired: " .. tostring(timerId))
end)
```

---

### `opengothic.timer.every(seconds, fn)`

Schedules a repeating real-time callback.

- `seconds` (number): Interval in seconds (`> 0`).
- `fn` (function): Callback `fn(timerId)`.
- **Returns**: `timerId` (number or `nil`), `err` (`nil` or string)

```lua
local heartbeatId = opengothic.timer.every(1.0, function(timerId)
    print("Tick " .. tostring(timerId))
end)
```

---

### `opengothic.timer.everyGameMinute(fn)`

Schedules a callback for each in-game minute change.

- `fn` (function): Callback `fn(timerId)`.
- **Returns**: `timerId` (number or `nil`), `err` (`nil` or string)

```lua
local id = opengothic.timer.everyGameMinute(function()
    local h, m = opengothic.world():time()
    print("Game minute changed: " .. h .. ":" .. m)
end)
```

---

### `opengothic.timer.cancel(timerId)`

Cancels a timer.

- `timerId` (number): ID returned by a timer creation call.
- **Returns**: `boolean` (`true` if timer existed and was removed)

```lua
opengothic.timer.cancel(heartbeatId)
```
