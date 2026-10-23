#ifndef CTR_IVBOOST_H
#define CTR_IVBOOST_H

/*
 * The IV boost (diff_iv_boost): winning against a gym leader, and against the
 * rival in each of his five battles, earns a boost that sets one IV of one
 * of the player's Pokémon to 31. What was earned is told by the trainer
 * flags the game already keeps, so no battle needs a hook; only what was
 * spent is kept, with the run's state (3ds_nuzlocke.h).
 */
#include <stdbool.h>
#include <stdint.h>

/* Boosts earned so far: the leaders beaten and the rival's battles won. */
unsigned CtrIvBoost_Earned(void);
/* Earned and not spent yet. */
unsigned CtrIvBoost_Available(void);
/* The most there are to earn. */
unsigned CtrIvBoost_Total(void);
/* The IV (0 to 5: HP, Attack, Defense, Speed, Sp. Atk, Sp. Def) of the party
 * Pokémon in this slot; -1 when the slot holds none. */
int CtrIvBoost_Current(uint8_t slot, uint8_t stat);
/* Sets that IV to 31 and spends a boost; false (and nothing spent) when there
 * is none to spend, the slot holds no Pokémon or the IV already is 31. */
bool CtrIvBoost_Apply(uint8_t slot, uint8_t stat);
/* The party slot's species name in the game's characters, for the OPTIONS page. */
const uint8_t *CtrIvBoost_Name(uint8_t slot);
/* The party slot after this one with a Pokémon (an egg does not count). */
uint8_t CtrIvBoost_NextSlot(uint8_t slot, int direction);

#endif // CTR_IVBOOST_H
