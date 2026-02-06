-- Item Primitives Test Suite
-- Tests all Item methods using assertion helpers

local test = opengothic.test

opengothic.events.register("onWorldLoaded", function()
    test.suite("Item Primitives")

    local world = opengothic.world()
    local player = opengothic.player()
    local inv = player:inventory()
    local items = inv:items()

    if #items == 0 then
        print("[SKIP] Item Primitives: No items in inventory to test")
        return
    end

    local item = items[1]
    test.assert_not_nil(item, "first item exists")

    -- String properties
    test.assert_type(item:displayName(), "string", "displayName returns string")
    test.assert_type(item:description(), "string", "description returns string")
    local ix, iy, iz = item:position()
    test.assert_type(ix, "number", "position x returns number")
    test.assert_type(iy, "number", "position y returns number")
    test.assert_type(iz, "number", "position z returns number")

    -- Numeric properties
    test.assert_type(item:count(), "number", "count returns number")
    test.assert_true(item:count() > 0, "count > 0")
    test.assert_type(item:cost(), "number", "cost returns number")
    test.assert_type(item:sellCost(), "number", "sellCost returns number")
    test.assert_type(item:clsId(), "number", "clsId returns number")
    test.assert_type(item:weight(), "number", "weight returns number")
    test.assert_type(item:damage(), "number", "damage returns number")
    test.assert_type(item:damageType(), "number", "damageType returns number")
    test.assert_type(item:range(), "number", "range returns number")
    test.assert_type(item:flags(), "number", "flags returns number")

    -- Protection array test
    local Prot = opengothic.CONSTANTS.Protection
    if Prot then
        test.assert_type(item:protection(Prot.PROT_EDGE), "number", "protection returns number")
    end

    -- Boolean type checks
    test.assert_type(item:isEquipped(), "boolean", "isEquipped returns boolean")
    test.assert_type(item:isMission(), "boolean", "isMission returns boolean")
    test.assert_type(item:isGold(), "boolean", "isGold returns boolean")
    test.assert_type(item:isMulti(), "boolean", "isMulti returns boolean")
    test.assert_type(item:is2H(), "boolean", "is2H returns boolean")
    test.assert_type(item:isCrossbow(), "boolean", "isCrossbow returns boolean")
    test.assert_type(item:isRing(), "boolean", "isRing returns boolean")
    test.assert_type(item:isArmor(), "boolean", "isArmor returns boolean")
    test.assert_type(item:isSpellShoot(), "boolean", "isSpellShoot returns boolean")
    test.assert_type(item:isSpellOrRune(), "boolean", "isSpellOrRune returns boolean")
    test.assert_type(item:isSpell(), "boolean", "isSpell returns boolean")
    test.assert_type(item:isRune(), "boolean", "isRune returns boolean")

    -- Test specific item type categorization
    -- At least one should be true if item is equipment
    local isWeapon = item:is2H() or (not item:is2H() and item:damage() > 0)
    local isProtection = item:isArmor() or item:isRing()
    local isMagic = item:isSpell() or item:isRune() or item:isSpellOrRune()
    local isCurrency = item:isGold()
    print("  Item: " .. item:displayName())
    print("  Type flags - weapon-like: " .. tostring(isWeapon) .. ", protection: " .. tostring(isProtection) .. ", magic: " .. tostring(isMagic) .. ", gold: " .. tostring(isCurrency))

    test.summary()
end)

print("[Test] Item Primitives test loaded - runs on world load")
