#ifndef CTR_RANDOM_H
#define CTR_RANDOM_H

/*
 * The randomizer (diff_random, docs/DIFFICULTY_PLAN.md). Everything is told
 * by a hash of the run's seed (3ds_nuzlocke.h keeps it with the save), so
 * nothing but the seed is stored and the same save sees the same game.
 *
 *   1  wild Pokémon          3  + leaders, Elite Four, Champion, rival,
 *   2  + trainers' Pokémon        starters, gifts and static encounters
 *                            4  + abilities and level-up moves
 *
 * Game translation units call these from the hooks patches/difficulty/ adds;
 * each returns what it is given while its mode is not on.
 */
#include <stdint.h>

struct Pokemon;

/* The species of a random wild Pokémon, by the map it is met on. */
uint16_t CtrRandom_Wild(uint16_t species);
/* The party a trainer battle just built: its species replaced. */
void CtrRandom_Party(struct Pokemon *party, uint8_t count, uint16_t trainerNum);
/* A starter, and a Pokémon given or met by a script. */
uint16_t CtrRandom_Starter(uint16_t species);
uint16_t CtrRandom_Gift(uint16_t species);
/* A species' ability in this slot, and a move it learns by level. */
uint16_t CtrRandom_Ability(uint16_t species, uint8_t slot, uint16_t ability);
uint16_t CtrRandom_Move(uint16_t species, uint16_t move);
/* Wally's Ralts is built with the moves it always had: set while it is made. */
void CtrRandom_Tutorial(int on);

#endif // CTR_RANDOM_H
