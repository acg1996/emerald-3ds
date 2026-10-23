/*
 * The redesigned teams of the leaders and the Elite Four
 * (include/3ds_trainer_teams.h, docs/DIFFICULTY_PLAN.md). A game translation
 * unit. Each team was decided leader by leader; the game's own formats keep no
 * ability, nature or EVs, so they live here.
 */
#include "global.h"
#include "battle.h"
#include "data.h"
#include "pokemon.h"
#include "constants/abilities.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "3ds_extras.h"
#include "3ds_trainer_teams.h"

enum
{
    TEAMS_OFF,
    TEAMS_LEADERS,
    TEAMS_LEADERS_AND_ELITE_FOUR
};

/* ABILITY slot 0 or 1 of the species. EVs in the order HP, Atk, Def, Spe, SpA, SpD. */
struct TeamMon
{
    u16 species;
    u8 level;
    u8 abilitySlot;
    u8 nature;
    u8 iv; // 0-31 while the IV setting leaves the game's own
    u16 heldItem;
    u16 moves[MAX_MON_MOVES];
    u8 evs[NUM_STATS];
};

struct Team
{
    u16 trainerNum;
    u8 count;
    const struct TeamMon *mons;
};

/* Roxanne: a team of speed control and staying power; Rock Tomb slows,
 * Block and Thunder Wave keep the target where it is. A fossil fan's team:
 * Anorith is Roxanne's fossil. Rollout (Geodude), Metal Claw and Fury Cutter
 * (Anorith) and Thunder Wave (Nosepass) are ahead of what the species learn
 * at those levels. */
static const struct TeamMon sRoxanne1[] =
{
    {SPECIES_GEODUDE, 12, 0, NATURE_IMPISH, 20, ITEM_SITRUS_BERRY,
     {MOVE_ROLLOUT, MOVE_DEFENSE_CURL, MOVE_ROCK_TOMB, MOVE_ROCK_THROW}, {0, 40, 40, 0, 0, 0}},
    {SPECIES_ARON, 12, 0, NATURE_ADAMANT, 20, ITEM_FOCUS_BAND,
     {MOVE_METAL_CLAW, MOVE_ROCK_TOMB, MOVE_HARDEN, MOVE_HEADBUTT}, {0, 40, 40, 0, 0, 0}},
    {SPECIES_ANORITH, 13, 0, NATURE_ADAMANT, 20, ITEM_QUICK_CLAW,
     {MOVE_METAL_CLAW, MOVE_ROCK_TOMB, MOVE_FURY_CUTTER, MOVE_HARDEN}, {0, 40, 0, 40, 0, 0}},
    {SPECIES_NOSEPASS, 15, 0, NATURE_RELAXED, 20, ITEM_ORAN_BERRY,
     {MOVE_BLOCK, MOVE_ROCK_TOMB, MOVE_THUNDER_WAVE, MOVE_HARDEN}, {40, 0, 40, 0, 0, 0}},
};

/* Brawly: Bulk Up and critical hits, each Pokémon covering a weakness of the
 * type; the ace is a Reversal with a Salac Berry. High Jump Kick (Meditite),
 * Reversal (Makuhita) and Quick Attack (Hitmontop) are ahead of what the
 * species learn at those levels. */
static const struct TeamMon sBrawly1[] =
{
    {SPECIES_MACHOP, 16, 0, NATURE_ADAMANT, 20, ITEM_SCOPE_LENS,
     {MOVE_KARATE_CHOP, MOVE_SEISMIC_TOSS, MOVE_BULK_UP, MOVE_ROCK_TOMB}, {0, 40, 0, 40, 0, 0}},
    {SPECIES_MEDITITE, 16, 0, NATURE_JOLLY, 20, ITEM_LUM_BERRY,
     {MOVE_HI_JUMP_KICK, MOVE_BULK_UP, MOVE_CONFUSION, MOVE_DETECT}, {0, 40, 0, 40, 0, 0}},
    {SPECIES_HITMONTOP, 17, 0, NATURE_JOLLY, 20, ITEM_BLACK_BELT,
     {MOVE_ROLLING_KICK, MOVE_REVENGE, MOVE_PURSUIT, MOVE_QUICK_ATTACK}, {0, 40, 0, 40, 0, 0}},
    {SPECIES_MAKUHITA, 19, 1, NATURE_ADAMANT, 20, ITEM_SALAC_BERRY,
     {MOVE_REVERSAL, MOVE_BULK_UP, MOVE_VITAL_THROW, MOVE_ARM_THRUST}, {40, 40, 0, 0, 0, 0}},
};

