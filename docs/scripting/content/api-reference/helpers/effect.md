# opengothic.effect

The `opengothic.effect` table provides convenience visual-effect helpers.

---

### `opengothic.effect.play(effectName, source, [target])`

Plays an effect at `source` position.

- `effectName` (string): Effect identifier.
- `source` (Npc): Source NPC.
- `target` (Npc, optional): Reserved for compatibility, currently unused.
- **Returns**: `boolean` - `true` on successful world/position resolution.

```lua
local player = opengothic.player()
if player then
    opengothic.effect.play("spellFX_YOUREALLYGETME", player)
end
```
