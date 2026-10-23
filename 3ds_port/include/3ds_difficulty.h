#ifndef CTR_DIFFICULTY_H
#define CTR_DIFFICULTY_H

/*
 * The DIFFICULTY page's settings (include/3ds_extras.h) applied to the game.
 * Game translation units call these from the few hooks patches/difficulty/
 * adds to the decompilation; every one returns at once while its setting is
 * on the original game.
 */
#include <stdint.h>

struct Pokemon;

/* The party a trainer battle just built (battle_main.c, CreateNPCTrainerParty):
 * its IVs by diff_iv_scope and diff_iv_level. */
void CtrDifficulty_ApplyTrainerIvs(struct Pokemon *party, uint8_t count, uint16_t trainerNum);

#endif // CTR_DIFFICULTY_H
