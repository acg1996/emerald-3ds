#ifndef CTR_NUZLOCKE_H
#define CTR_NUZLOCKE_H

/*
 * The Nuzlocke rules (diff_nuzlocke, docs/DIFFICULTY_PLAN.md): one catch per
 * area, a fainted Pokémon is dead for good, and being defeated ends the run.
 * The Softlocke (diff_nuzlocke = 2) is the same, but Revives bring the dead back.
 * The run's state is kept next to the save (3ds_nuz_store.h).
 *
 * Game translation units call these from the few hooks patches/difficulty/
 * adds; every one does nothing while the setting is off.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct Pokemon;

/* A random wild battle has begun (battle_setup.c): the area's first one is the
 * only one that may be caught, unless the Pokémon is shiny or already owned. */
void CtrNuzlocke_OnWildBattle(void);
/* Whether a Poké Ball may be thrown at the wild Pokémon (handleballthrow). */
bool CtrNuzlocke_CanCatch(void);
/* A battle ended: the fainted are marked dead, and being defeated fails the run. */
void CtrNuzlocke_OnBattleEnd(bool defeated);
/* Marks every fainted Pokémon of the party dead. */
void CtrNuzlocke_ScanParty(void);
/* A dead Pokémon: it is not healed by the Centers. In the Softlocke (setting 2)
 * a revived one is alive again. */
bool CtrNuzlocke_IsDead(const struct Pokemon *mon);
/* Whether items are refused to the Pokémon: it is dead, and the setting is the
 * Nuzlocke (setting 1); the Softlocke lets a Revive work. */
bool CtrNuzlocke_BlocksItem(const struct Pokemon *mon);

/* A new game starts a new run (new_game.c). */
void CtrNuzlocke_Reset(void);
/* The run's state with the save (save.c). */
void CtrNuzlocke_Load(const void *save, size_t size);
void CtrNuzlocke_Save(const void *save, size_t size);

/* For the OPTIONS page: areas used, the dead, and whether the run failed. */
unsigned CtrNuzlocke_UsedAreas(void);
unsigned CtrNuzlocke_DeadCount(void);
bool CtrNuzlocke_Failed(void);
/* The run's seed, for the randomizer (3ds_random.h). */
uint32_t CtrNuzlocke_Seed(void);
/* The IV boosts used so far, and one more used (3ds_ivboost.h). */
unsigned CtrNuzlocke_IvSpent(void);
void CtrNuzlocke_SpendIv(void);

#endif // CTR_NUZLOCKE_H
