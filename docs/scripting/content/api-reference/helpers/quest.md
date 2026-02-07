# opengothic.quest

The `opengothic.quest` table provides quest-log helpers.

These helpers use dedicated quest hooks exposed by the engine:

- `opengothic._questCreateTopic`
- `opengothic._questSetTopicStatus`
- `opengothic._questAddEntry`

These helpers are non-throwing from Lua side: if a hook is unavailable they log to stdout with `print(...)` and return.
They require an active world/session with a quest log.

---

### `opengothic.quest.STATUS`

Status constants:

| Constant | Value | Description |
|---|---:|---|
| `FREE` | `0` | Quest is not active |
| `RUNNING` | `1` | Quest is in progress |
| `SUCCESS` | `2` | Quest completed successfully |
| `FAILED` | `3` | Quest failed |
| `OBSOLETE` | `4` | Quest is obsolete |

### `opengothic.quest.SECTION`

Section constants:

| Constant | Value | Description |
|---|---:|---|
| `MISSIONS` | `0` | Mission section |
| `NOTES` | `1` | Notes section |

---

### `opengothic.quest.create(topicName, [section])`

Creates a new quest topic.

- `topicName` (string): Topic identifier.
- `section` (number, optional): One of `opengothic.quest.SECTION.*`. Default: `MISSIONS`.
- Returns: `nil`

```lua
opengothic.quest.create("MY_QUEST", opengothic.quest.SECTION.MISSIONS)
opengothic.quest.create("MY_NOTE", opengothic.quest.SECTION.NOTES)
```

---

### `opengothic.quest.setState(topicName, status)`

Sets topic status.

- `topicName` (string): Topic identifier.
- `status` (number or string): Status number or case-insensitive string key (for example `"running"`).
- Returns: `nil`

```lua
opengothic.quest.setState("MY_QUEST", opengothic.quest.STATUS.RUNNING)
opengothic.quest.setState("MY_QUEST", "success")
```

---

### `opengothic.quest.addEntry(topicName, entryText)`

Adds a log entry to a topic.

- `topicName` (string): Topic identifier.
- `entryText` (string): Entry text.
- Returns: `nil`

```lua
opengothic.quest.addEntry("MY_QUEST", "I should speak to the merchant.")
```
