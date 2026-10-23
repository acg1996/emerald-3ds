/*
 * The DIFFICULTY page's settings applied to the game (include/3ds_difficulty.h,
 * docs/DIFFICULTY_PLAN.md). A game translation unit: it reads the trainer
 * tables and the Pokémon the battle built.
 */
#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_setup.h"
#include "battle_script_commands.h"
#include "data.h"
#include "pokemon.h"
#include "event_data.h"
#include "constants/battle.h"
#include "constants/battle_ai.h"
#include "constants/battle_move_effects.h"
#include "constants/flags.h"
#include "constants/opponents.h"
#include "constants/trainers.h"
#include "3ds_extras.h"
#include "3ds_difficulty.h"

enum
{
    IV_SCOPE_ORIGINAL,
    IV_SCOPE_LEADERS,
    IV_SCOPE_ALL
};

#define IV_GOOD_MIN 25
#define IV_PERFECT 31

/* Gym leaders (and their rematches), the Elite Four and the Champion. */
static bool8 IsLeaderOrEliteFour(u16 trainerNum)
{
    u8 class = gTrainers[trainerNum].trainerClass;

    return class == TRAINER_CLASS_LEADER || class == TRAINER_CLASS_ELITE_FOUR || class == TRAINER_CLASS_CHAMPION;
}

/*
 * One stat's IV for a Pokémon: 25 to 31 from its personality and the stat, so
 * a battle started again builds the same opponent.
 */
static u8 GoodIv(u32 personality, u8 stat)
{
    u32 hash = (personality ^ (stat * 0x9E3779B1u)) * 2654435761u;

    return IV_GOOD_MIN + (hash >> 16) % (IV_PERFECT - IV_GOOD_MIN + 1);
}

void CtrDifficulty_ApplyTrainerIvs(struct Pokemon *party, uint8_t count, uint16_t trainerNum)
{
    int scope = CtrExtras_Get("diff_iv_scope");
    bool8 perfect = CtrExtras_Get("diff_iv_level") == 1;
    u8 i, stat;

    if (scope == IV_SCOPE_ORIGINAL || (scope == IV_SCOPE_LEADERS && !IsLeaderOrEliteFour(trainerNum)))
        return;
    for (i = 0; i < count; i++)
    {
        u32 personality = GetMonData(&party[i], MON_DATA_PERSONALITY, NULL);

        for (stat = 0; stat < NUM_STATS; stat++)
        {
            u8 iv = perfect ? IV_PERFECT : GoodIv(personality, stat);

            SetMonData(&party[i], MON_DATA_HP_IV + stat, &iv);
        }
        CalculateMonStats(&party[i]);
    }
}

/*
 * The level cap. The leaders' order is the game's usual one: the cap is the
 * first of them whose badge is still missing, whatever order the badges were
 * won in.
 */
enum
{
    CAP_OFF,
    CAP_LEADERS,
    CAP_LEADERS_AND_ELITE_FOUR
};

static const u16 sLeaders[NUM_BADGES] =
{
    TRAINER_ROXANNE_1, TRAINER_BRAWLY_1, TRAINER_WATTSON_1, TRAINER_FLANNERY_1,
    TRAINER_NORMAN_1, TRAINER_WINONA_1, TRAINER_TATE_AND_LIZA_1, TRAINER_JUAN_1,
};
/* The Elite Four in the order they are fought, then the Champion. */
static const u16 sEliteFour[] = {TRAINER_SIDNEY, TRAINER_PHOEBE, TRAINER_GLACIA, TRAINER_DRAKE};
static const u16 sEliteFourDefeated[] =
{
    FLAG_DEFEATED_ELITE_4_SIDNEY, FLAG_DEFEATED_ELITE_4_PHOEBE,
    FLAG_DEFEATED_ELITE_4_GLACIA, FLAG_DEFEATED_ELITE_4_DRAKE,
};

/* The highest level in a trainer's party, whichever way it is stored. */
static u8 TopLevel(u16 trainerNum)
{
    const struct Trainer *trainer = &gTrainers[trainerNum];
    u8 top = 0, lvl;
    u8 i;

    for (i = 0; i < trainer->partySize; i++)
    {
        switch (trainer->partyFlags)
        {
        case F_TRAINER_PARTY_CUSTOM_MOVESET | F_TRAINER_PARTY_HELD_ITEM:
            lvl = trainer->party.ItemCustomMoves[i].lvl;
            break;
        case F_TRAINER_PARTY_CUSTOM_MOVESET:
            lvl = trainer->party.NoItemCustomMoves[i].lvl;
            break;
        case F_TRAINER_PARTY_HELD_ITEM:
            lvl = trainer->party.ItemDefaultMoves[i].lvl;
            break;
        default:
            lvl = trainer->party.NoItemDefaultMoves[i].lvl;
            break;
        }
        if (lvl > top)
            top = lvl;
    }
    return top;
}

