/*
 * The DIFFICULTY page's settings applied to the game (include/3ds_difficulty.h,
 * docs/DIFFICULTY_PLAN.md). A game translation unit: it reads the trainer
 * tables and the Pokémon the battle built.
 */
#include "global.h"
#include "data.h"
#include "pokemon.h"
#include "event_data.h"
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