/* The rival (Brendan or May: the same team) on Route 110, by the player's
 * starter; the three share Wingull-Electrike-Lombre-Slugma pieces. Levels as in
 * the game. Quick Attack (Wingull), Fake Out and Bubble Beam (Lombre), Mega
 * Drain (Grovyle) and Bulk Up are ahead of what the species learn. */
#define RIVAL_WINGULL \
    {SPECIES_WINGULL, 18, 0, NATURE_TIMID, 20, ITEM_SITRUS_BERRY, \
     {MOVE_WATER_GUN, MOVE_WING_ATTACK, MOVE_SUPERSONIC, MOVE_QUICK_ATTACK}, {0, 0, 0, 40, 40, 0}}
#define RIVAL_ELECTRIKE \
    {SPECIES_ELECTRIKE, 18, 0, NATURE_JOLLY, 20, ITEM_ORAN_BERRY, \
     {MOVE_SPARK, MOVE_THUNDER_WAVE, MOVE_QUICK_ATTACK, MOVE_HOWL}, {0, 40, 0, 40, 0, 0}}
#define RIVAL_LOMBRE \
    {SPECIES_LOMBRE, 18, 0, NATURE_MODEST, 20, ITEM_LUM_BERRY, \
     {MOVE_FAKE_OUT, MOVE_MEGA_DRAIN, MOVE_BUBBLE_BEAM, MOVE_NATURE_POWER}, {0, 0, 0, 0, 40, 40}}
#define RIVAL_SLUGMA \
    {SPECIES_SLUGMA, 18, 1, NATURE_MODEST, 20, ITEM_CHARCOAL, \
     {MOVE_EMBER, MOVE_ROCK_THROW, MOVE_SMOG, MOVE_YAWN}, {40, 0, 0, 0, 40, 0}}

static const struct TeamMon sRival110Treecko[] =
{
    RIVAL_WINGULL,
    RIVAL_ELECTRIKE,
    RIVAL_LOMBRE,
    {SPECIES_COMBUSKEN, 20, 0, NATURE_ADAMANT, 20, ITEM_CHARCOAL,
     {MOVE_DOUBLE_KICK, MOVE_EMBER, MOVE_BULK_UP, MOVE_PECK}, {0, 40, 0, 40, 0, 0}},
};

static const struct TeamMon sRival110Mudkip[] =
{
    RIVAL_SLUGMA,
    RIVAL_WINGULL,
    RIVAL_ELECTRIKE,
    {SPECIES_GROVYLE, 20, 0, NATURE_JOLLY, 20, ITEM_MIRACLE_SEED,
     {MOVE_MEGA_DRAIN, MOVE_FURY_CUTTER, MOVE_QUICK_ATTACK, MOVE_PURSUIT}, {0, 40, 0, 40, 0, 0}},
};

static const struct TeamMon sRival110Torchic[] =
{
    RIVAL_LOMBRE,
    RIVAL_SLUGMA,
    RIVAL_ELECTRIKE,
    {SPECIES_MARSHTOMP, 20, 0, NATURE_MODEST, 20, ITEM_MYSTIC_WATER,
     {MOVE_MUD_SHOT, MOVE_WATER_GUN, MOVE_ROCK_TOMB, MOVE_MUD_SLAP}, {40, 0, 0, 0, 40, 0}},
};

