/*
 * The randomizer (include/3ds_random.h). A game translation unit: it reads the
 * map, the trainer tables and the party.
 */
#include "global.h"
#include "data.h"
#include "pokemon.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "3ds_extras.h"
#include "3ds_nuzlocke.h"
#include "3ds_random.h"

enum
{
    RANDOM_OFF,
    RANDOM_WILD,
    RANDOM_TRAINERS,
    RANDOM_BOSSES,
    RANDOM_ABILITIES_AND_MOVES
};

static int Mode(void)
{
    return CtrExtras_Get("diff_random");
}

/* A number told by the seed and two values. */
static u32 Hash(u32 a, u32 b, u32 salt)
{
    u32 h = CtrNuzlocke_Seed() ^ (salt * 0x9E3779B1u);

    h = (h ^ a) * 0x85EBCA6Bu;
    h ^= h >> 15;
    h = (h ^ b) * 0xC2B2AE35u;
    h ^= h >> 13;
    return h * 0x27D4EB2Fu;
}

/* The Pokémon of the game: Bulbasaur to Celebi, then Treecko to Chimecho. */
static bool8 ValidSpecies(u16 species)
{
    return species >= SPECIES_BULBASAUR && (species <= SPECIES_CELEBI || (species >= SPECIES_TREECKO && species <= SPECIES_CHIMECHO));
}

static u16 RandomSpecies(u32 a, u32 b)
{
    u32 h = Hash(a, b, 1);
    u16 species;

    do
    {
        h = h * 1664525u + 1013904223u;
        species = 1 + (h >> 8) % SPECIES_CHIMECHO;
    } while (!ValidSpecies(species));
    return species;
}

uint16_t CtrRandom_Wild(uint16_t species)
{
    if (Mode() < RANDOM_WILD || species == SPECIES_NONE)
        return species;
    return RandomSpecies((gSaveBlock1Ptr->location.mapGroup << 8) | gSaveBlock1Ptr->location.mapNum, species);
}

uint16_t CtrRandom_Starter(uint16_t species)
{
    if (Mode() < RANDOM_BOSSES)
        return species;
    return RandomSpecies(0x57A7, species);
}

uint16_t CtrRandom_Gift(uint16_t species)
{
    if (Mode() < RANDOM_BOSSES || species == SPECIES_NONE)
        return species;
    return RandomSpecies(0x61F7, species);
}

static bool8 IsBoss(u16 trainerNum)
{
    u8 class;

    if (trainerNum >= TRAINERS_COUNT)
        return FALSE;
    class = gTrainers[trainerNum].trainerClass;
    return class == TRAINER_CLASS_LEADER || class == TRAINER_CLASS_ELITE_FOUR
        || class == TRAINER_CLASS_CHAMPION || class == TRAINER_CLASS_RIVAL;
}

void CtrRandom_Party(struct Pokemon *party, uint8_t count, uint16_t trainerNum)
{
    int mode = Mode();
    u8 i, stat;

    if (mode < RANDOM_TRAINERS || (mode < RANDOM_BOSSES && IsBoss(trainerNum)))
        return;
    for (i = 0; i < count; i++)
    {
        u16 species = GetMonData(&party[i], MON_DATA_SPECIES);
        u8 level = GetMonData(&party[i], MON_DATA_LEVEL);
        u16 item = GetMonData(&party[i], MON_DATA_HELD_ITEM);
        u8 ivs[NUM_STATS];
        u32 id = ((u32)trainerNum << 3) | i;

        if (species == SPECIES_NONE)
            continue;
        for (stat = 0; stat < NUM_STATS; stat++)
            ivs[stat] = GetMonData(&party[i], MON_DATA_HP_IV + stat);
        /* A new Pokémon of the same level, IVs and item; its moves are the
         * ones its species learns by then (random ones from mode 4). */
        CreateMon(&party[i], RandomSpecies(id, species), level, 0, TRUE, Hash(id, species, 4), OT_ID_RANDOM_NO_SHINY, 0);
        for (stat = 0; stat < NUM_STATS; stat++)
            SetMonData(&party[i], MON_DATA_HP_IV + stat, &ivs[stat]);
        SetMonData(&party[i], MON_DATA_HELD_ITEM, &item);
        CalculateMonStats(&party[i]);
    }
}

uint16_t CtrRandom_Ability(uint16_t species, uint8_t slot, uint16_t ability)
{
    if (Mode() < RANDOM_ABILITIES_AND_MOVES || ability == ABILITY_NONE)
        return ability;
    return 1 + Hash(species, slot, 2) % (ABILITIES_COUNT - 1);
}

uint16_t CtrRandom_Move(uint16_t species, uint16_t move)
{
    u16 random;

    if (Mode() < RANDOM_ABILITIES_AND_MOVES || move == MOVE_NONE)
        return move;
    random = 1 + (Hash(species, move, 3) >> 8) % (MOVES_COUNT - 1);
    return random == MOVE_STRUGGLE ? MOVE_TACKLE : random;
}
