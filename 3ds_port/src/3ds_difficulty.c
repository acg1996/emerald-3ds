/*
 * The DIFFICULTY page's settings applied to the game (include/3ds_difficulty.h,
 * docs/DIFFICULTY_PLAN.md). A game translation unit: it reads the trainer
 * tables and the Pokémon the battle built.
 */
#include <string.h>
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
#include "3ds_trainer_teams.h"

enum
{
    IV_SCOPE_ORIGINAL,
    IV_SCOPE_LEADERS,
    IV_SCOPE_ALL
};

#define IV_GOOD_MIN 25
#define IV_PERFECT 31

/* Gym leaders (and their rematches), the Elite Four, the Champion and the rival. */
static bool8 IsLeaderOrEliteFour(u16 trainerNum)
{
    u8 class = gTrainers[trainerNum].trainerClass;

    return class == TRAINER_CLASS_LEADER || class == TRAINER_CLASS_ELITE_FOUR || class == TRAINER_CLASS_CHAMPION
        || class == TRAINER_CLASS_RIVAL;
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
    u8 top = CtrTeams_TopLevel(trainerNum), lvl;
    u8 i;

    if (top != 0)
        return top;
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
 * when it is safe to (it survives the target's best move, or the best of both
 * foes in a double battle). A move that hits the partner too is weighed by the
 * damage the partner would take.
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
/* A move that hits the partner too: down if it would knock it out, or take half
 * its health, up a little if the partner is immune. */
#define AI_ALLY_KO_PENALTY 6
#define AI_ALLY_HURT_PENALTY 3
#define AI_ALLY_SPARED_BONUS 1
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

/* The best damage every foe of the battler could do to it this turn: both
 * foes in a double battle. */
static s32 ThreatTo(u8 battler)
{
    s32 total = 0;
    u8 i;

    for (i = 0; i < gBattlersCount; i++)
        if (GetBattlerSide(i) != GetBattlerSide(battler) && !(gAbsentBattlerFlags & (1 << i)) && gBattleMons[i].hp > 0)
            total += BestDamage(i, battler);
    return total;
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
    u8 partner = BATTLE_PARTNER(battler);
    bool8 partnerHere = (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) && !(gAbsentBattlerFlags & (1 << partner))
                        && gBattleMons[partner].hp > 0;
    s32 threat;
    bool8 canKo = FALSE;
    s32 damage[MAX_MON_MOVES];
    s32 allyDamage[MAX_MON_MOVES];
    u8 i, stat;

    /* A move aimed at the ally is not one the scripts want: the game drops it
     * unless it scores 100. */
    if (!CtrAi_IsSmart(battler) || GetBattlerSide(target) == GetBattlerSide(battler))
        return;
    threat = ThreatTo(battler);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = gBattleMons[battler].moves[i];

        damage[i] = scores[i] > 0 ? SimulatedDamage(battler, target, move) : 0;
        /* Earthquake, Surf and the explosions also hit the partner. */
        allyDamage[i] = (partnerHere && damage[i] > 0 && (gBattleMoves[move].target & MOVE_TARGET_FOES_AND_ALLY))
                      ? SimulatedDamage(battler, partner, move) : -1;
        if (damage[i] >= targetHp && (allyDamage[i] < 0 || allyDamage[i] < gBattleMons[partner].hp))
            canKo = TRUE;
    }
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = gBattleMons[battler].moves[i];

        if (scores[i] <= 0 || move == MOVE_NONE)
            continue;
        if (allyDamage[i] >= 0)
        {
            s32 partnerHp = gBattleMons[partner].hp;

            if (allyDamage[i] >= partnerHp)
            {
                AddScore(&scores[i], -AI_ALLY_KO_PENALTY);
                continue;
            }
            if (allyDamage[i] * 2 >= partnerHp)
            {
                AddScore(&scores[i], -AI_ALLY_HURT_PENALTY);
                continue;
            }
            if (allyDamage[i] == 0)
                AddScore(&scores[i], AI_ALLY_SPARED_BONUS);
        }
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

/*
 * Switching. A smart trainer sends in the Pokémon that fares best against the
 * foes on the field (the game's own pick takes the one that suffers most),
 * keeps its last Pokémon, the ace, back until two are left, and switches out a
 * Pokémon that cannot hurt the foes and is about to be hurt by them. Damage is
 * an estimate from the stats and moves, in percent of the defender's health.
 */
struct Fighter
{
    u16 species;
    u16 atk, spAtk, def, spDef, speed, hp;
    u16 moves[MAX_MON_MOVES];
    u8 ability, level;
    u8 types[2];
};

/* Switch out when the foes would take this much, or more, of its health ... */
#define AI_SWITCH_HURT_PCT 50
/* ... and none of its moves does this much to a foe ... */
#define AI_SWITCH_THREAT_PCT 40
/* ... a Pokémon in the party is this much better ... */
#define AI_SWITCH_MARGIN 50
/* ... and survives a hit (the foes take a free turn). */
#define AI_SWITCH_SURVIVE_PCT 60
/* Turns before a trainer's Pokémon may switch out by choice again. */
#define AI_SWITCH_COOLDOWN 3
/* A damage estimate is capped so one huge hit does not decide the score. */
#define AI_PCT_CAP 150

static u8 sLastSwitchTurn[MAX_BATTLERS_COUNT];

static void FighterOfBattler(struct Fighter *f, u8 battler)
{
    const struct BattlePokemon *mon = &gBattleMons[battler];
    u8 i;

    f->species = mon->species;
    f->atk = mon->attack;
    f->spAtk = mon->spAttack;
    f->def = mon->defense;
    f->spDef = mon->spDefense;
    f->speed = mon->speed;
    f->hp = mon->hp;
    f->ability = mon->ability;
    f->level = mon->level;
    f->types[0] = mon->types[0];
    f->types[1] = mon->types[1];
    for (i = 0; i < MAX_MON_MOVES; i++)
        f->moves[i] = mon->moves[i];
}

static void FighterOfMon(struct Fighter *f, struct Pokemon *mon)
{
    u8 i;

    f->species = GetMonData(mon, MON_DATA_SPECIES);
    f->atk = GetMonData(mon, MON_DATA_ATK);
    f->spAtk = GetMonData(mon, MON_DATA_SPATK);
    f->def = GetMonData(mon, MON_DATA_DEF);
    f->spDef = GetMonData(mon, MON_DATA_SPDEF);
    f->speed = GetMonData(mon, MON_DATA_SPEED);
    f->hp = GetMonData(mon, MON_DATA_HP);
    f->ability = GetMonAbility(mon);
    f->level = GetMonData(mon, MON_DATA_LEVEL);
    f->types[0] = gSpeciesInfo[f->species].types[0];
    f->types[1] = gSpeciesInfo[f->species].types[1];
    for (i = 0; i < MAX_MON_MOVES; i++)
        f->moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
}

/* What one move of a does to d, in percent of d's health. */
static s32 PctOfMove(const struct Fighter *a, const struct Fighter *d, u16 move)
{
    u8 type, flags;
    s32 atk, def, damage;

    if (move == MOVE_NONE || gBattleMoves[move].power < 2)
        return 0;
    type = gBattleMoves[move].type;
    flags = AI_TypeCalc(move, d->species, d->ability);
    if (flags & MOVE_RESULT_DOESNT_AFFECT_FOE)
        return 0;
    atk = IS_TYPE_PHYSICAL(type) ? a->atk : a->spAtk;
    def = IS_TYPE_PHYSICAL(type) ? d->def : d->spDef;
    damage = (2 * a->level / 5 + 2) * gBattleMoves[move].power * atk / (def > 0 ? def : 1) / 50 + 2;
    if (type == a->types[0] || type == a->types[1])
        damage = damage * 3 / 2;
    flags &= MOVE_RESULT_SUPER_EFFECTIVE | MOVE_RESULT_NOT_VERY_EFFECTIVE;
    if (flags == MOVE_RESULT_SUPER_EFFECTIVE)
        damage *= 2;
    else if (flags == MOVE_RESULT_NOT_VERY_EFFECTIVE)
        damage /= 2;
    damage = damage * 100 / (d->hp > 0 ? d->hp : 1);
    return damage > AI_PCT_CAP ? AI_PCT_CAP : damage;
}

static s32 BestPct(const struct Fighter *a, const struct Fighter *d)
{
    s32 best = 0;
    u8 i;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        s32 pct = PctOfMove(a, d, a->moves[i]);

        if (pct > best)
            best = pct;
    }
    return best;
}

