#ifndef CTR_TRAINER_TEAMS_H
#define CTR_TRAINER_TEAMS_H

/*
 * The redesigned teams of the leaders and the Elite Four (diff_teams,
 * docs/DIFFICULTY_PLAN.md): the table is src/3ds_trainer_teams.c. A trainer
 * without an entry, or a setting that does not reach it, keeps the game's team.
 */
#include <stdint.h>

struct Pokemon;

/* The party a trainer battle just built (battle_main.c, CreateNPCTrainerParty):
 * replaced by the redesigned one where there is one. Returns the new count. */
uint8_t CtrTeams_Apply(struct Pokemon *party, uint8_t count, uint16_t trainerNum);
/* The highest level of the trainer's redesigned team, or 0 when the setting
 * leaves it its own (the level cap reads it). */
uint8_t CtrTeams_TopLevel(uint16_t trainerNum);

#endif // CTR_TRAINER_TEAMS_H