uint8_t CtrDifficulty_LevelCap(void)
{
    int scope = CtrExtras_Get("diff_level_cap");
    u8 i;

    if (scope == CAP_OFF || FlagGet(FLAG_IS_CHAMPION))
        return 0;
    for (i = 0; i < NUM_BADGES; i++)
        if (!FlagGet(FLAG_BADGE01_GET + i))
            return TopLevel(sLeaders[i]);
    if (scope != CAP_LEADERS_AND_ELITE_FOUR)
        return 0;
    for (i = 0; i < ARRAY_COUNT(sEliteFour); i++)
        if (!FlagGet(sEliteFourDefeated[i]))
            return TopLevel(sEliteFour[i]);
    return TopLevel(TRAINER_WALLACE);
}

bool CtrDifficulty_AtLevelCap(const struct Pokemon *mon)
{
    u8 cap = CtrDifficulty_LevelCap();

    return cap != 0 && GetMonData((struct Pokemon *)mon, MON_DATA_LEVEL, NULL) >= cap;
}

int32_t CtrDifficulty_ClampExp(const struct Pokemon *mon, int32_t exp)
{
    u8 cap = CtrDifficulty_LevelCap();
    u16 species;
    s32 room;

    if (cap == 0 || cap >= MAX_LEVEL)
        return exp;
    species = GetMonData((struct Pokemon *)mon, MON_DATA_SPECIES, NULL);
    /* Level cap + 1 starts one point above this: the last point before it. */
    room = (s32)gExperienceTables[gSpeciesInfo[species].growthRate][cap + 1] - 1
         - (s32)GetMonData((struct Pokemon *)mon, MON_DATA_EXP, NULL);
    return room < 0 ? 0 : (exp < room ? exp : room);
}

/*
 * The trainer AI. A smart trainer runs the sturdy scripts and, on top of what
 * they score, prefers a knock-out and a big hit, and raises its stats only
 * when it is safe to (it survives the target's best move, or two of them).
 * Like the game's own AI it reads the target's real moves.
 */
enum
{
    AI_SCOPE_ORIGINAL,
    AI_SCOPE_LEADERS,
    AI_SCOPE_ALL
};

#define AI_KO_BONUS 5
#define AI_BIG_HIT_BONUS 1
#define AI_SAFE_SETUP_BONUS 3
#define AI_OK_SETUP_BONUS 1
#define AI_UNSAFE_SETUP_PENALTY 4
/* Stat stages above the default a raised stat stops at (+2). */
#define AI_SETUP_STAGE_CAP 2

/* Battles with a trainer's own AI that is not a story script: not the first
 * battle, Wally's, a recorded or a link battle. */
#define AI_EXCLUDED_BATTLES (BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_WALLY_TUTORIAL | BATTLE_TYPE_RECORDED \
                             | BATTLE_TYPE_LINK | BATTLE_TYPE_SAFARI | BATTLE_TYPE_ROAMER)
/* Trainers who are not in the trainer table. */
#define AI_OWN_TRAINER_BATTLES (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_HILL \
                                | BATTLE_TYPE_SECRET_BASE | BATTLE_TYPE_FACTORY)

static bool8 IsLeaderOrEliteFourSafe(u16 trainerNum)
{
    return trainerNum < TRAINERS_COUNT && IsLeaderOrEliteFour(trainerNum);
}

bool CtrAi_IsSmart(uint8_t battler)
{
    int scope = CtrExtras_Get("diff_ai");

    if (scope == AI_SCOPE_ORIGINAL || GetBattlerSide(battler) != B_SIDE_OPPONENT)
        return false;
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER) || (gBattleTypeFlags & AI_EXCLUDED_BATTLES))
        return false;
    if (scope == AI_SCOPE_ALL)
        return true;
    if (gBattleTypeFlags & AI_OWN_TRAINER_BATTLES)
        return false;
    return IsLeaderOrEliteFourSafe(gTrainerBattleOpponent_A)
        || ((gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS) && IsLeaderOrEliteFourSafe(gTrainerBattleOpponent_B));
}

uint32_t CtrAi_BaseFlags(void)
{
    return AI_SCRIPT_CHECK_BAD_MOVE | AI_SCRIPT_TRY_TO_FAINT | AI_SCRIPT_CHECK_VIABILITY
         | AI_SCRIPT_SETUP_FIRST_TURN | AI_SCRIPT_HP_AWARE;
}

