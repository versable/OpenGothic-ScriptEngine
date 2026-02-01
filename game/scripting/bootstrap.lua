-- OpenGothic Lua Bootstrap
-- This file sets up the Lua scripting environment

-- Event system
opengothic.events = {
    _handlers = {}
}

function opengothic.events.register(eventName, callback)
    if not opengothic.events._handlers[eventName] then
        opengothic.events._handlers[eventName] = {}
    end
    table.insert(opengothic.events._handlers[eventName], callback)
end

-- Called from C++ to dispatch events
function opengothic._dispatchEvent(eventName, ...)
    local handlers = opengothic.events._handlers[eventName]
    if not handlers then
        return false
    end

    for _, handler in ipairs(handlers) do
        local handled = handler(...)
        if handled then
            return true
        end
    end
    return false
end

-- Print message to game screen
function opengothic.printMessage(msg)
    opengothic._printMessage(msg)
end

-- Extend Npc with convenience methods
function opengothic.Npc:takeAllFrom(srcInv)
    local items = srcInv:items()
    local transferred = {}
    local dstInv = self:inventory()
    local world = self:world()
    for _, item in ipairs(items) do
        if not item.equipped then
            dstInv:transfer(srcInv, item.id, item.count, world)
            table.insert(transferred, {name = item.name, count = item.count})
        end
    end
    return transferred
end