/* How f fares against the foes of the battler: what it does minus what it takes,
 * averaged over the foes, and the most it does and takes to any one of them. */
static s32 ScoreAgainstFoes(const struct Fighter *f, u8 battler, s32 *dealtMax, s32 *takenMax)
{
    s32 total = 0, foes = 0;
    u8 i;

    *dealtMax = 0;
    *takenMax = 0;
    for (i = 0; i < gBattlersCount; i++)
    {
        struct Fighter foe;
        s32 dealt, taken, score;

        if (GetBattlerSide(i) == GetBattlerSide(battler) || (gAbsentBattlerFlags & (1 << i)) || gBattleMons[i].hp == 0)
            continue;
        FighterOfBattler(&foe, i);
        dealt = BestPct(f, &foe);
        taken = BestPct(&foe, f);
        score = dealt - taken;
        if (dealt >= 100 && (f->speed > foe.speed || taken < 100))
            score += 40;
        if (taken >= 100 && !(dealt >= 100 && f->speed > foe.speed))
            score -= 40;
        total += score;
        foes++;
        if (dealt > *dealtMax)
            *dealtMax = dealt;
        if (taken > *takenMax)
            *takenMax = taken;
    }
    return foes ? total / foes : 0;
}

static void PartyRange(u8 battler, u8 *first, u8 *last)
{
    if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
    {
        *first = (battler & BIT_FLANK) == B_FLANK_LEFT ? 0 : PARTY_SIZE / 2;
        *last = (battler & BIT_FLANK) == B_FLANK_LEFT ? PARTY_SIZE / 2 : PARTY_SIZE;
    }
    else
    {
        *first = 0;
        *last = PARTY_SIZE;
    }
}

