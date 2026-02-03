-- Test script for game event hooks
-- All hooks return false to allow default behavior

opengothic.events.register("onOpen", function(player, container)
    local ctype = container:isContainer() and "container" or "door"
    print("onOpen", player:displayName(), "->", ctype)
    print("  focusName:", container:focusName())
    print("  schemeName:", container:schemeName())
    print("  state:", container:state())
    return false
end)

opengothic.events.register("onRansack", function(player, corpse)
    print("onRansack", player:displayName(), "->", corpse:displayName())
    return false
end)

opengothic.events.register("onNpcTakeDamage", function(victim, attacker, isSpell, spellId)
    print("onNpcTakeDamage", victim:displayName(), "<-", attacker:displayName(), isSpell and "spell:"..spellId or "melee")
    return false
end)

opengothic.events.register("onNpcDeath", function(victim, killer, isDeath)
    local killerName = killer and killer:displayName() or "unknown"
    print("onNpcDeath", victim:displayName(), "<-", killerName, isDeath and "dead" or "unconscious")
    return false
end)

opengothic.events.register("onDialogStart", function(npc, player)
    print("onDialogStart", player:displayName(), "->", npc:displayName())
    return false
end)

opengothic.events.register("onSpellCast", function(caster, target, spellId)
    local targetName = target and target:displayName() or "none"
    print("onSpellCast", caster:displayName(), "->", targetName, "spell:"..spellId)
    return false
end)

print("Game Events Test Script Loaded.")
