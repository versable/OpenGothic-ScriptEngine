# Interactive API

An `Interactive` object represents a "mobsi" in the world: chests, doors, switches, beds, thrones, and similar usable objects.

---

### `interactive:focusName()`

- **Returns**: `name` (string) - The display name shown when focused.

### `interactive:schemeName()`

- **Returns**: `name` (string) - Internal scheme/type name (for example `"CHEST"` or `"DOOR"`).

### `interactive:state()`

- **Returns**: `stateId` (number) - Current interactive state.

---

### `interactive:isContainer()`

- **Returns**: `boolean` - `true` if this interactive acts as a container.

### `interactive:isDoor()`

- **Returns**: `boolean` - `true` if this interactive is a door.

### `interactive:isLadder()`

- **Returns**: `boolean` - `true` if this interactive is a ladder.

---

### `interactive:inventory()`

- **Returns**: `Inventory` or `nil` - Inventory object for container interactives.

### `interactive:needToLockpick(player)`

- `player` (Npc): NPC checking lockpicking requirement.
- **Returns**: `boolean` - `true` if lockpicking is needed.

### `interactive:isCracked()`

- **Returns**: `boolean` - `true` if the lock/container is already cracked.

### `interactive:setAsCracked(cracked)`

- `cracked` (boolean): New cracked-state value.

---

### `interactive:isTrueDoor(npc)`

- `npc` (Npc): NPC context used for door checks.
- **Returns**: `boolean` - `true` if object behaves as a usable door for `npc`.

---

### `interactive:attach(npc)`

- `npc` (Npc): NPC to attach to this interactive.
- **Returns**: `success` (boolean).

### `interactive:detach(npc, quick)`

- `npc` (Npc): NPC to detach.
- `quick` (boolean): If `true`, force quicker detachment.
- **Returns**: `success` (boolean).