/* The party slot to send in, or PARTY_SIZE when there is none to choose. */
static u8 PickSwitchIn(u8 battler, s32 *bestScore, s32 *bestDealt, s32 *bestTaken)
{
    u8 partner = BATTLE_PARTNER(battler);
    bool8 partnerHere = (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) && !(gAbsentBattlerFlags & (1 << partner));
    u8 eligible[PARTY_SIZE];
    u8 first, last, ace = PARTY_SIZE, count = 0, alive = 0, best = PARTY_SIZE, i, j;

    PartyRange(battler, &first, &last);
    for (i = first; i < last; i++)
    {
        u16 species = GetMonData(&gEnemyParty[i], MON_DATA_SPECIES_OR_EGG);

        if (species == SPECIES_NONE || species == SPECIES_EGG)
            continue;
        ace = i;
        if (GetMonData(&gEnemyParty[i], MON_DATA_HP) == 0)
            continue;
        alive++;
        if (i == gBattlerPartyIndexes[battler] || i == gBattleStruct->monToSwitchIntoId[battler]
         || (partnerHere && (i == gBattlerPartyIndexes[partner] || i == gBattleStruct->monToSwitchIntoId[partner])))
            continue;
        eligible[count++] = i;
    }
    /* The ace stays in the back while the team still has more than two. */
    if (count > 1 && alive > 2)
        for (i = 0; i < count; i++)
            if (eligible[i] == ace)
            {
                eligible[i] = eligible[--count];
                break;
            }
    *bestScore = -10000;
    *bestDealt = 0;
    *bestTaken = 0;
    for (i = 0; i < count; i++)
    {
        struct Fighter f;
        s32 dealt, taken, score;

        FighterOfMon(&f, &gEnemyParty[eligible[i]]);
        score = ScoreAgainstFoes(&f, battler, &dealt, &taken);
        /* A partner's Earthquake or Surf would hit the one that comes in. */
        if (partnerHere && gBattleMons[partner].hp > 0)
        {
            struct Fighter p;
            s32 worst = 0;

            FighterOfBattler(&p, partner);
            for (j = 0; j < MAX_MON_MOVES; j++)
                if (p.moves[j] != MOVE_NONE && (gBattleMoves[p.moves[j]].target & MOVE_TARGET_FOES_AND_ALLY))
                {
                    s32 pct = PctOfMove(&p, &f, p.moves[j]);

                    if (pct > worst)
                        worst = pct;
                }
            if (worst >= 100)
                score -= 80;
            else if (worst >= 50)
                score -= 40;
        }
        if (score > *bestScore)
        {
            *bestScore = score;
            *bestDealt = dealt;
            *bestTaken = taken;
            best = eligible[i];
        }
    }
    return best;
}

uint8_t CtrAi_BestSwitchIn(uint8_t battler)
{
    s32 score, dealt, taken;

    return PickSwitchIn(battler, &score, &dealt, &taken);
}

bool CtrAi_ShouldSwitchOut(uint8_t battler)
{
    struct Fighter cur;
    s32 curScore, curDealt, curTaken, bestScore, bestDealt, bestTaken;
    u8 turn = gBattleResults.battleTurnCounter;

    if (!CtrAi_IsSmart(battler))
        return false;
    if (turn == 0)
        memset(sLastSwitchTurn, 0, sizeof(sLastSwitchTurn));
    if (sLastSwitchTurn[battler] != 0 && turn + 1 - sLastSwitchTurn[battler] < AI_SWITCH_COOLDOWN)
        return false;
    FighterOfBattler(&cur, battler);
    curScore = ScoreAgainstFoes(&cur, battler, &curDealt, &curTaken);
    if (curTaken < AI_SWITCH_HURT_PCT || curDealt >= AI_SWITCH_THREAT_PCT)
        return false;
    if (PickSwitchIn(battler, &bestScore, &bestDealt, &bestTaken) == PARTY_SIZE
     || bestScore < curScore + AI_SWITCH_MARGIN || bestTaken >= AI_SWITCH_SURVIVE_PCT)
        return false;
    sLastSwitchTurn[battler] = turn + 1;
    return true;
}
