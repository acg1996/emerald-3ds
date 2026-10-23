#!/usr/bin/env python3
"""Writes 3ds_port/src/3ds_trainer_teams_data.h: the redesigned teams of
docs/DIFFICULTY_PLAN.md (gym leaders, their rematches, the Elite Four, the
Champion and the rival), from the sets below.

    python tools/gen_trainer_teams.py [--tree build/upstream]

With --tree every species, move and item name is checked against the
decompilation's constants, so a typo fails here and not in the compiler.
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EV = {"hp": 0, "atk": 1, "def": 2, "spe": 3, "spa": 4, "spd": 5}

# key: (species, ability slot, nature, item, moves, two EV stats)
T = {}


def t(key, species, slot, nature, item, moves, evs):
    T[key] = (species, slot, nature, item, moves, evs)


# --- Gym leaders: first battle -------------------------------------------
t("GEODUDE", "GEODUDE", 0, "IMPISH", "SITRUS_BERRY", ["ROLLOUT", "DEFENSE_CURL", "ROCK_TOMB", "ROCK_THROW"], ("atk", "def"))
t("ARON", "ARON", 0, "ADAMANT", "FOCUS_BAND", ["METAL_CLAW", "ROCK_TOMB", "HARDEN", "HEADBUTT"], ("atk", "def"))
t("ANORITH", "ANORITH", 0, "ADAMANT", "QUICK_CLAW", ["METAL_CLAW", "ROCK_TOMB", "FURY_CUTTER", "HARDEN"], ("atk", "spe"))
t("NOSEPASS", "NOSEPASS", 0, "RELAXED", "ORAN_BERRY", ["BLOCK", "ROCK_TOMB", "THUNDER_WAVE", "HARDEN"], ("hp", "def"))
t("MACHOP", "MACHOP", 0, "ADAMANT", "SCOPE_LENS", ["KARATE_CHOP", "SEISMIC_TOSS", "BULK_UP", "ROCK_TOMB"], ("atk", "spe"))
t("MEDITITE", "MEDITITE", 0, "JOLLY", "LUM_BERRY", ["HI_JUMP_KICK", "BULK_UP", "CONFUSION", "DETECT"], ("atk", "spe"))
t("HITMONTOP", "HITMONTOP", 0, "JOLLY", "BLACK_BELT", ["ROLLING_KICK", "REVENGE", "PURSUIT", "QUICK_ATTACK"], ("atk", "spe"))
t("MAKUHITA", "MAKUHITA", 1, "ADAMANT", "SALAC_BERRY", ["REVERSAL", "BULK_UP", "VITAL_THROW", "ARM_THRUST"], ("hp", "atk"))
t("VOLTORB", "VOLTORB", 1, "TIMID", "SITRUS_BERRY", ["SPARK", "THUNDER_WAVE", "SCREECH", "SELF_DESTRUCT"], ("spe", "spa"))
t("PLUSLE", "PLUSLE", 0, "TIMID", "ORAN_BERRY", ["SPARK", "THUNDER_WAVE", "QUICK_ATTACK", "ENCORE"], ("spe", "spa"))
t("MAGNETON", "MAGNETON", 0, "MODEST", "MAGNET", ["THUNDERBOLT", "THUNDER_WAVE", "SUPERSONIC", "SPARK"], ("hp", "spa"))
t("ELECTABUZZ", "ELECTABUZZ", 0, "HASTY", "LEFTOVERS", ["THUNDER_PUNCH", "QUICK_ATTACK", "LIGHT_SCREEN", "SWIFT"], ("spe", "spa"))
t("MANECTRIC", "MANECTRIC", 1, "JOLLY", "LUM_BERRY", ["THUNDERBOLT", "QUICK_ATTACK", "THUNDER_WAVE", "BITE"], ("spe", "spa"))
t("SLUGMA", "SLUGMA", 1, "MODEST", "SITRUS_BERRY", ["SUNNY_DAY", "OVERHEAT", "ROCK_THROW", "YAWN"], ("hp", "spa"))
t("NUMEL", "NUMEL", 0, "MODEST", "ORAN_BERRY", ["OVERHEAT", "MAGNITUDE", "TAKE_DOWN", "AMNESIA"], ("hp", "spa"))
t("MAGMAR", "MAGMAR", 0, "HASTY", "SHELL_BELL", ["FIRE_PUNCH", "THUNDER_PUNCH", "SMOG", "SMOKESCREEN"], ("spe", "spa"))
t("CAMERUPT", "CAMERUPT", 0, "MODEST", "LEFTOVERS", ["OVERHEAT", "EARTHQUAKE", "ROCK_SLIDE", "AMNESIA"], ("hp", "spa"))
t("TORKOAL", "TORKOAL", 0, "MODEST", "WHITE_HERB", ["OVERHEAT", "FLAMETHROWER", "BODY_SLAM", "PROTECT"], ("hp", "spa"))
t("SPINDA", "SPINDA", 0, "JOLLY", "SILK_SCARF", ["FACADE", "PSYBEAM", "DIZZY_PUNCH", "ENCORE"], ("hp", "atk"))
t("VIGOROTH", "VIGOROTH", 0, "ADAMANT", "QUICK_CLAW", ["FACADE", "SLASH", "BRICK_BREAK", "BULK_UP"], ("atk", "spe"))
t("KECLEON", "KECLEON", 0, "ADAMANT", "KINGS_ROCK", ["THIEF", "PSYBEAM", "FAINT_ATTACK", "SCREECH"], ("hp", "atk"))
t("LINOONE", "LINOONE", 0, "ADAMANT", "SALAC_BERRY", ["BELLY_DRUM", "SLASH", "FACADE", "HEADBUTT"], ("hp", "atk"))
t("SLAKING", "SLAKING", 0, "ADAMANT", "LEFTOVERS", ["FACADE", "SLACK_OFF", "COUNTER", "FAINT_ATTACK"], ("hp", "atk"))
t("SWABLU", "SWABLU", 0, "CALM", "SITRUS_BERRY", ["AERIAL_ACE", "SING", "SAFEGUARD", "PERISH_SONG"], ("hp", "spd"))
t("TROPIUS", "TROPIUS", 0, "MODEST", "MIRACLE_SEED", ["SUNNY_DAY", "SOLAR_BEAM", "AERIAL_ACE", "SYNTHESIS"], ("hp", "spa"))
t("PELIPPER", "PELIPPER", 0, "BOLD", "MYSTIC_WATER", ["SURF", "AERIAL_ACE", "PROTECT", "SUPERSONIC"], ("hp", "def"))
t("SKARMORY", "SKARMORY", 0, "IMPISH", "SHARP_BEAK", ["AERIAL_ACE", "STEEL_WING", "AGILITY", "TOXIC"], ("hp", "def"))
t("ALTARIA", "ALTARIA", 0, "ADAMANT", "LUM_BERRY", ["DRAGON_DANCE", "EARTHQUAKE", "DRAGON_CLAW", "AERIAL_ACE"], ("hp", "atk"))
t("CLAYDOL", "CLAYDOL", 0, "IMPISH", "LEFTOVERS", ["EARTHQUAKE", "ANCIENT_POWER", "PSYCHIC", "RAPID_SPIN"], ("hp", "def"))
t("XATU", "XATU", 0, "TIMID", "TWISTED_SPOON", ["PSYCHIC", "CALM_MIND", "WISH", "CONFUSE_RAY"], ("spe", "spa"))
t("LUNATONE", "LUNATONE", 0, "MODEST", "SITRUS_BERRY", ["PSYCHIC", "CALM_MIND", "HYPNOSIS", "SHADOW_BALL"], ("hp", "spa"))
t("SOLROCK", "SOLROCK", 0, "ADAMANT", "MIRACLE_SEED", ["SUNNY_DAY", "SOLAR_BEAM", "ROCK_SLIDE", "PSYCHIC"], ("hp", "spa"))
t("GARDEVOIR", "GARDEVOIR", 1, "MODEST", "LUM_BERRY", ["CALM_MIND", "PSYCHIC", "SHADOW_BALL", "THUNDERBOLT"], ("spe", "spa"))
t("WHISCASH", "WHISCASH", 0, "CALM", "SITRUS_BERRY", ["RAIN_DANCE", "SURF", "EARTHQUAKE", "AMNESIA"], ("hp", "spd"))
t("LUVDISC", "LUVDISC", 0, "TIMID", "SALAC_BERRY", ["SURF", "FLAIL", "AGILITY", "ATTRACT"], ("spe", "spa"))
t("SEALEO", "SEALEO", 0, "CALM", "CHESTO_BERRY", ["ICE_BEAM", "SURF", "REST", "BODY_SLAM"], ("hp", "spd"))
t("CRAWDAUNT", "CRAWDAUNT", 0, "ADAMANT", "LIECHI_BERRY", ["SWORDS_DANCE", "CRABHAMMER", "KNOCK_OFF", "TAUNT"], ("atk", "spe"))
t("KINGDRA", "KINGDRA", 0, "MODEST", "MYSTIC_WATER", ["HYDRO_PUMP", "ICE_BEAM", "DRAGON_BREATH", "AGILITY"], ("spe", "spa"))

# --- Elite Four and Champion ---------------------------------------------
t("MIGHTYENA", "MIGHTYENA", 0, "ADAMANT", "BLACK_GLASSES", ["CRUNCH", "DOUBLE_EDGE", "TAUNT", "ROAR"], ("hp", "atk"))
t("SHIFTRY", "SHIFTRY", 0, "JOLLY", "LUM_BERRY", ["SWORDS_DANCE", "FAINT_ATTACK", "EXTRASENSORY", "WHIRLWIND"], ("atk", "spe"))
t("CACTURNE", "CACTURNE", 0, "IMPISH", "LEFTOVERS", ["SPIKES", "NEEDLE_ARM", "FAINT_ATTACK", "LEECH_SEED"], ("hp", "def"))
t("SHARPEDO", "SHARPEDO", 0, "HASTY", "MYSTIC_WATER", ["CRUNCH", "SURF", "ICE_BEAM", "TAUNT"], ("spe", "atk"))
t("ABSOL", "ABSOL", 0, "ADAMANT", "SCOPE_LENS", ["SWORDS_DANCE", "SLASH", "AERIAL_ACE", "ROCK_SLIDE"], ("atk", "spe"))
t("SHEDINJA", "SHEDINJA", 0, "ADAMANT", "LUM_BERRY", ["SHADOW_BALL", "CONFUSE_RAY", "PROTECT", "SPITE"], ("atk", "spe"))
t("BANETTE", "BANETTE", 0, "ADAMANT", "SPELL_TAG", ["SHADOW_BALL", "FAINT_ATTACK", "WILL_O_WISP", "KNOCK_OFF"], ("hp", "atk"))
t("GENGAR", "GENGAR", 0, "TIMID", "SHELL_BELL", ["HYPNOSIS", "DREAM_EATER", "SHADOW_BALL", "THUNDERBOLT"], ("spe", "spa"))
t("SABLEYE", "SABLEYE", 0, "CALM", "BRIGHT_POWDER", ["MEAN_LOOK", "SHADOW_BALL", "CONFUSE_RAY", "KNOCK_OFF"], ("hp", "spd"))
t("DUSCLOPS", "DUSCLOPS", 0, "IMPISH", "LEFTOVERS", ["MEAN_LOOK", "WILL_O_WISP", "SHADOW_BALL", "EARTHQUAKE"], ("hp", "def"))
t("GLALIE", "GLALIE", 0, "MODEST", "NEVER_MELT_ICE", ["HAIL", "ICE_BEAM", "CRUNCH", "PROTECT"], ("hp", "spa"))
t("SNEASEL", "SNEASEL", 1, "JOLLY", "SCOPE_LENS", ["SWORDS_DANCE", "SLASH", "FAINT_ATTACK", "QUICK_ATTACK"], ("atk", "spe"))
t("JYNX", "JYNX", 0, "MODEST", "LUM_BERRY", ["ICE_BEAM", "PSYCHIC", "LOVELY_KISS", "CALM_MIND"], ("spe", "spa"))
t("LAPRAS", "LAPRAS", 1, "MODEST", "LEFTOVERS", ["ICE_BEAM", "SURF", "THUNDERBOLT", "CONFUSE_RAY"], ("hp", "spa"))
t("WALREIN", "WALREIN", 0, "CALM", "CHESTO_BERRY", ["ICE_BEAM", "SURF", "BODY_SLAM", "REST"], ("hp", "spd"))
t("FLYGON", "FLYGON", 0, "JOLLY", "SOFT_SAND", ["EARTHQUAKE", "ROCK_SLIDE", "DRAGON_CLAW", "SCREECH"], ("atk", "spe"))
t("GYARADOS", "GYARADOS", 0, "ADAMANT", "LUM_BERRY", ["DRAGON_DANCE", "THRASH", "EARTHQUAKE", "ROCK_SLIDE"], ("atk", "spe"))
t("DRAGONITE", "DRAGONITE", 0, "ADAMANT", "LEFTOVERS", ["THUNDER_WAVE", "AGILITY", "EARTHQUAKE", "AERIAL_ACE"], ("hp", "atk"))
t("LATIOS", "LATIOS", 0, "MODEST", "SOUL_DEW", ["PSYCHIC", "CALM_MIND", "RECOVER", "DRAGON_CLAW"], ("spe", "spa"))
t("TYRANITAR", "TYRANITAR", 0, "ADAMANT", "SHELL_BELL", ["SANDSTORM", "ROCK_SLIDE", "EARTHQUAKE", "THRASH"], ("hp", "atk"))
t("SALAMENCE", "SALAMENCE", 0, "NAIVE", "DRAGON_FANG", ["DRAGON_CLAW", "FLY", "EARTHQUAKE", "FLAMETHROWER"], ("atk", "spe"))
t("WAILORD", "WAILORD", 0, "CALM", "CHESTO_BERRY", ["RAIN_DANCE", "WATER_SPOUT", "ICE_BEAM", "REST"], ("hp", "spa"))
t("TENTACRUEL", "TENTACRUEL", 0, "TIMID", "POISON_BARB", ["HYDRO_PUMP", "SLUDGE_BOMB", "ICE_BEAM", "TOXIC"], ("spe", "spa"))
t("LUDICOLO", "LUDICOLO", 0, "MODEST", "LUM_BERRY", ["SURF", "GIGA_DRAIN", "ICE_BEAM", "LEECH_SEED"], ("hp", "spa"))
t("STARMIE", "STARMIE", 1, "TIMID", "SALAC_BERRY", ["SURF", "PSYCHIC", "THUNDER", "RECOVER"], ("spe", "spa"))
t("KYOGRE", "KYOGRE", 0, "MODEST", "MYSTIC_WATER", ["HYDRO_PUMP", "ICE_BEAM", "THUNDER", "CALM_MIND"], ("hp", "spa"))
t("MILOTIC", "MILOTIC", 0, "BOLD", "LEFTOVERS", ["RECOVER", "SURF", "ICE_BEAM", "TOXIC"], ("hp", "def"))

# --- Rival -----------------------------------------------------------------
t("WINGULL", "WINGULL", 0, "TIMID", "SITRUS_BERRY", ["WATER_GUN", "WING_ATTACK", "SUPERSONIC", "QUICK_ATTACK"], ("spe", "spa"))
t("ELECTRIKE", "ELECTRIKE", 0, "JOLLY", "ORAN_BERRY", ["SPARK", "THUNDER_WAVE", "QUICK_ATTACK", "HOWL"], ("atk", "spe"))
t("LOMBRE", "LOMBRE", 0, "MODEST", "LUM_BERRY", ["FAKE_OUT", "MEGA_DRAIN", "BUBBLE_BEAM", "NATURE_POWER"], ("spa", "spd"))
t("SLUGMA_R", "SLUGMA", 1, "MODEST", "CHARCOAL", ["EMBER", "ROCK_THROW", "SMOG", "YAWN"], ("hp", "spa"))
t("COMBUSKEN110", "COMBUSKEN", 0, "ADAMANT", "CHARCOAL", ["DOUBLE_KICK", "EMBER", "BULK_UP", "PECK"], ("atk", "spe"))
t("GROVYLE110", "GROVYLE", 0, "JOLLY", "MIRACLE_SEED", ["MEGA_DRAIN", "FURY_CUTTER", "QUICK_ATTACK", "PURSUIT"], ("atk", "spe"))
t("MARSHTOMP110", "MARSHTOMP", 0, "MODEST", "MYSTIC_WATER", ["MUD_SHOT", "WATER_GUN", "ROCK_TOMB", "MUD_SLAP"], ("hp", "spa"))
t("TORCHIC5", "TORCHIC", 0, "NAIVE", "ORAN_BERRY", ["SCRATCH", "GROWL", "FOCUS_ENERGY", "NONE"], ("atk", "spe"))
t("TREECKO5", "TREECKO", 0, "JOLLY", "ORAN_BERRY", ["POUND", "LEER", "ABSORB", "QUICK_ATTACK"], ("atk", "spe"))
t("MUDKIP5", "MUDKIP", 0, "ADAMANT", "ORAN_BERRY", ["TACKLE", "GROWL", "MUD_SLAP", "NONE"], ("atk", "hp"))
t("LOTAD", "LOTAD", 0, "MODEST", "SITRUS_BERRY", ["ABSORB", "NATURE_POWER", "ASTONISH", "GROWL"], ("spa", "spd"))
t("RALTS", "RALTS", 0, "TIMID", "TWISTED_SPOON", ["CONFUSION", "CALM_MIND", "DOUBLE_TEAM", "GROWL"], ("spe", "spa"))
t("KIRLIA", "KIRLIA", 0, "TIMID", "TWISTED_SPOON", ["PSYCHIC", "CALM_MIND", "DOUBLE_TEAM", "TELEPORT"], ("spe", "spa"))
t("TORCHIC15", "TORCHIC", 0, "NAIVE", "CHARCOAL", ["EMBER", "PECK", "FOCUS_ENERGY", "SAND_ATTACK"], ("atk", "spe"))
t("TREECKO15", "TREECKO", 0, "JOLLY", "MIRACLE_SEED", ["ABSORB", "QUICK_ATTACK", "PURSUIT", "LEER"], ("atk", "spe"))
t("MUDKIP15", "MUDKIP", 0, "ADAMANT", "MYSTIC_WATER", ["WATER_GUN", "MUD_SLAP", "BIDE", "TACKLE"], ("atk", "hp"))
t("PELIPPER_R", "PELIPPER", 0, "MODEST", "MYSTIC_WATER", ["SURF", "WING_ATTACK", "PROTECT", "SUPERSONIC"], ("hp", "spa"))
t("COMBUSKEN_R", "COMBUSKEN", 0, "ADAMANT", "CHARCOAL", ["FLAMETHROWER", "DOUBLE_KICK", "BULK_UP", "QUICK_ATTACK"], ("atk", "spe"))
t("MANECTRIC_R", "MANECTRIC", 0, "JOLLY", "MAGNET", ["THUNDERBOLT", "QUICK_ATTACK", "THUNDER_WAVE", "BITE"], ("spe", "spa"))
t("TROPIUS_R", "TROPIUS", 0, "MODEST", "MIRACLE_SEED", ["MAGICAL_LEAF", "AERIAL_ACE", "WHIRLWIND", "SWEET_SCENT"], ("hp", "spa"))
t("LUDICOLO_R", "LUDICOLO", 0, "MODEST", "LUM_BERRY", ["RAIN_DANCE", "SURF", "GIGA_DRAIN", "ICE_BEAM"], ("hp", "spa"))
t("GARDEVOIR_R", "GARDEVOIR", 1, "MODEST", "TWISTED_SPOON", ["PSYCHIC", "CALM_MIND", "SHADOW_BALL", "THUNDERBOLT"], ("spe", "spa"))
t("GROVYLE_R", "GROVYLE", 0, "JOLLY", "MIRACLE_SEED", ["MEGA_DRAIN", "FURY_CUTTER", "QUICK_ATTACK", "PURSUIT"], ("atk", "spe"))
t("MARSHTOMP_R", "MARSHTOMP", 0, "MODEST", "MYSTIC_WATER", ["MUD_SHOT", "WATER_GUN", "ROCK_TOMB", "MUD_SLAP"], ("hp", "spa"))

# --- Rematches: new species --------------------------------------------------
t("GOLEM", "GOLEM", 0, "ADAMANT", "LEFTOVERS", ["EARTHQUAKE", "ROCK_SLIDE", "EXPLOSION", "ROLLOUT"], ("hp", "atk"))
t("LAIRON", "LAIRON", 0, "ADAMANT", "LEFTOVERS", ["METAL_CLAW", "ROCK_TOMB", "IRON_DEFENSE", "HEADBUTT"], ("hp", "def"))
t("AGGRON", "AGGRON", 0, "ADAMANT", "LEFTOVERS", ["EARTHQUAKE", "ROCK_SLIDE", "IRON_TAIL", "THUNDERBOLT"], ("atk", "def"))
t("KABUTO", "KABUTO", 0, "ADAMANT", "SITRUS_BERRY", ["ROCK_TOMB", "SURF", "SWORDS_DANCE", "ABSORB"], ("atk", "spe"))
t("KABUTOPS", "KABUTOPS", 0, "ADAMANT", "SITRUS_BERRY", ["SWORDS_DANCE", "ROCK_SLIDE", "SURF", "SLASH"], ("atk", "spe"))
t("OMANYTE", "OMANYTE", 0, "MODEST", "SITRUS_BERRY", ["SURF", "ROCK_TOMB", "ICE_BEAM", "PROTECT"], ("hp", "spa"))
t("OMASTAR", "OMASTAR", 0, "MODEST", "LEFTOVERS", ["SURF", "ROCK_SLIDE", "ICE_BEAM", "PROTECT"], ("hp", "spa"))
t("ARMALDO", "ARMALDO", 0, "ADAMANT", "QUICK_CLAW", ["SWORDS_DANCE", "ROCK_BLAST", "METAL_CLAW", "SLASH"], ("atk", "spe"))
t("AERODACTYL", "AERODACTYL", 0, "JOLLY", "SCOPE_LENS", ["ROCK_SLIDE", "EARTHQUAKE", "AERIAL_ACE", "SUPERSONIC"], ("atk", "spe"))
t("REGIROCK", "REGIROCK", 0, "ADAMANT", "LEFTOVERS", ["EARTHQUAKE", "ROCK_SLIDE", "SUPERPOWER", "THUNDER_WAVE"], ("hp", "atk"))
t("REGISTEEL", "REGISTEEL", 0, "IMPISH", "LEFTOVERS", ["EARTHQUAKE", "ROCK_SLIDE", "TOXIC", "THUNDER_WAVE"], ("hp", "def"))
t("MACHAMP", "MACHAMP", 0, "ADAMANT", "BLACK_BELT", ["CROSS_CHOP", "ROCK_SLIDE", "EARTHQUAKE", "BULK_UP"], ("hp", "atk"))
t("MEDICHAM", "MEDICHAM", 0, "JOLLY", "LUM_BERRY", ["HI_JUMP_KICK", "BULK_UP", "ROCK_TOMB", "PSYCHIC"], ("atk", "spe"))
t("HITMONCHAN", "HITMONCHAN", 0, "ADAMANT", "LEFTOVERS", ["THUNDER_PUNCH", "FIRE_PUNCH", "ICE_PUNCH", "MACH_PUNCH"], ("hp", "atk"))
t("HITMONLEE", "HITMONLEE", 0, "JOLLY", "SCOPE_LENS", ["HI_JUMP_KICK", "ROCK_SLIDE", "EARTHQUAKE", "BRICK_BREAK"], ("atk", "spe"))
t("HARIYAMA", "HARIYAMA", 1, "ADAMANT", "SALAC_BERRY", ["REVERSAL", "BULK_UP", "CROSS_CHOP", "EARTHQUAKE"], ("hp", "atk"))
t("MEW", "MEW", 0, "JOLLY", "LEFTOVERS", ["FOCUS_PUNCH", "BRICK_BREAK", "PSYCHIC", "SWORDS_DANCE"], ("atk", "spe"))
t("MEWTWO", "MEWTWO", 0, "TIMID", "LEFTOVERS", ["PSYCHIC", "CALM_MIND", "RECOVER", "SHADOW_BALL"], ("spe", "spa"))
t("MAREEP", "MAREEP", 0, "TIMID", "ORAN_BERRY", ["THUNDERBOLT", "THUNDER_WAVE", "SIGNAL_BEAM", "COTTON_GUARD"], ("hp", "spa"))
t("FLAAFFY", "FLAAFFY", 0, "TIMID", "SITRUS_BERRY", ["THUNDERBOLT", "THUNDER_WAVE", "SWIFT", "REFLECT"], ("hp", "spa"))
t("AMPHAROS", "AMPHAROS", 0, "MODEST", "MAGNET", ["THUNDERBOLT", "THUNDER_WAVE", "FIRE_PUNCH", "REFLECT"], ("hp", "spa"))
t("ELECTRODE", "ELECTRODE", 1, "TIMID", "SITRUS_BERRY", ["THUNDERBOLT", "THUNDER_WAVE", "EXPLOSION", "SCREECH"], ("spe", "spa"))
t("PIKACHU", "PIKACHU", 0, "TIMID", "LIGHT_BALL", ["THUNDERBOLT", "THUNDER_WAVE", "QUICK_ATTACK", "SURF"], ("spe", "spa"))
t("RAICHU", "RAICHU", 0, "TIMID", "LIGHT_BALL", ["THUNDERBOLT", "THUNDER_WAVE", "SURF", "FOCUS_PUNCH"], ("spe", "spa"))
t("RAIKOU", "RAIKOU", 0, "TIMID", "LEFTOVERS", ["THUNDERBOLT", "CALM_MIND", "SHADOW_BALL", "CRUNCH"], ("spe", "spa"))
t("ZAPDOS", "ZAPDOS", 0, "TIMID", "LEFTOVERS", ["THUNDERBOLT", "DRILL_PECK", "TOXIC", "REST"], ("hp", "spa"))
t("MAGCARGO", "MAGCARGO", 1, "MODEST", "CHARCOAL", ["FLAMETHROWER", "ROCK_SLIDE", "AMNESIA", "YAWN"], ("hp", "spa"))
t("PONYTA", "PONYTA", 1, "JOLLY", "CHARCOAL", ["FIRE_SPIN", "TAKE_DOWN", "STOMP", "AGILITY"], ("atk", "spe"))
t("RAPIDASH", "RAPIDASH", 1, "JOLLY", "CHARCOAL", ["FIRE_BLAST", "MEGAHORN", "STOMP", "AGILITY"], ("atk", "spe"))
t("GROWLITHE", "GROWLITHE", 1, "ADAMANT", "CHARCOAL", ["FLAMETHROWER", "TAKE_DOWN", "ROAR", "CRUNCH"], ("atk", "spe"))
t("ARCANINE", "ARCANINE", 1, "ADAMANT", "CHARCOAL", ["FLAMETHROWER", "EXTREME_SPEED", "EARTHQUAKE", "CRUNCH"], ("atk", "spe"))
t("HOUNDOUR", "HOUNDOUR", 1, "TIMID", "CHARCOAL", ["FLAMETHROWER", "CRUNCH", "SUNNY_DAY", "ROAR"], ("spe", "spa"))
t("HOUNDOOM", "HOUNDOOM", 1, "TIMID", "CHARCOAL", ["FLAMETHROWER", "CRUNCH", "SUNNY_DAY", "SHADOW_BALL"], ("spe", "spa"))
t("ENTEI", "ENTEI", 0, "ADAMANT", "CHARCOAL", ["FLAMETHROWER", "SACRED_FIRE", "EARTHQUAKE", "BULK_UP"], ("hp", "atk"))
t("MOLTRES", "MOLTRES", 0, "MODEST", "CHARCOAL", ["FLAMETHROWER", "AERIAL_ACE", "TOXIC", "REST"], ("hp", "spa"))
t("CHANSEY", "CHANSEY", 0, "BOLD", "LEFTOVERS", ["SOFT_BOILED", "SEISMIC_TOSS", "TOXIC", "THUNDER_WAVE"], ("hp", "def"))
t("BLISSEY", "BLISSEY", 0, "BOLD", "LEFTOVERS", ["SOFT_BOILED", "SEISMIC_TOSS", "TOXIC", "THUNDER_WAVE"], ("hp", "def"))
t("KANGASKHAN", "KANGASKHAN", 0, "ADAMANT", "LEFTOVERS", ["RETURN", "EARTHQUAKE", "FOCUS_PUNCH", "SHADOW_BALL"], ("hp", "atk"))
t("TAUROS", "TAUROS", 0, "JOLLY", "SILK_SCARF", ["RETURN", "EARTHQUAKE", "ROCK_SLIDE", "SWORDS_DANCE"], ("atk", "spe"))
t("DEOXYS", "DEOXYS", 0, "HASTY", "LEFTOVERS", ["PSYCHIC", "THUNDERBOLT", "ICE_BEAM", "RECOVER"], ("spe", "spa"))
t("JIRACHI", "JIRACHI", 0, "CALM", "LEFTOVERS", ["WISH", "PSYCHIC", "THUNDER_WAVE", "FIRE_PUNCH"], ("hp", "spd"))
t("DRATINI", "DRATINI", 0, "ADAMANT", "SITRUS_BERRY", ["DRAGON_DANCE", "EXTREME_SPEED", "AQUA_TAIL", "AERIAL_ACE"], ("atk", "spe"))
t("DRAGONAIR", "DRAGONAIR", 0, "ADAMANT", "SITRUS_BERRY", ["DRAGON_DANCE", "OUTRAGE", "THUNDER_WAVE", "AERIAL_ACE"], ("atk", "spe"))
t("HOOTHOOT", "HOOTHOOT", 0, "CALM", "SITRUS_BERRY", ["HYPNOSIS", "AERIAL_ACE", "REFLECT", "TOXIC"], ("hp", "spd"))
t("NOCTOWL", "NOCTOWL", 0, "CALM", "LEFTOVERS", ["HYPNOSIS", "AERIAL_ACE", "PSYCHIC", "TOXIC"], ("hp", "spd"))
t("ARTICUNO", "ARTICUNO", 0, "BOLD", "LEFTOVERS", ["ICE_BEAM", "AERIAL_ACE", "TOXIC", "REST"], ("hp", "def"))
t("LUGIA", "LUGIA", 0, "CALM", "LEFTOVERS", ["PSYCHIC", "AERIAL_ACE", "CALM_MIND", "RECOVER"], ("hp", "spd"))
t("SLOWPOKE", "SLOWPOKE", 0, "BOLD", "SITRUS_BERRY", ["SURF", "PSYCHIC", "AMNESIA", "REST"], ("hp", "def"))
t("SLOWKING", "SLOWKING", 0, "BOLD", "LEFTOVERS", ["SURF", "PSYCHIC", "AMNESIA", "REST"], ("hp", "def"))
t("DROWZEE", "DROWZEE", 0, "CALM", "SITRUS_BERRY", ["HYPNOSIS", "PSYCHIC", "THUNDER_WAVE", "SHADOW_BALL"], ("hp", "spd"))
t("HYPNO", "HYPNO", 0, "CALM", "LEFTOVERS", ["HYPNOSIS", "PSYCHIC", "THUNDER_WAVE", "SHADOW_BALL"], ("hp", "spd"))
t("LATIAS", "LATIAS", 0, "MODEST", "SOUL_DEW", ["PSYCHIC", "CALM_MIND", "RECOVER", "DRAGON_CLAW"], ("hp", "spa"))
t("CELEBI", "CELEBI", 0, "CALM", "LEFTOVERS", ["PSYCHIC", "LEECH_SEED", "RECOVER", "GIGA_DRAIN"], ("hp", "spd"))
t("POLIWHIRL", "POLIWHIRL", 0, "BOLD", "SITRUS_BERRY", ["SURF", "ICE_BEAM", "HYPNOSIS", "BELLY_DRUM"], ("hp", "spa"))
t("POLITOED", "POLITOED", 0, "BOLD", "LEFTOVERS", ["SURF", "ICE_BEAM", "HYPNOSIS", "RAIN_DANCE"], ("hp", "def"))
t("SUICUNE", "SUICUNE", 0, "BOLD", "LEFTOVERS", ["SURF", "ICE_BEAM", "CALM_MIND", "REST"], ("hp", "def"))
t("GROUDON", "GROUDON", 0, "ADAMANT", "LEFTOVERS", ["EARTHQUAKE", "ROCK_SLIDE", "FIRE_BLAST", "SWORDS_DANCE"], ("hp", "atk"))

# Per-team strength: (IV, EVs on each of the two key stats).
BASE, ELITE, RIVAL_EARLY, RIVAL_MID, RIVAL_LATE = (20, 40), (25, 120), (20, 40), (22, 60), (25, 100)
REMATCH_1, REMATCH_2, REMATCH_3, FINAL = (28, 180), (31, 252), (31, 252), (31, 252)

TEAMS = []  # (array name, [trainer ids], (iv, ev), [(template, level)])


def team(name, ids, power, mons):
    TEAMS.append((name, ids, power, mons))


# Gym leaders: first battle.
team("ROXANNE_1", ["ROXANNE_1"], BASE, [("GEODUDE", 12), ("ARON", 12), ("ANORITH", 13), ("NOSEPASS", 15)])
team("BRAWLY_1", ["BRAWLY_1"], BASE, [("MACHOP", 16), ("MEDITITE", 16), ("HITMONTOP", 17), ("MAKUHITA", 19)])
team("WATTSON_1", ["WATTSON_1"], BASE, [("VOLTORB", 20), ("PLUSLE", 20), ("MAGNETON", 22), ("ELECTABUZZ", 23), ("MANECTRIC", 24)])
team("FLANNERY_1", ["FLANNERY_1"], BASE, [("SLUGMA", 24), ("NUMEL", 24), ("MAGMAR", 25), ("CAMERUPT", 26), ("TORKOAL", 29)])
team("NORMAN_1", ["NORMAN_1"], BASE, [("SPINDA", 27), ("VIGOROTH", 27), ("KECLEON", 28), ("LINOONE", 29), ("SLAKING", 31)])
team("WINONA_1", ["WINONA_1"], BASE, [("SWABLU", 29), ("TROPIUS", 29), ("PELIPPER", 30), ("SKARMORY", 31), ("ALTARIA", 33)])
team("TATE_AND_LIZA_1", ["TATE_AND_LIZA_1"], BASE, [("CLAYDOL", 41), ("XATU", 41), ("LUNATONE", 42), ("SOLROCK", 42), ("GARDEVOIR", 42)])
team("JUAN_1", ["JUAN_1"], BASE, [("WHISCASH", 41), ("LUVDISC", 41), ("SEALEO", 43), ("CRAWDAUNT", 43), ("KINGDRA", 46)])

# Elite Four and Champion.
team("SIDNEY", ["SIDNEY"], ELITE, [("MIGHTYENA", 46), ("SHIFTRY", 48), ("CACTURNE", 46), ("SHARPEDO", 48), ("ABSOL", 49)])
team("PHOEBE", ["PHOEBE"], ELITE, [("SHEDINJA", 48), ("BANETTE", 49), ("GENGAR", 49), ("SABLEYE", 50), ("DUSCLOPS", 51)])
team("GLACIA", ["GLACIA"], ELITE, [("GLALIE", 50), ("SNEASEL", 50), ("JYNX", 52), ("LAPRAS", 52), ("WALREIN", 53)])
team("DRAKE", ["DRAKE"], ELITE, [("FLYGON", 53), ("GYARADOS", 53), ("DRAGONITE", 54), ("LATIOS", 54), ("TYRANITAR", 54), ("SALAMENCE", 55)])
team("WALLACE", ["WALLACE"], ELITE, [("WAILORD", 57), ("TENTACRUEL", 55), ("LUDICOLO", 56), ("STARMIE", 56), ("KYOGRE", 57), ("MILOTIC", 58)])

# Leader rematches. The game has four: they are the old third, fourth and
# fifth, and a last one with every Pokemon at level 100.
REMATCHES = {
    "ROXANNE": [
        [("OMANYTE", 37), ("GOLEM", 37), ("LAIRON", 38), ("ARMALDO", 40), ("KABUTOPS", 40), ("NOSEPASS", 42)],
        [("OMASTAR", 42), ("GOLEM", 42), ("AGGRON", 43), ("KABUTOPS", 45), ("REGIROCK", 47), ("NOSEPASS", 47)],
        [("AERODACTYL", 47), ("AGGRON", 49), ("KABUTOPS", 50), ("REGIROCK", 52), ("REGISTEEL", 52), ("NOSEPASS", 52)],
        [("TYRANITAR", 100), ("AERODACTYL", 100), ("GOLEM", 100), ("AGGRON", 100), ("REGISTEEL", 100), ("REGIROCK", 100)],
    ],
    "BRAWLY": [
        [("MACHAMP", 38), ("MEDICHAM", 38), ("HITMONTOP", 40), ("HITMONLEE", 39), ("HARIYAMA", 42)],
        [("HITMONCHAN", 40), ("MACHAMP", 43), ("MEDICHAM", 43), ("HITMONTOP", 45), ("MEW", 47), ("HARIYAMA", 47)],
        [("HITMONLEE", 46), ("HITMONCHAN", 46), ("MACHAMP", 48), ("MEW", 52), ("MEWTWO", 52), ("HARIYAMA", 52)],
        [("MACHAMP", 100), ("MEDICHAM", 100), ("HITMONCHAN", 100), ("HITMONLEE", 100), ("HARIYAMA", 100), ("MEWTWO", 100)],
    ],
    "WATTSON": [
        [("PIKACHU", 39), ("FLAAFFY", 41), ("ELECTRODE", 41), ("MAGNETON", 43), ("ELECTABUZZ", 44), ("MANECTRIC", 45)],
        [("RAICHU", 44), ("AMPHAROS", 46), ("ELECTRODE", 46), ("MAGNETON", 48), ("RAIKOU", 50), ("MANECTRIC", 50)],
        [("ELECTABUZZ", 50), ("AMPHAROS", 51), ("MAGNETON", 53), ("RAIKOU", 53), ("ZAPDOS", 55), ("MANECTRIC", 55)],
        [("RAIKOU", 100), ("ELECTABUZZ", 100), ("AMPHAROS", 100), ("MANECTRIC", 100), ("MAGNETON", 100), ("ZAPDOS", 100)],
    ],
    "FLANNERY": [
        [("GROWLITHE", 41), ("PONYTA", 41), ("MAGCARGO", 43), ("MAGMAR", 43), ("CAMERUPT", 43), ("TORKOAL", 45)],
        [("HOUNDOUR", 46), ("GROWLITHE", 46), ("RAPIDASH", 46), ("MAGCARGO", 48), ("ENTEI", 50), ("TORKOAL", 50)],
        [("ARCANINE", 51), ("HOUNDOOM", 51), ("MAGCARGO", 53), ("ENTEI", 53), ("MOLTRES", 55), ("TORKOAL", 55)],
        [("ARCANINE", 100), ("HOUNDOOM", 100), ("MAGCARGO", 100), ("ENTEI", 100), ("TORKOAL", 100), ("MOLTRES", 100)],
    ],
    "NORMAN": [
        [("KANGASKHAN", 45), ("CHANSEY", 47), ("LINOONE", 47), ("SPINDA", 48), ("KECLEON", 48), ("SLAKING", 50)],
        [("KANGASKHAN", 50), ("BLISSEY", 52), ("LINOONE", 52), ("SPINDA", 53), ("DEOXYS", 55), ("SLAKING", 55)],
        [("KANGASKHAN", 55), ("BLISSEY", 57), ("TAUROS", 57), ("DEOXYS", 57), ("JIRACHI", 58), ("SLAKING", 60)],
        [("BLISSEY", 100), ("KANGASKHAN", 100), ("TAUROS", 100), ("SLAKING", 100), ("JIRACHI", 100), ("DEOXYS", 100)],
    ],
    "WINONA": [
        [("HOOTHOOT", 43), ("TROPIUS", 43), ("DRAGONAIR", 45), ("PELIPPER", 46), ("SKARMORY", 48), ("ALTARIA", 50)],
        [("NOCTOWL", 48), ("TROPIUS", 49), ("PELIPPER", 51), ("SKARMORY", 53), ("ARTICUNO", 53), ("ALTARIA", 55)],
        [("PELIPPER", 55), ("DRAGONITE", 55), ("ARTICUNO", 56), ("SKARMORY", 58), ("LUGIA", 60), ("ALTARIA", 60)],
        [("ARTICUNO", 100), ("SKARMORY", 100), ("DRAGONITE", 100), ("ALTARIA", 100), ("PELIPPER", 100), ("LUGIA", 100)],
    ],
    "TATE_AND_LIZA": [
        [("DROWZEE", 53), ("SLOWPOKE", 53), ("CLAYDOL", 54), ("XATU", 54), ("LUNATONE", 55), ("SOLROCK", 55)],
        [("HYPNO", 58), ("CLAYDOL", 59), ("XATU", 59), ("LATIAS", 59), ("LUNATONE", 60), ("SOLROCK", 60)],
        [("HYPNO", 63), ("CLAYDOL", 64), ("LATIAS", 64), ("CELEBI", 65), ("LUNATONE", 65), ("SOLROCK", 65)],
        [("CLAYDOL", 100), ("GARDEVOIR", 100), ("XATU", 100), ("SOLROCK", 100), ("CELEBI", 100), ("LATIAS", 100)],
    ],
    "JUAN": [
        [("POLIWHIRL", 50), ("WHISCASH", 51), ("LAPRAS", 52), ("WALREIN", 53), ("CRAWDAUNT", 53), ("KINGDRA", 56)],
        [("LAPRAS", 56), ("POLIWHIRL", 56), ("WHISCASH", 58), ("SUICUNE", 58), ("CRAWDAUNT", 58), ("KINGDRA", 61)],
        [("POLITOED", 61), ("WHISCASH", 63), ("SUICUNE", 63), ("CRAWDAUNT", 63), ("GROUDON", 65), ("KINGDRA", 66)],
        [("LAPRAS", 100), ("POLITOED", 100), ("CRAWDAUNT", 100), ("SUICUNE", 100), ("GROUDON", 100), ("KINGDRA", 100)],
    ],
}
STRENGTH = [REMATCH_1, REMATCH_2, REMATCH_3, FINAL]
for leader, tiers in REMATCHES.items():
    for number, (mons, power) in enumerate(zip(tiers, STRENGTH), start=2):
        team("%s_%d" % (leader, number), ["%s_%d" % (leader, number)], power, mons)

# The rival (Brendan or May: the same team), by the player's starter.
# TREECKO: the rival has Torchic; MUDKIP: Treecko; TORCHIC: Mudkip.
RIVAL = {
    "TREECKO": {
        "103": [("TORCHIC5", 5)],
        "RUSTBORO": [("LOTAD", 13), ("RALTS", 13), ("TORCHIC15", 15)],
        "110": [("WINGULL", 18), ("ELECTRIKE", 18), ("LOMBRE", 18), ("COMBUSKEN110", 20)],
        "119": [("PELIPPER_R", 29), ("KIRLIA", 29), ("LOMBRE", 29), ("MANECTRIC_R", 30), ("COMBUSKEN_R", 31)],
        "LILYCOVE": [("TROPIUS_R", 31), ("PELIPPER_R", 32), ("LUDICOLO_R", 32), ("MANECTRIC_R", 33), ("GARDEVOIR_R", 33), ("COMBUSKEN_R", 34)],
    },
    "MUDKIP": {
        "103": [("TREECKO5", 5)],
        "RUSTBORO": [("WINGULL", 13), ("RALTS", 13), ("TREECKO15", 15)],
        "110": [("SLUGMA_R", 18), ("WINGULL", 18), ("ELECTRIKE", 18), ("GROVYLE110", 20)],
        "119": [("SLUGMA_R", 29), ("PELIPPER_R", 29), ("KIRLIA", 29), ("MANECTRIC_R", 30), ("GROVYLE_R", 31)],
        "LILYCOVE": [("TROPIUS_R", 31), ("SLUGMA_R", 32), ("PELIPPER_R", 32), ("MANECTRIC_R", 33), ("GARDEVOIR_R", 33), ("GROVYLE_R", 34)],
    },
    "TORCHIC": {
        "103": [("MUDKIP5", 5)],
        "RUSTBORO": [("SLUGMA_R", 13), ("RALTS", 13), ("MUDKIP15", 15)],
        "110": [("LOMBRE", 18), ("SLUGMA_R", 18), ("ELECTRIKE", 18), ("MARSHTOMP110", 20)],
        "119": [("LOMBRE", 29), ("SLUGMA_R", 29), ("KIRLIA", 29), ("MANECTRIC_R", 30), ("MARSHTOMP_R", 31)],
        "LILYCOVE": [("TROPIUS_R", 31), ("LUDICOLO_R", 32), ("SLUGMA_R", 32), ("MANECTRIC_R", 33), ("GARDEVOIR_R", 33), ("MARSHTOMP_R", 34)],
    },
}
RIVAL_POWER = {"103": RIVAL_EARLY, "RUSTBORO": RIVAL_EARLY, "110": RIVAL_EARLY, "119": RIVAL_MID, "LILYCOVE": RIVAL_LATE}
for starter, encounters in RIVAL.items():
    for place, mons in encounters.items():
        if place in ("RUSTBORO", "LILYCOVE"):
            ids = ["BRENDAN_%s_%s" % (place, starter), "MAY_%s_%s" % (place, starter)]
        else:
            ids = ["BRENDAN_ROUTE_%s_%s" % (place, starter), "MAY_ROUTE_%s_%s" % (place, starter)]
        team("RIVAL_%s_%s" % (place, starter), ids, RIVAL_POWER[place], mons)


def constants(tree):
    names = set()
    for header in ("species", "moves", "items", "pokemon", "abilities", "opponents"):
        path = tree / "include/constants" / (header + ".h")
        names.update(re.findall(r"#define\s+(\w+)", path.read_text(encoding="utf-8")))
    return names


def emit(tree):
    known = constants(tree) if tree else None
    errors = []

    def check(name):
        if known is not None and name not in known and name != "MOVE_NONE":
            errors.append(name)

    out = ["/* Generated by tools/gen_trainer_teams.py from docs/DIFFICULTY_PLAN.md; do not edit. */", ""]
    table = []
    for name, ids, (iv, ev), mons in TEAMS:
        out.append("static const struct TeamMon s%s[] =" % "".join(w.capitalize() for w in name.split("_")))
        out.append("{")
        for key, level in mons:
            species, slot, nature, item, moves, evs = T[key]
            ivs = [0] * 6
            for stat in evs:
                ivs[EV[stat]] = ev
            check("SPECIES_" + species)
            check("ITEM_" + item)
            check("NATURE_" + nature)
            for move in moves:
                check("MOVE_" + move)
            out.append("    {SPECIES_%s, %d, %d, NATURE_%s, %d, ITEM_%s," % (species, level, slot, nature, iv, item))
            out.append("     {%s}, {%s}}," % (", ".join("MOVE_" + m for m in moves), ", ".join(str(v) for v in ivs)))
        out.append("};")
        out.append("")
        array = "s%s" % "".join(w.capitalize() for w in name.split("_"))
        for trainer in ids:
            check("TRAINER_" + trainer)
            table.append("    {TRAINER_%s, ARRAY_COUNT(%s), %s}," % (trainer, array, array))
    out.append("static const struct Team sTeams[] =")
    out.append("{")
    out.extend(table)
    out.append("};")
    return "\n".join(out) + "\n", errors


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--tree", type=Path, default=ROOT / "build" / "upstream")
    args = ap.parse_args()
    text, errors = emit(args.tree if args.tree.exists() else None)
    if errors:
        print("unknown names:", " ".join(sorted(set(errors))))
        return 1
    (ROOT / "3ds_port/src/3ds_trainer_teams_data.h").write_text(text, encoding="utf-8", newline="\n")
    print("%d teams, %d lines" % (len(TEAMS), text.count("\n")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
