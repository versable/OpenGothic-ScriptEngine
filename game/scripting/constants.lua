-- game/scripting/constants.lua
opengothic.CONSTANTS = {}

opengothic.CONSTANTS.Attribute = {
    ATR_HITPOINTS      = 0,
    ATR_HITPOINTSMAX   = 1,
    ATR_MANA           = 2,
    ATR_MANAMAX        = 3,
    ATR_STRENGTH       = 4,
    ATR_DEXTERITY      = 5,
    ATR_REGENERATEHP   = 6,
    ATR_REGENERATEMANA = 7,
    ATR_MAX            = 8
}

opengothic.CONSTANTS.Talent = {
    TALENT_UNKNOWN          = 0,
    TALENT_1H               = 1,
    TALENT_2H               = 2,
    TALENT_BOW              = 3,
    TALENT_CROSSBOW         = 4,
    TALENT_PICKLOCK         = 5,
    TALENT_MAGE             = 7,
    TALENT_SNEAK            = 8,
    TALENT_REGENERATE       = 9,
    TALENT_FIREMASTER       = 10,
    TALENT_ACROBAT          = 11,
    TALENT_PICKPOCKET       = 12,
    TALENT_SMITH            = 13,
    TALENT_RUNES            = 14,
    TALENT_ALCHEMY          = 15,
    TALENT_TAKEANIMALTROPHY = 16,
    TALENT_FOREIGNLANGUAGE  = 17,
    TALENT_WISPDETECTOR     = 18,
    TALENT_C                = 19,
    TALENT_D                = 20,
    TALENT_E                = 21,
    TALENT_MAX_G1           = 12,
    TALENT_MAX_G2           = 22
}

opengothic.CONSTANTS.WalkBit = {
    WM_Run   = 0,
    WM_Walk  = 1,
    WM_Sneak = 2,
    WM_Water = 4,
    WM_Swim  = 8,
    WM_Dive  = 16
}

opengothic.CONSTANTS.BodyState = {
    BS_NONE                  = 0,
    BS_MOD_HIDDEN            = bit32.lshift(1, 7),
    BS_MOD_DRUNK             = bit32.lshift(1, 8),
    BS_MOD_NUTS              = bit32.lshift(1, 9),
    BS_MOD_BURNING           = bit32.lshift(1, 10),
    BS_MOD_CONTROLLED        = bit32.lshift(1, 11),
    BS_MOD_TRANSFORMED       = bit32.lshift(1, 12),
    BS_FLAG_INTERRUPTABLE    = bit32.lshift(1, 15),
    BS_FLAG_FREEHANDS        = bit32.lshift(1, 16),
    BS_WALK                  = 1,
    BS_SNEAK                 = 2,
    BS_RUN                   = 3,
    BS_SPRINT                = 4,
    BS_SWIM                  = 5,
    BS_CRAWL                 = 6,
    BS_DIVE                  = 7,
    BS_JUMP                  = 8,
    BS_CLIMB                 = 9,
    BS_FALL                  = 10,
    BS_SIT                   = 11,
    BS_LIE                   = 12,
    BS_INVENTORY             = 13,
    BS_ITEMINTERACT          = 14,
    BS_MOBINTERACT           = 15,
    BS_MOBINTERACT_INTERRUPT = 16,
    BS_TAKEITEM              = 17,
    BS_DROPITEM              = 18,
    BS_THROWITEM             = 19,
    BS_PICKPOCKET            = 20,
    BS_STUMBLE               = 21,
    BS_UNCONSCIOUS           = 22,
    BS_DEAD                  = 23,
    BS_AIMNEAR               = 24,
    BS_AIMFAR                = 25,
    BS_HIT                   = 26,
    BS_PARADE                = 27,
    BS_CASTING               = 28,
    BS_PETRIFIED             = 29,
    BS_CONTROLLING           = 30,
    BS_MAX                   = 31,
    BS_STAND                 = bit32.bor(bit32.lshift(1, 15), bit32.lshift(1, 16))
}

opengothic.CONSTANTS.Attitude = {
    ATT_HOSTILE  = 0,
    ATT_ANGRY    = 1,
    ATT_NEUTRAL  = 2,
    ATT_FRIENDLY = 3,
    ATT_NULL     = -1
}

opengothic.CONSTANTS.Protection = {
    PROT_BARRIER = 0,
    PROT_BLUNT   = 1,
    PROT_EDGE    = 2,
    PROT_FIRE    = 3,
    PROT_FLY     = 4,
    PROT_MAGIC   = 5,
    PROT_POINT   = 6,
    PROT_FALL    = 7,
    PROT_MAX     = 8
}