static const struct Team sTeams[] =
{
    {TRAINER_ROXANNE_1, ARRAY_COUNT(sRoxanne1), sRoxanne1},
    {TRAINER_BRAWLY_1, ARRAY_COUNT(sBrawly1), sBrawly1},
    {TRAINER_BRENDAN_ROUTE_110_TREECKO, ARRAY_COUNT(sRival110Treecko), sRival110Treecko},
    {TRAINER_BRENDAN_ROUTE_110_MUDKIP, ARRAY_COUNT(sRival110Mudkip), sRival110Mudkip},
    {TRAINER_BRENDAN_ROUTE_110_TORCHIC, ARRAY_COUNT(sRival110Torchic), sRival110Torchic},
    {TRAINER_MAY_ROUTE_110_TREECKO, ARRAY_COUNT(sRival110Treecko), sRival110Treecko},
    {TRAINER_MAY_ROUTE_110_MUDKIP, ARRAY_COUNT(sRival110Mudkip), sRival110Mudkip},
    {TRAINER_MAY_ROUTE_110_TORCHIC, ARRAY_COUNT(sRival110Torchic), sRival110Torchic},
};

/* The team a trainer has now, or NULL: the setting must reach their class. */
static const struct Team *ActiveTeam(u16 trainerNum)
{
    int scope = CtrExtras_Get("diff_teams");
    u8 class;
    u8 i;

    if (scope == TEAMS_OFF || trainerNum >= TRAINERS_COUNT)
        return NULL;
    class = gTrainers[trainerNum].trainerClass;
    if (class == TRAINER_CLASS_LEADER || class == TRAINER_CLASS_RIVAL ? scope < TEAMS_LEADERS : scope < TEAMS_LEADERS_AND_ELITE_FOUR)
        return NULL;
    for (i = 0; i < ARRAY_COUNT(sTeams); i++)
        if (sTeams[i].trainerNum == trainerNum)
            return &sTeams[i];
    return NULL;
}

/* The same Pokémon every time the battle starts: a personality of the
 * wanted nature, drawn from the trainer and the slot. */
static u32 PersonalityOfNature(u16 trainerNum, u8 slot, u8 nature)
{
    u32 personality = (trainerNum * 0x9E3779B1u) ^ (slot * 0x85EBCA6Bu);

    do
        personality = personality * 1664525u + 1013904223u;
    while (GetNatureFromPersonality(personality) != nature);
    return personality;
}

static void BuildMon(struct Pokemon *mon, const struct TeamMon *team, u16 trainerNum, u8 slot)
{
    u8 i;

    CreateMon(mon, team->species, team->level, team->iv, TRUE,
              PersonalityOfNature(trainerNum, slot, team->nature), OT_ID_RANDOM_NO_SHINY, 0);
    SetMonData(mon, MON_DATA_ABILITY_NUM, &team->abilitySlot);
    SetMonData(mon, MON_DATA_HELD_ITEM, &team->heldItem);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonData(mon, MON_DATA_MOVE1 + i, &team->moves[i]);
        SetMonData(mon, MON_DATA_PP1 + i, &gBattleMoves[team->moves[i]].pp);
    }
    for (i = 0; i < NUM_STATS; i++)
        SetMonData(mon, MON_DATA_HP_EV + i, &team->evs[i]);
    CalculateMonStats(mon);
}

uint8_t CtrTeams_Apply(struct Pokemon *party, uint8_t count, uint16_t trainerNum)
{
    const struct Team *team = ActiveTeam(trainerNum);
    u8 i;

    if (team == NULL || gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
        return count;
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&party[i]);
    for (i = 0; i < team->count; i++)
        BuildMon(&party[i], &team->mons[i], trainerNum, i);
    return team->count;
}

uint8_t CtrTeams_TopLevel(uint16_t trainerNum)
{
    const struct Team *team = ActiveTeam(trainerNum);
    u8 top = 0, i;

    if (team == NULL)
        return 0;
    for (i = 0; i < team->count; i++)
        if (team->mons[i].level > top)
            top = team->mons[i].level;
    return top;
}
