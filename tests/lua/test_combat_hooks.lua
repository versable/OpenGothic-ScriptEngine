-- Test script for combat and perception hooks
-- All hooks return false to allow default behavior

local WeaponType = {
    FIST = 1,
    MELEE_1H = 3,
    MELEE_2H = 4,
    BOW = 5,
    CROSSBOW = 6,
    MAGE = 7
}

local function weaponTypeName(wtype)
    if wtype == WeaponType.FIST then return "fist"
    elseif wtype == WeaponType.MELEE_1H then return "1h melee"
    elseif wtype == WeaponType.MELEE_2H then return "2h melee"
    elseif wtype == WeaponType.BOW then return "bow"
    elseif wtype == WeaponType.CROSSBOW then return "crossbow"
    elseif wtype == WeaponType.MAGE then return "mage"
    else return "unknown("..wtype..")"
    end
end

opengothic.events.register("onDrawWeapon", function(npc, weaponType)
    print("onDrawWeapon", npc:displayName(), weaponTypeName(weaponType))
    return false
end)

opengothic.events.register("onCloseWeapon", function(npc)
    print("onCloseWeapon", npc:displayName())
    return false
end)

opengothic.events.register("onNpcPerception", function(npc, other, percType)
    -- Only log player perceptions to avoid spam
    if other:isPlayer() then
        print("onNpcPerception", npc:displayName(), "perceives", other:displayName(), "type:"..percType)
    end
    return false
end)

opengothic.events.register("onTrade", function(npc, other, itemId, count, isBuying)
    local action = isBuying and "buys from" or "sells to"
    print("onTrade", npc:displayName(), action, other:displayName(), "itemId:"..itemId, "x"..count)
    return false
end)

print("Combat Hooks Test Script Loaded.")