/* The damage a move would do, by the game's own estimate, or 0. */
static s32 SimulatedDamage(u8 attacker, u8 defender, u16 move)
{
    if (move == MOVE_NONE || gBattleMoves[move].power < 2)
        return 0;
    gDynamicBasePower = 0;
    gBattleStruct->dynamicMoveType = 0;
    gBattleScripting.dmgMultiplier = 1;
    gMoveResultFlags = 0;
    gCritMultiplier = 1;
    gCurrentMove = move;
    AI_CalcDmg(attacker, defender);
    TypeCalc(move, attacker, defender);
    return (gMoveResultFlags & MOVE_RESULT_DOESNT_AFFECT_FOE) ? 0 : gBattleMoveDamage;
}

static s32 BestDamage(u8 attacker, u8 defender)
{
    s32 best = 0;
    u8 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        s32 damage = SimulatedDamage(attacker, defender, gBattleMons[attacker].moves[i]);

        if (damage > best)
            best = damage;
    }
    return best;
}

/* Whether the move only raises the stat this returns, for the ones worth stacking. */
static bool8 RaisedStat(u16 effect, u8 *stat)
{
    switch (effect)
    {
    case EFFECT_ATTACK_UP:
    case EFFECT_ATTACK_UP_2:
    case EFFECT_BULK_UP:
    case EFFECT_DRAGON_DANCE:
        *stat = STAT_ATK;
        return TRUE;
    case EFFECT_SPECIAL_ATTACK_UP:
    case EFFECT_SPECIAL_ATTACK_UP_2:
    case EFFECT_CALM_MIND:
        *stat = STAT_SPATK;
        return TRUE;
    case EFFECT_SPEED_UP:
    case EFFECT_SPEED_UP_2:
        *stat = STAT_SPEED;
        return TRUE;
    default:
        return FALSE;
    }
}

/* A move's score is a signed byte and 0 means the move is not to be chosen:
 * raise it up to 127, lower it down to 1. */
static void AddScore(s8 *score, int delta)
{
    int value = *score + delta;

    *score = value < 1 ? 1 : (value > 127 ? 127 : value);
}

void CtrAi_AdjustScores(uint8_t battler, uint8_t target, int8_t *scores)
{
    u16 savedMove = gCurrentMove;
    s32 savedDamage = gBattleMoveDamage;
    u8 savedFlags = gMoveResultFlags;
    u16 savedPower = gDynamicBasePower;
    u8 savedCrit = gCritMultiplier;
    u8 savedMultiplier = gBattleScripting.dmgMultiplier;
    u8 savedType = gBattleStruct->dynamicMoveType;
    s32 targetHp = gBattleMons[target].hp, myHp = gBattleMons[battler].hp;
    s32 threat;
    bool8 canKo = FALSE;
    s32 damage[MAX_MON_MOVES];
    u8 i, stat;

    if (!CtrAi_IsSmart(battler) || (gBattleTypeFlags & BATTLE_TYPE_DOUBLE))
        return;
    threat = BestDamage(target, battler);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        damage[i] = scores[i] > 0 ? SimulatedDamage(battler, target, gBattleMons[battler].moves[i]) : 0;
        if (damage[i] >= targetHp)
            canKo = TRUE;
    }
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = gBattleMons[battler].moves[i];

        if (scores[i] <= 0 || move == MOVE_NONE)
            continue;
        if (damage[i] >= targetHp)
            AddScore(&scores[i], AI_KO_BONUS);
        else if (damage[i] > 0 && damage[i] * 2 >= targetHp)
            AddScore(&scores[i], AI_BIG_HIT_BONUS);
        else if (RaisedStat(gBattleMoves[move].effect, &stat) && !canKo
              && gBattleMons[battler].statStages[stat] < DEFAULT_STAT_STAGE + AI_SETUP_STAGE_CAP)
        {
            if (threat >= myHp)
                AddScore(&scores[i], -AI_UNSAFE_SETUP_PENALTY);
            else if (threat * 2 < myHp)
                AddScore(&scores[i], AI_SAFE_SETUP_BONUS);
            else
                AddScore(&scores[i], AI_OK_SETUP_BONUS);
        }
    }
    gCurrentMove = savedMove;
    gBattleMoveDamage = savedDamage;
    gMoveResultFlags = savedFlags;
    gDynamicBasePower = savedPower;
    gCritMultiplier = savedCrit;
    gBattleScripting.dmgMultiplier = savedMultiplier;
    gBattleStruct->dynamicMoveType = savedType;
}
