#ifndef CTR_DIFFICULTY_H
#define CTR_DIFFICULTY_H

/*
 * The DIFFICULTY page's settings (include/3ds_extras.h) applied to the game.
 * Game translation units call these from the few hooks patches/difficulty/
 * adds to the decompilation; every one returns at once while its setting is
 * on the original game.
 */
#include <stdbool.h>
#include <stdint.h>

struct Pokemon;

/* The party a trainer battle just built (battle_main.c, CreateNPCTrainerParty):
 * its IVs by diff_iv_scope and diff_iv_level. */
void CtrDifficulty_ApplyTrainerIvs(struct Pokemon *party, uint8_t count, uint16_t trainerNum);

/*
 * The level cap (diff_level_cap): the level of the strongest Pokémon of the
 * next leader whose badge is missing and, with the Elite Four setting, of the
 * next of them still standing. 0 when there is no cap.
 */
uint8_t CtrDifficulty_LevelCap(void);
/* Whether this Pokémon is at the cap: it gains no experience and levels no more. */
bool CtrDifficulty_AtLevelCap(const struct Pokemon *mon);
/* The experience it may gain: cut so that it ends no higher than the cap. */
int32_t CtrDifficulty_ClampExp(const struct Pokemon *mon, int32_t exp);

/*
 * The trainer AI (diff_ai). Smart means this battler is a trainer's, in a battle
 * the setting reaches: leaders and the Elite Four, or every trainer.
 */
bool CtrAi_IsSmart(uint8_t battler);
/* The AI scripts a smart trainer always runs, added to its own. */
uint32_t CtrAi_BaseFlags(void);
/*
 * After the AI scripts have scored the moves against one target, in a single
 * or a double battle: more for a move that knocks the target out or hits hard,
 * for a stat-raising move only when the battler is not about to be knocked out,
 * and less for a move that would hurt its own partner.
 */
void CtrAi_AdjustScores(uint8_t battler, uint8_t target, int8_t *scores);
/*
 * Switching, for a smart trainer. The party slot to send in for this battler (the
 * one that fares best against the foes, the ace held back until two are left), or
 * 6 for none; and whether the battler should switch out now: it cannot hurt the
 * foes, they are about to hurt it, and a Pokemon in the party is clearly better.
 */
uint8_t CtrAi_BestSwitchIn(uint8_t battler);
bool CtrAi_ShouldSwitchOut(uint8_t battler);
/*
 * Whether a smart trainer should spend its healing item on this battler now: once per
 * Pokemon, when it is low, the item gives back a good part, and the foes would not
 * undo it. Always true for the other trainers.
 */
bool CtrAi_ShouldHeal(uint8_t battler, int amount);

#endif // CTR_DIFFICULTY_H
