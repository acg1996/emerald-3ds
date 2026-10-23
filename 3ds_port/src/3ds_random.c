/*
 * The randomizer (include/3ds_random.h). A game translation unit: it reads the
 * map, the trainer tables and the party.
 */
#include "global.h"
#include "data.h"
#include "pokemon.h"
#include "constants/abilities.h"
#include "constants/items.h"
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

static bool8 IsWally(u16 trainerNum)
{
    return trainerNum == TRAINER_WALLY_MAUVILLE || trainerNum == TRAINER_WALLY_VR_1 || trainerNum == TRAINER_WALLY_VR_2
        || trainerNum == TRAINER_WALLY_VR_3 || trainerNum == TRAINER_WALLY_VR_4 || trainerNum == TRAINER_WALLY_VR_5;
}

/* The battles of the story: leaders, Elite Four, Champion, the rival, Wally
 * and the bosses and admins of Team Aqua and Team Magma. */
static bool8 IsBoss(u16 trainerNum)
{
    u8 class;

    if (trainerNum >= TRAINERS_COUNT)
        return FALSE;
    class = gTrainers[trainerNum].trainerClass;
    return class == TRAINER_CLASS_LEADER || class == TRAINER_CLASS_ELITE_FOUR
        || class == TRAINER_CLASS_CHAMPION || class == TRAINER_CLASS_RIVAL
        || class == TRAINER_CLASS_AQUA_LEADER || class == TRAINER_CLASS_AQUA_ADMIN
        || class == TRAINER_CLASS_MAGMA_LEADER || class == TRAINER_CLASS_MAGMA_ADMIN;
}

/* Wally and the rival keep their last Pokémon, the main one, the same species
 * in every battle: it is told by the run, not by the battle. */
#define ACE_WALLY 0x3A11
#define ACE_RIVAL 0x3B22

static u32 AceKey(u16 trainerNum)
{
    if (IsWally(trainerNum))
        return ACE_WALLY;
    if (trainerNum < TRAINERS_COUNT && gTrainers[trainerNum].trainerClass == TRAINER_CLASS_RIVAL)
        return ACE_RIVAL;
    return 0;
}

/* What a story battle's Pokémon hold: useful ones, by the side they hit from. */
static const u16 sPhysicalItems[] =
{
    ITEM_CHOICE_BAND, ITEM_LIECHI_BERRY, ITEM_SCOPE_LENS, ITEM_LEFTOVERS, ITEM_LUM_BERRY,
    ITEM_SITRUS_BERRY, ITEM_SHELL_BELL, ITEM_FOCUS_BAND, ITEM_SALAC_BERRY,
};
static const u16 sSpecialItems[] =
{
    ITEM_PETAYA_BERRY, ITEM_SCOPE_LENS, ITEM_LEFTOVERS, ITEM_LUM_BERRY,
    ITEM_SITRUS_BERRY, ITEM_SHELL_BELL, ITEM_FOCUS_BAND, ITEM_SALAC_BERRY,
};

/* The same personality every time, of the wanted nature. */
static u32 PersonalityOfNature(u32 seed, u8 nature)
{
    u32 personality = seed;

    do
        personality = personality * 1664525u + 1013904223u;
    while (GetNatureFromPersonality(personality) != nature);
    return personality;
}

void CtrRandom_Party(struct Pokemon *party, uint8_t count, uint16_t trainerNum)
{
    int mode = Mode();
    bool8 boss = IsBoss(trainerNum);
    u32 ace = AceKey(trainerNum);
    u8 i, stat;

    if (mode < RANDOM_TRAINERS || (mode < RANDOM_BOSSES && boss))
        return;
    for (i = 0; i < count; i++)
    {
        u16 species = GetMonData(&party[i], MON_DATA_SPECIES);
        u8 level = GetMonData(&party[i], MON_DATA_LEVEL);
        u16 item = GetMonData(&party[i], MON_DATA_HELD_ITEM);
        u8 ivs[NUM_STATS], evs[NUM_STATS], strongest = 0;
        u32 id = ((u32)trainerNum << 3) | i;
        u32 personality = Hash(id, species, 4);
        u16 newSpecies = (ace != 0 && i == count - 1) ? RandomSpecies(ace, 0) : RandomSpecies(id, species);

        if (species == SPECIES_NONE)
            continue;
        for (stat = 0; stat < NUM_STATS; stat++)
        {
            ivs[stat] = GetMonData(&party[i], MON_DATA_HP_IV + stat);
            evs[stat] = GetMonData(&party[i], MON_DATA_HP_EV + stat);
            if (evs[stat] > strongest)
                strongest = evs[stat];
        }
        if (boss)
        {
            /* A story battle: a nature, EVs and an item that suit the new
             * species. The EVs are as many as the redesigned team had. */
            const struct SpeciesInfo *base = &gSpeciesInfo[newSpecies];
            bool8 physical = base->baseAttack >= base->baseSpAttack, fast = base->baseSpeed >= 80;
            u8 nature = physical ? (fast ? NATURE_JOLLY : NATURE_ADAMANT) : (fast ? NATURE_TIMID : NATURE_MODEST);

            personality = PersonalityOfNature(personality, nature);
            for (stat = 0; stat < NUM_STATS; stat++)
                evs[stat] = 0;
            evs[physical ? 1 : 4] = strongest;
            evs[base->baseSpeed >= 60 ? 3 : 0] = strongest;
            if (physical)
                item = sPhysicalItems[Hash(id, newSpecies, 5) % ARRAY_COUNT(sPhysicalItems)];
            else
                item = sSpecialItems[Hash(id, newSpecies, 5) % ARRAY_COUNT(sSpecialItems)];
        }
        /* A new Pokémon of the same level and IVs; its moves are the ones its
         * species learns by then (random ones from mode 4). */
        CreateMon(&party[i], newSpecies, level, 0, TRUE, personality, OT_ID_RANDOM_NO_SHINY, 0);
        for (stat = 0; stat < NUM_STATS; stat++)
        {
            SetMonData(&party[i], MON_DATA_HP_IV + stat, &ivs[stat]);
            if (boss)
                SetMonData(&party[i], MON_DATA_HP_EV + stat, &evs[stat]);
        }
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
