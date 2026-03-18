#ifndef INC_1OOM_GAME_SHIPTECH_H
#define INC_1OOM_GAME_SHIPTECH_H

#include "game_types.h"
#include "game_shiptech_types.h"
#include "types.h"

struct shiptech_weap_s {
    char const * const * const nameptr;
    char const * const * const extratextptr;
    uint16_t damagemin; /* damagemax != damagemin means beam or bomb weapon */
    uint16_t damagemax;
    uint8_t range;
    uint8_t extraacc;
    bool halveshield;
    bool is_bomb;
    bool damagefade;
    uint8_t misstype;
    uint8_t damagemul;
    uint8_t numfire;
    int8_t numshots;
    uint16_t cost;
    uint16_t space;
    uint16_t power;
    bool is_bio;
    uint8_t tech_i;
    uint8_t v24;    /* beam: ? ; missile: fuel */
    uint8_t dtbl[7]; /* beam: color table ; missile: 0=speed */
    uint8_t sound;
    uint8_t nummiss; /* beam: streaming */
};

struct shiptech_comp_s {
    char const * const * const nameptr;
    uint16_t power[SHIP_HULL_NUM];
    uint16_t space[SHIP_HULL_NUM];
    uint16_t cost[SHIP_HULL_NUM];
    uint8_t tech_i;
    uint8_t level;
};

struct shiptech_jammer_s {
    char const * const * const nameptr;
    uint16_t power[SHIP_HULL_NUM];
    uint16_t space[SHIP_HULL_NUM];
    uint16_t cost[SHIP_HULL_NUM];
    uint8_t tech_i;
    uint8_t level;
};

struct shiptech_engine_s {
    char const * const * const nameptr;
    uint16_t power;
    uint16_t space;
    uint16_t cost;
    uint8_t warp;
    uint8_t tech_i;
};

struct shiptech_armor_s {
    char const * const * const nameptr;
    uint16_t cost[SHIP_HULL_NUM];
    uint16_t space[SHIP_HULL_NUM];
    uint16_t armor;
    uint8_t tech_i;
};

struct shiptech_shield_s {
    char const * const * const nameptr;
    uint16_t cost[SHIP_HULL_NUM];
    uint16_t space[SHIP_HULL_NUM];
    uint16_t power[SHIP_HULL_NUM];
    uint8_t absorb;
    uint8_t tech_i;
};

typedef enum {
    SHIP_SPECIAL_BOOL_SCANNER = 0,
    SHIP_SPECIAL_BOOL_REPULSOR, /*1*/
    SHIP_SPECIAL_BOOL_WARPDIS, /*2*/
    SHIP_SPECIAL_BOOL_STASIS, /*3*/
    SHIP_SPECIAL_BOOL_CLOAK, /*4*/
    SHIP_SPECIAL_BOOL_BLACKHOLE, /*5*/
    SHIP_SPECIAL_BOOL_SUBSPACE, /*6*/
    SHIP_SPECIAL_BOOL_TECHNULL, /*7*/
    SHIP_SPECIAL_BOOL_ORACLE, /*8*/
    SHIP_SPECIAL_BOOL_DISP /*9*/
} ship_special_bool_i_t;

struct shiptech_special_s {
    char const * const * const nameptr;
    char const * const * const extratextptr;
    uint16_t cost[SHIP_HULL_NUM];
    uint16_t space[SHIP_HULL_NUM];
    uint16_t power[SHIP_HULL_NUM];
    uint8_t tech_i;
    tech_field_t field;
    uint8_t type;
    uint8_t repair;
    uint8_t extraman;
    uint8_t misshield;
    uint8_t extrarange;
    uint8_t pulsar;
    uint8_t stream;
    uint16_t boolmask;
};

struct shiptech_hull_s {
    char const * const * const nameptr;
    uint16_t cost; /* BC*10*/
    uint16_t space;
    uint16_t hits;
    uint16_t power;
    int16_t defense;
};

extern struct shiptech_weap_s tbl_shiptech_weap[WEAPON_NUM];
extern struct shiptech_comp_s tbl_shiptech_comp[SHIP_COMP_NUM];
extern struct shiptech_engine_s tbl_shiptech_engine[SHIP_ENGINE_NUM];
extern struct shiptech_armor_s tbl_shiptech_armor[SHIP_ARMOR_NUM];
extern struct shiptech_shield_s tbl_shiptech_shield[SHIP_SHIELD_NUM];
extern struct shiptech_jammer_s tbl_shiptech_jammer[SHIP_JAMMER_NUM];
extern struct shiptech_special_s tbl_shiptech_special[SHIP_SPECIAL_NUM];
extern struct shiptech_hull_s tbl_shiptech_hull[SHIP_HULL_NUM];

#endif
