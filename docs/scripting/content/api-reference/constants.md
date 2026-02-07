# Constants

The `opengothic.CONSTANTS` table provides named variables for common "magic numbers" used throughout the game's systems. You should always use these constants instead of raw numbers to make your code readable and resilient to future game changes.

## Accessing Constants

You access constants via the global table:
`opengothic.CONSTANTS.<GroupName>.<ConstantName>`

**Example:**
```lua
-- GOOD: Using a constant
local str = player:attribute(opengothic.CONSTANTS.Attribute.ATR_STRENGTH)

-- BAD: Using a raw number
local str = player:attribute(0) -- What is 0? It's hard to tell.
```

---

## Constant Groups

Below are the available groups of constants.

### `Attribute`
Used for getting and setting primary NPC attributes with `npc:attribute()` and `npc:changeAttribute()`.
- `ATR_HITPOINTS`
- `ATR_HITPOINTSMAX`
- `ATR_MANA`
- `ATR_MANAMAX`
- `ATR_STRENGTH`
- `ATR_DEXTERITY`
- `ATR_REGENERATEHP`
- `ATR_REGENERATEMANA`
- `ATR_MAX`

### `Protection`
Used for getting protection values with `npc:protection()` and `item:protection()`.
- `PROT_BARRIER`
- `PROT_BLUNT`
- `PROT_EDGE`
- `PROT_FIRE`
- `PROT_FLY`
- `PROT_MAGIC`
- `PROT_POINT`
- `PROT_FALL`
- `PROT_MAX`

### `Talent`
Used for getting and setting talent skills and values with `npc:talentSkill()`, `npc:setTalentSkill()`, `npc:talentValue()`, and `npc:hitChance()`.
- `TALENT_UNKNOWN`
- `TALENT_1H`
- `TALENT_2H`
- `TALENT_BOW`
- `TALENT_CROSSBOW`
- `TALENT_PICKLOCK`
- `TALENT_MAGE`
- `TALENT_SNEAK`
- `TALENT_REGENERATE`
- `TALENT_FIREMASTER`
- `TALENT_ACROBAT`
- `TALENT_PICKPOCKET`
- `TALENT_SMITH`
- `TALENT_RUNES`
- `TALENT_ALCHEMY`
- `TALENT_TAKEANIMALTROPHY`
- `TALENT_FOREIGNLANGUAGE`
- `TALENT_WISPDETECTOR`
- `TALENT_C`
- `TALENT_D`
- `TALENT_E`
- `TALENT_MAX_G1`
- `TALENT_MAX_G2`

### `BodyState`
Used for checking an NPC's current physical state with `npc:hasState()`.
- `BS_NONE`
- `BS_WALK`
- `BS_SNEAK`
- `BS_RUN`
- `BS_SPRINT`
- `BS_SWIM`
- `BS_CRAWL`
- `BS_DIVE`
- `BS_JUMP`
- `BS_CLIMB`
- `BS_FALL`
- `BS_SIT`
- `BS_LIE`
- `BS_INVENTORY`
- `BS_ITEMINTERACT`
- `BS_MOBINTERACT`
- `BS_MOBINTERACT_INTERRUPT`
- `BS_TAKEITEM`
- `BS_DROPITEM`
- `BS_THROWITEM`
- `BS_PICKPOCKET`
- `BS_STUMBLE`
- `BS_UNCONSCIOUS`
- `BS_DEAD`
- `BS_AIMNEAR`
- `BS_AIMFAR`
- `BS_HIT`
- `BS_PARADE`
- `BS_CASTING`
- `BS_PETRIFIED`
- `BS_CONTROLLING`
- `BS_MAX`
- `BS_STAND`
- `BS_MOD_HIDDEN`
- `BS_MOD_DRUNK`
- `BS_MOD_NUTS`
- `BS_MOD_BURNING`
- `BS_MOD_CONTROLLED`
- `BS_MOD_TRANSFORMED`
- `BS_FLAG_INTERRUPTABLE`
- `BS_FLAG_FREEHANDS`

### `WalkBit`
Used for setting an NPC's movement mode with `npc:setWalkMode()`.
- `WM_Run`
- `WM_Walk`
- `WM_Sneak`
- `WM_Water`
- `WM_Swim`
- `WM_Dive`

### `Attitude`
Used for getting and setting an NPC's attitude towards the player with `npc:attitude()` and `npc:setAttitude()`.
- `ATT_HOSTILE`
- `ATT_ANGRY`
- `ATT_NEUTRAL`
- `ATT_FRIENDLY`
- `ATT_NULL`

### `PercType`
Used with the `onNpcPerception` event to identify the type of perception that fired.
- `PERC_None`
- `PERC_ASSESSPLAYER`
- `PERC_ASSESSENEMY`
- `PERC_ASSESSFIGHTER`
- `PERC_ASSESSBODY`
- `PERC_ASSESSITEM`
- `PERC_ASSESSMURDER`
- `PERC_ASSESSDEFEAT`
- `PERC_ASSESSDAMAGE`
- `PERC_ASSESSOTHERSDAMAGE`
- `PERC_ASSESSTHREAT`
- `PERC_ASSESSREMOVEWEAPON`
- `PERC_OBSERVEINTRUDER`
- `PERC_ASSESSFIGHTSOUND`
- `PERC_ASSESSQUIETSOUND`
- `PERC_ASSESSWARN`
- `PERC_CATCHTHIEF`
- `PERC_ASSESSTHEFT`
- `PERC_ASSESSCALL`
- `PERC_ASSESSTALK`
- `PERC_ASSESSGIVENITEM`
- `PERC_ASSESSFAKEGUILD`
- `PERC_MOVEMOB`
- `PERC_MOVENPC`
- `PERC_DRAWWEAPON`
- `PERC_OBSERVESUSPECT`
- `PERC_NPCCOMMAND`
- `PERC_ASSESSMAGIC`
- `PERC_ASSESSSTOPMAGIC`
- `PERC_ASSESSCASTER`
- `PERC_ASSESSSURPRISE`
- `PERC_ASSESSENTERROOM`
- `PERC_ASSESSUSEMOB`
- `PERC_Count`
