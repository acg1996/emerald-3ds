/*
 * The IV boost (include/3ds_ivboost.h). A game translation unit: it reads the
 * trainer flags and the party.
 */
#include "global.h"
#include "battle_setup.h"
#include "data.h"
#include "pokemon.h"
#include "characters.h"
#include "constants/opponents.h"
#include "constants/species.h"
#include "3ds_extras.h"
#include "3ds_ivboost.h"
#include "3ds_nuzlocke.h"

#define MAX_IV 31

static const u16 sLeaders[] =
{
    TRAINER_ROXANNE_1, TRAINER_BRAWLY_1, TRAINER_WATTSON_1, TRAINER_FLANNERY_1,
    TRAINER_NORMAN_1, TRAINER_WINONA_1, TRAINER_TATE_AND_LIZA_1, TRAINER_JUAN_1,
    /* Wally, in Mauville and in Victory Road. */
    TRAINER_WALLY_MAUVILLE, TRAINER_WALLY_VR_1,
};

/* One battle of the rival: Brendan or May, with whichever starter was chosen. */
#define RIVAL_BATTLE(place) \
    {TRAINER_BRENDAN_##place##_TREECKO, TRAINER_BRENDAN_##place##_MUDKIP, TRAINER_BRENDAN_##place##_TORCHIC, \
     TRAINER_MAY_##place##_TREECKO, TRAINER_MAY_##place##_MUDKIP, TRAINER_MAY_##place##_TORCHIC}

static const u16 sRivalBattles[][6] =
{
    RIVAL_BATTLE(ROUTE_103),
    RIVAL_BATTLE(RUSTBORO),
    RIVAL_BATTLE(ROUTE_110),
    RIVAL_BATTLE(ROUTE_119),
    RIVAL_BATTLE(LILYCOVE),
};

unsigned CtrIvBoost_Total(void)
{
    return ARRAY_COUNT(sLeaders) + ARRAY_COUNT(sRivalBattles);
}

unsigned CtrIvBoost_Earned(void)
{
    unsigned earned = 0;
    u8 i, j;

    for (i = 0; i < ARRAY_COUNT(sLeaders); i++)
        if (HasTrainerBeenFought(sLeaders[i]))
            earned++;
    for (i = 0; i < ARRAY_COUNT(sRivalBattles); i++)
    {
        for (j = 0; j < 6; j++)
        {
            if (HasTrainerBeenFought(sRivalBattles[i][j]))
            {
                earned++;
                break;
            }
        }
    }
    return earned;
}

unsigned CtrIvBoost_Available(void)
{
    unsigned earned = CtrIvBoost_Earned(), spent = CtrNuzlocke_IvSpent();

    return earned > spent ? earned - spent : 0;
}

static bool8 HasMon(u8 slot)
{
    u16 species;

    if (slot >= PARTY_SIZE)
        return FALSE;
    species = GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES_OR_EGG);
    return species != SPECIES_NONE && species != SPECIES_EGG;
}

int CtrIvBoost_Current(uint8_t slot, uint8_t stat)
{
    if (!HasMon(slot) || stat >= NUM_STATS)
        return -1;
    return GetMonData(&gPlayerParty[slot], MON_DATA_HP_IV + stat);
}

bool CtrIvBoost_Apply(uint8_t slot, uint8_t stat)
{
    u8 iv = MAX_IV;

    if (CtrIvBoost_Available() == 0 || CtrIvBoost_Current(slot, stat) < 0 || CtrIvBoost_Current(slot, stat) >= MAX_IV)
        return false;
    SetMonData(&gPlayerParty[slot], MON_DATA_HP_IV + stat, &iv);
    CalculateMonStats(&gPlayerParty[slot]);
    CtrNuzlocke_SpendIv();
    return true;
}

const uint8_t *CtrIvBoost_Name(uint8_t slot)
{
    static const u8 none[] = {CHAR_HYPHEN, EOS};

    return HasMon(slot) ? gSpeciesNames[GetMonData(&gPlayerParty[slot], MON_DATA_SPECIES)] : none;
}

uint8_t CtrIvBoost_NextSlot(uint8_t slot, int direction)
{
    u8 tries, next = slot;

    for (tries = 0; tries < PARTY_SIZE; tries++)
    {
        next = direction < 0 ? (next == 0 ? PARTY_SIZE - 1 : next - 1) : (next + 1 >= PARTY_SIZE ? 0 : next + 1);
        if (HasMon(next))
            return next;
    }
    return slot;
}
