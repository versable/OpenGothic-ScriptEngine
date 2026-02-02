-- Lua test script for new lifecycle and settings hooks

-- Register handlers for each new hook
opengothic.events.register("onStartGame", function(worldName)
    print("onStartGame", worldName)
    return false
end)

opengothic.events.register("onLoadGame", function(savegameName)
    print("onLoadGame", savegameName)
    return false
end)

opengothic.events.register("onSaveGame", function(slotName, userName)
    print("onSaveGame", slotName, userName)
    return false
end)

opengothic.events.register("onWorldLoaded", function()
    print("onWorldLoaded")
    return false
end)

opengothic.events.register("onStartLoading", function()
    print("onStartLoading")
    return false
end)

opengothic.events.register("onSessionExit", function()
    print("onSessionExit")
    return false
end)

opengothic.events.register("onSettingsChanged", function()
    print("onSettingsChanged")
    return false
end)

print("Lifecycle Hooks Test Script Loaded.")
