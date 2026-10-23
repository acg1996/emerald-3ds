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

static const struct Team sTeams[] =
{
    {TRAINER_ROXANNE_1, ARRAY_COUNT(sRoxanne1), sRoxanne1},
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
    if (class == TRAINER_CLASS_LEADER ? scope < TEAMS_LEADERS : scope < TEAMS_LEADERS_AND_ELITE_FOUR)
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
