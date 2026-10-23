/*
 * The DIFFICULTY page's settings applied to the game (include/3ds_difficulty.h,
 * docs/DIFFICULTY_PLAN.md). A game translation unit: it reads the trainer
 * tables and the Pokémon the battle built.
 */
#include "global.h"
#include "data.h"
#include "pokemon.h"
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
