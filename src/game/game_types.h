#ifndef INC_1OOM_GAME_TYPES_H
#define INC_1OOM_GAME_TYPES_H

#include "types.h"

typedef enum {
    PLAYER_0 = 0,
    PLAYER_1,
    PLAYER_2,
    PLAYER_3,
    PLAYER_4,
    PLAYER_5,
    PLAYER_NUM
} player_id_t;

#define PLAYER_NONE PLAYER_NUM

typedef enum tech_field_e {
    TECH_FIELD_COMPUTER = 0,
    TECH_FIELD_CONSTRUCTION, /*1*/
    TECH_FIELD_FORCE_FIELD, /*2*/
    TECH_FIELD_PLANETOLOGY, /*3*/
    TECH_FIELD_PROPULSION, /*4*/
    TECH_FIELD_WEAPON, /*5*/
    TECH_FIELD_NUM
} tech_field_t;

typedef enum race_e {
    RACE_HUMAN = 0,
    RACE_MRRSHAN, /*1*/
    RACE_SILICOID, /*2*/
    RACE_SAKKRA, /*3*/
    RACE_PSILON, /*4*/
    RACE_ALKARI, /*5*/
    RACE_KLACKON, /*6*/
    RACE_BULRATHI, /*7*/
    RACE_MEKLAR, /*8*/
    RACE_DARLOK, /*9*/
    RACE_NUM
} race_t;

#define RACE_RANDOM RACE_NUM

typedef enum banner_e {
    BANNER_BLUE = 0,
    BANNER_GREEN, /*1*/
    BANNER_PURPLE, /*2*/
    BANNER_RED,  /*3*/
    BANNER_WHITE, /*4*/
    BANNER_YELLOW, /*5*/
    BANNER_NUM
} banner_t;

#define BANNER_RANDOM   BANNER_NUM

typedef enum {
    GALAXY_SIZE_SMALL = 0,
    GALAXY_SIZE_MEDIUM, /*1*/
    GALAXY_SIZE_LARGE, /*2*/
    GALAXY_SIZE_HUGE, /*3*/
    GALAXY_SIZE_NUM
} galaxy_size_t;

typedef enum {
    DIFFICULTY_SIMPLE = 0,
    DIFFICULTY_EASY, /*1*/
    DIFFICULTY_AVERAGE, /*2*/
    DIFFICULTY_HARD, /*3*/
    DIFFICULTY_IMPOSSIBLE, /*4*/
    DIFFICULTY_NUM
} difficulty_t;

typedef enum {
    TREATY_NONE = 0,
    TREATY_NONAGGRESSION, /*1*/
    TREATY_ALLIANCE, /*2*/
    TREATY_WAR, /*3*/
    TREATY_FINAL_WAR, /*4*/
    TREATY_NUM
} treaty_t;

typedef enum {
    SPYMODE_HIDE = 0,
    SPYMODE_ESPIONAGE, /*1*/
    SPYMODE_SABOTAGE /*2*/
} spymode_t;

typedef enum {
    TRAIT1_XENOPHOBIC = 0,
    TRAIT1_RUTHLESS, /*1*/
    TRAIT1_AGGRESSIVE, /*2*/
    TRAIT1_ERRATIC, /*3*/
    TRAIT1_HONORABLE, /*4*/
    TRAIT1_PACIFISTIC, /*5*/
    TRAIT1_NUM
} trait1_t;

typedef enum {
    TRAIT2_DIPLOMAT = 0,
    TRAIT2_MILITARIST, /*1*/
    TRAIT2_EXPANSIONIST, /*2*/
    TRAIT2_TECHNOLOGIST, /*3*/
    TRAIT2_INDUSTRIALIST, /*4*/
    TRAIT2_ECOLOGIST, /*5*/
    TRAIT2_NUM
} trait2_t;

typedef enum {
    GAME_EVENT_NONE = 0,
    GAME_EVENT_PLAGUE, /*1*/
    GAME_EVENT_QUAKE, /*2*/
    GAME_EVENT_NOVA, /*3*/
    GAME_EVENT_ACCIDENT, /*4*/
    GAME_EVENT_ASSASSIN, /*5*/
    GAME_EVENT_VIRUS, /*6*/
    GAME_EVENT_COMET, /*7*/
    GAME_EVENT_PIRATES, /*8*/
    GAME_EVENT_DERELICT, /*9*/
    GAME_EVENT_REBELLION, /*10*/
    GAME_EVENT_CRYSTAL, /*11*/
    GAME_EVENT_AMOEBA, /*12*/
    GAME_EVENT_ENVIRO, /*13*/
    GAME_EVENT_RICH, /*14*/
    GAME_EVENT_SUPPORT, /*15*/
    GAME_EVENT_POOR, /*16*/
    GAME_EVENT_NUM
} gameevent_type_t;

typedef enum {
    MONSTER_CRYSTAL = 0,
    MONSTER_AMOEBA, /*1*/
    MONSTER_GUARDIAN, /*2*/
    MONSTER_NUM
} monster_id_t;

typedef enum game_end_type_e {
    GAME_END_NONE = 0,
    GAME_END_LOST_EXILE, /*1*/
    GAME_END_WON_GOOD, /*2*/
    GAME_END_FINAL_WAR, /*3*/
    GAME_END_WON_TYRANT,
    GAME_END_LOST_FUNERAL,
    GAME_END_QUIT
} game_end_type_t;

typedef enum {
    GOVERNOR_ECO_MODE_GROW_BEFORE_DEF = 0,
    GOVERNOR_ECO_MODE_GROW_BEFORE_LAST, /*1*/
    GOVERNOR_ECO_MODE_NEVER_GROW, /*2*/
    GOVERNOR_ECO_MODE_DO_NOT_DECREASE, /*3*/
    GOVERNOR_ECO_MODE_DO_NOT_TOUCH, /*4*/
    GOVERNOR_ECO_MODE_NUM
} governor_eco_mode_t;

typedef enum {
    TECHSOURCE_RESEARCH = 0,
    TECHSOURCE_SPY, /*1*/
    TECHSOURCE_FOUND, /*2*/
    TECHSOURCE_AI_SPY, /*3*/
    TECHSOURCE_TRADE /*4*/
} techsource_t;

#define TECHSOURCE_CHOOSE   TECHSOURCE_AI_SPY

typedef uint16_t shipcount_t;
typedef uint32_t shipsum_t;

#define SHIP_NAME_NUM   12
#define NUM_SHIPDESIGNS 6
#define BUILDSHIP_STARGATE NUM_SHIPDESIGNS

#define TECH_SPY_MAX    6
#define TECH_NEXT_MAX   12

#define FLEET_ENROUTE_AI_MAX    208
#define FLEET_ENROUTE_MAX   260

#define TRANSPORT_MAX   100

#define PLANETS_MAX 108

#endif
