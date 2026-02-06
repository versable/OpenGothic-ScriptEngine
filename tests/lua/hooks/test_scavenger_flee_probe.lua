-- Scavenger Flee Probe (manual regression script)
-- Set ENABLE_SCAVENGER_FLEE_PROBE to true when you want to run this in-game.

local ENABLE_SCAVENGER_FLEE_PROBE = false

if not ENABLE_SCAVENGER_FLEE_PROBE then
    return
end

print("[scavenger_status_probe_v1] LOADED")

local SCAVENGER = nil
local YSCAVENGER = nil
local now = 0.0
local nextFleeAt = {} -- keyed by tostring(npc)
local retaliate = {}  -- keyed by tostring(npc)

opengothic.events.register("onUpdate", function(dt)
    if type(dt) == "number" then
        now = now + dt
    end
    return false
end)

opengothic.events.register("onWorldLoaded", function()
    SCAVENGER = opengothic.resolve("SCAVENGER")
    YSCAVENGER = opengothic.resolve("YSCAVENGER")
    now = 0.0
    nextFleeAt = {}
    retaliate = {}
    print("[scavenger_status_probe_v1] world loaded SCAVENGER=" .. tostring(SCAVENGER) .. " YSCAVENGER=" .. tostring(YSCAVENGER))
    return false
end)

opengothic.events.register("onNpcTakeDamage", function(victim, attacker, isSpell, spellId)
    local okInst, inst = pcall(function()
        return victim:instanceId()
    end)
    if not okInst or (inst ~= SCAVENGER and inst ~= YSCAVENGER) then
        return false
    end

    local key = tostring(victim)
    local okPlayer, attackerIsPlayer = pcall(function()
        return attacker:isPlayer()
    end)
    if okPlayer and attackerIsPlayer == true then
        retaliate[key] = true
        print("[scavenger_status_probe_v1] mode=RETALIATE key=" .. key)
    end

    print("[scavenger_status_probe_v1] damage key=" .. key
        .. " attackerIsPlayer=" .. tostring(okPlayer and attackerIsPlayer or "ERR")
        .. " isSpell=" .. tostring(isSpell)
        .. " spellId=" .. tostring(spellId))

    return false
end)

opengothic.events.register("onNpcPerception", function(npc, other, percType)
    local okInst, inst = pcall(function()
        return npc:instanceId()
    end)
    if not okInst or (inst ~= SCAVENGER and inst ~= YSCAVENGER) then
        return false
    end

    local key = tostring(npc)
    local okOtherPlayer, otherIsPlayer = pcall(function()
        return other:isPlayer()
    end)
    local okDist, dist = pcall(function()
        return npc:distanceTo(other)
    end)

    print("[scavenger_status_probe_v1] perc=" .. tostring(percType)
        .. " key=" .. key
        .. " dist=" .. tostring(okDist and dist or "ERR")
        .. " otherIsPlayer=" .. tostring(okOtherPlayer and otherIsPlayer or "ERR")
        .. " mode=" .. (retaliate[key] and "RETALIATE" or "COWARD"))

    if not (okOtherPlayer and otherIsPlayer == true) then
        return false
    end
    if retaliate[key] then
        return false
    end
    if percType ~= 2 then
        return false
    end

    local gate = nextFleeAt[key] or 0.0
    if now < gate then
        print("[scavenger_status_probe_v1] action=BLOCK_DEFAULT cooldown")
        return true
    end

    local okCall, okResult, errResult = pcall(function()
        return opengothic.ai.fleeFrom(npc, other)
    end)

    print("[scavenger_status_probe_v1] fleeFrom_call_ok=" .. tostring(okCall)
        .. " fleeFrom_result=" .. tostring(okResult)
        .. " fleeFrom_err=" .. tostring(errResult))

    if okCall and okResult == true then
        nextFleeAt[key] = now + 0.8
        print("[scavenger_status_probe_v1] action=FLEE_BLOCK_DEFAULT next=" .. tostring(nextFleeAt[key]))
        return true
    end

    print("[scavenger_status_probe_v1] action=ALLOW_DEFAULT")
    return false
end)
