/*
 * The Nuzlocke rules (include/3ds_nuzlocke.h). A game translation unit: it
 * reads the party, the map and the Pokédex.
 */
#include <string.h>
#include "global.h"
#include "overworld.h"
#include "pokedex.h"
#include "pokemon.h"
#include "random.h"
#include "constants/species.h"
#include "3ds_extras.h"
#include "3ds_nuz_store.h"
#include "3ds_nuzlocke.h"

#define NUZ_AREAS 256
#define NUZ_DEAD_MAX 160

/* What the sidecar holds. A Pokémon is told by its personality and OT ID. */
struct NuzState
{
    u8 failed;
    u8 ivSpent; /* IV boosts used (3ds_ivboost.h) */
    u16 deadCount;
    u32 seed; /* the randomizer's, chosen when the run begins */
    u8 used[NUZ_AREAS / 8];
    u32 dead[NUZ_DEAD_MAX][2];
};

static struct NuzState sState;
/* The wild Pokémon of this battle may not be caught (its area is spent). */
static bool8 sCatchBlocked;

static bool8 Active(void)
{
    return CtrExtras_Get("diff_nuzlocke") != 0;
}

static bool8 AreaUsed(u8 area)
{
    return (sState.used[area / 8] >> (area % 8)) & 1;
}

void CtrNuzlocke_OnWildBattle(void)
{
    struct Pokemon *wild = &gEnemyParty[0];
    u16 species = GetMonData(wild, MON_DATA_SPECIES);
    u8 area = GetCurrentRegionMapSectionId();

    sCatchBlocked = FALSE;
    if (!Active() || sState.failed || species == SPECIES_NONE)
        return;
    /* The shiny and the dupes clauses: these never use the area up. */
    if (IsMonShiny(wild) || GetSetPokedexFlag(SpeciesToNationalPokedexNum(species), FLAG_GET_CAUGHT))
        return;
    if (AreaUsed(area))
        sCatchBlocked = TRUE;
    else
        sState.used[area / 8] |= 1 << (area % 8);
}

bool CtrNuzlocke_CanCatch(void)
{
    return !(Active() && sCatchBlocked);
}

bool CtrNuzlocke_IsDead(const struct Pokemon *mon)
{
    u32 personality, otId;
    u16 i;

    if (!Active() || GetMonData((struct Pokemon *)mon, MON_DATA_SPECIES, NULL) == SPECIES_NONE)
        return FALSE;
    personality = GetMonData((struct Pokemon *)mon, MON_DATA_PERSONALITY, NULL);
    otId = GetMonData((struct Pokemon *)mon, MON_DATA_OT_ID, NULL);
    for (i = 0; i < sState.deadCount; i++)
        if (sState.dead[i][0] == personality && sState.dead[i][1] == otId)
            return TRUE;
    return FALSE;
}

void CtrNuzlocke_ScanParty(void)
{
    u8 i;

    if (!Active())
        return;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gPlayerParty[i];
        u16 species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);

        if (species == SPECIES_NONE || species == SPECIES_EGG || GetMonData(mon, MON_DATA_HP) != 0)
            continue;
        if (!CtrNuzlocke_IsDead(mon) && sState.deadCount < NUZ_DEAD_MAX)
        {
            sState.dead[sState.deadCount][0] = GetMonData(mon, MON_DATA_PERSONALITY);
            sState.dead[sState.deadCount][1] = GetMonData(mon, MON_DATA_OT_ID);
            sState.deadCount++;
        }
    }
}

void CtrNuzlocke_OnBattleEnd(bool defeated)
{
    sCatchBlocked = FALSE;
    if (!Active())
        return;
    CtrNuzlocke_ScanParty();
    if (defeated)
        sState.failed = TRUE;
}

void CtrNuzlocke_Reset(void)
{
    memset(&sState, 0, sizeof(sState));
    sState.seed = ((u32)Random() << 16) | Random();
    sCatchBlocked = FALSE;
}

void CtrNuzlocke_Load(const void *save, size_t size)
{
    memset(&sState, 0, sizeof(sState));
    CtrNuzStore_Load(save, size, &sState, sizeof(sState));
    if (sState.deadCount > NUZ_DEAD_MAX)
        memset(&sState, 0, sizeof(sState));
}

void CtrNuzlocke_Save(const void *save, size_t size)
{
    CtrNuzlocke_ScanParty();
    CtrNuzStore_Save(save, size, &sState, sizeof(sState));
}

unsigned CtrNuzlocke_UsedAreas(void)
{
    unsigned count = 0, area;

    for (area = 0; area < NUZ_AREAS; area++)
        count += AreaUsed(area);
    return count;
}

unsigned CtrNuzlocke_DeadCount(void)
{
    return sState.deadCount;
}

bool CtrNuzlocke_Failed(void)
{
    return sState.failed;
}

uint32_t CtrNuzlocke_Seed(void)
{
    /* A save with none yet (it began before the run kept one) has a fixed seed. */
    return sState.seed != 0 ? sState.seed : 0x0E3D5EEDu;
}

unsigned CtrNuzlocke_IvSpent(void)
{
    return sState.ivSpent;
}

void CtrNuzlocke_SpendIv(void)
{
    if (sState.ivSpent < 255)
        sState.ivSpent++;
}
