-- NPC Primitives Test Suite
-- Tests all Npc methods using assertion helpers

local Attr = opengothic.CONSTANTS.Attribute
local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Npc Primitives")

    local world = opengothic.player:world()
    local player = world:player()

    -- Identity tests
    test.assert_not_nil(player, "player exists")
    test.assert_true(player:isPlayer(), "isPlayer() returns true for player")
    test.assert_type(player:displayName(), "string", "displayName returns string")
    test.assert_type(player:instanceId(), "number", "instanceId returns number")

    -- Attribute tests
    local hp = player:attribute(Attr.ATR_HITPOINTS)
    local maxHp = player:attribute(Attr.ATR_HITPOINTSMAX)
    test.assert_type(hp, "number", "HP is number")
    test.assert_true(hp > 0, "HP > 0")
    test.assert_true(hp <= maxHp, "HP <= MaxHP")

    local mana = player:attribute(Attr.ATR_MANA)
    local maxMana = player:attribute(Attr.ATR_MANAMAX)
    test.assert_type(mana, "number", "Mana is number")
    test.assert_true(mana >= 0, "Mana >= 0")
    test.assert_true(mana <= maxMana, "Mana <= MaxMana")

    local strength = player:attribute(Attr.ATR_STRENGTH)
    local dexterity = player:attribute(Attr.ATR_DEXTERITY)
    test.assert_type(strength, "number", "Strength is number")
    test.assert_type(dexterity, "number", "Dexterity is number")

    -- Stats tests
    test.assert_type(player:level(), "number", "level returns number")
    test.assert_type(player:experience(), "number", "experience returns number")
    test.assert_type(player:learningPoints(), "number", "learningPoints returns number")
    test.assert_type(player:guild(), "number", "guild returns number")

    -- Position tests
    local x, y, z = player:position()
    test.assert_type(x, "number", "position x is number")
    test.assert_type(y, "number", "position y is number")
    test.assert_type(z, "number", "position z is number")

    -- Rotation tests
    test.assert_type(player:rotation(), "number", "rotation returns number")
    test.assert_type(player:rotationY(), "number", "rotationY returns number")

    -- State tests
    test.assert_eq(player:isDead(), false, "player not dead")
    test.assert_eq(player:isUnconscious(), false, "player not unconscious")
    test.assert_eq(player:isDown(), false, "player not down")
    test.assert_type(player:isTalking(), "boolean", "isTalking returns boolean")
    test.assert_type(player:bodyState(), "number", "bodyState returns number")
    test.assert_type(player:walkMode(), "number", "walkMode returns number")

    -- Attitude tests
    test.assert_type(player:attitude(), "number", "attitude returns number")

    -- Talent tests
    local Talent = opengothic.CONSTANTS.Talent
    if Talent then
        test.assert_type(player:talentSkill(Talent.NPC_TALENT_1H), "number", "talentSkill returns number")
        test.assert_type(player:talentValue(Talent.NPC_TALENT_1H), "number", "talentValue returns number")
        test.assert_type(player:hitChance(Talent.NPC_TALENT_1H), "number", "hitChance returns number")
    end

    -- Protection tests
    local Prot = opengothic.CONSTANTS.Protection
    if Prot then
        test.assert_type(player:protection(Prot.PROT_EDGE), "number", "protection returns number")
    end

    -- Inventory test
    local inv = player:inventory()
    test.assert_not_nil(inv, "player has inventory")

    -- World access test
    local playerWorld = player:world()
    test.assert_not_nil(playerWorld, "player:world() returns world")

    -- Active weapon/spell tests (may be nil)
    local weapon = player:activeWeapon()
    test.assert_true(weapon == nil or type(weapon) == "userdata", "activeWeapon returns nil or Item")
    test.assert_type(player:activeSpell(), "number", "activeSpell returns number")

    test.summary()
end)

print("[Test] NPC Primitives test loaded - runs on world load")
