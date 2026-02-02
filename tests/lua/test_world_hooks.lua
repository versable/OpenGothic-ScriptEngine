-- Test script for world and movement hooks
-- Notification hooks (return value ignored): onNpcSpawn, onNpcRemove, onSwimStart/End, onDiveStart/End
-- Blocking hooks (return true to block): onMobInteract, onJump

opengothic.events.register("onNpcSpawn", function(npc)
    print("onNpcSpawn", npc:displayName())
end)

opengothic.events.register("onNpcRemove", function(npc)
    print("onNpcRemove", npc:displayName())
end)

opengothic.events.register("onMobInteract", function(npc, mob)
    print("onMobInteract", npc:displayName(), "->", "mob")
    return false
end)

opengothic.events.register("onJump", function(npc)
    print("onJump", npc:displayName())
    return false
end)

opengothic.events.register("onSwimStart", function(npc)
    print("onSwimStart", npc:displayName())
end)

opengothic.events.register("onSwimEnd", function(npc)
    print("onSwimEnd", npc:displayName())
end)

opengothic.events.register("onDiveStart", function(npc)
    print("onDiveStart", npc:displayName())
end)

opengothic.events.register("onDiveEnd", function(npc)
    print("onDiveEnd", npc:displayName())
end)

print("World Hooks Test Script Loaded.")
