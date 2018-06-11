#ifndef INC_1OOM_GAME_BATTLE_H
#define INC_1OOM_GAME_BATTLE_H

#include "game_design.h"
#include "game_shiptech.h"
#include "game_types.h"
#include "types.h"

struct game_s;
struct game_aux_s;

#define BATTLE_AREA_W 10
#define BATTLE_AREA_H 8

typedef enum { SIDE_L, SIDE_R, SIDE_NONE } battle_side_i_t;

struct battle_item_s {
    uint8_t *gfx;
    char name[SHIP_NAME_LEN];
    uint16_t f10;
    uint16_t f12;
    uint16_t f14;
    uint16_t f16;
    uint8_t shiptbli;
    uint8_t complevel;
    ship_special_t special[SPECIAL_SLOT_NUM];
    uint16_t hp1;
    uint16_t hp2;
    uint8_t man;
    uint8_t defense;
    uint8_t misdefense;
    uint8_t absorb;
    uint8_t repair;
    uint8_t misshield;
    uint8_t pshield;
    uint8_t extrarange;
    shipcount_t num;
    uint8_t look;
    int8_t pulsar; /* temporarily used as antidote for planets */
    int8_t stream;
    int8_t stasis;
    int8_t cloak;
    int8_t subspace;
    int8_t warpdis;
    int8_t blackhole;
    int8_t technull;
    int8_t repulsor;
    uint16_t sbmask;
    uint8_t retreat;
    int8_t sx;  /* -1, 0..BATTLE_AREA_W - 1 */
    int8_t sy;  /* -1, 0..BATTLE_AREA_H - 1 */
    uint8_t f48;    /* 0, 1, 2 */
    uint8_t stasisby;
    uint8_t unman;
    bool can_retaliate;
    battle_side_i_t side;
    int8_t actman;
    uint16_t hploss;
    int8_t maxrange;
    int8_t missile; /* -1=none, 0=disabled, 1=enabled */
    struct {
        weapon_t t;
        uint8_t n;
        int8_t numfire;
        int8_t numshots;
    } wpn[WEAPON_SLOT_NUM];
};

#define BATTLE_ROCK_MAX 7

struct battle_rock_s {
    int8_t sx;  /* -1, 0..9 */
    int8_t sy;  /* -1, 0..9 */
    uint8_t *gfx;
};

#define BATTLE_MISSILE_MAX 30
#define MISSILE_TARGET_NONE -1

struct battle_missile_s {
    uint16_t nummissiles;
    uint16_t damagemul2;
    int8_t source;  /* item index */
    int8_t target;  /* item index or MISSILE_TARGET_NONE */
    int16_t x;
    int16_t y;
    uint16_t hmm0c;
    weapon_t wpnt;
    uint16_t hmm10;
};

struct battle_side_s {
    int party;
    race_t race;
    shipcount_t tbl_ships[NUM_SHIPDESIGNS];
    uint8_t tbl_shiptype[NUM_SHIPDESIGNS];
    uint8_t num_types;
    uint8_t items; /* not counting planet */
    uint32_t apparent_force;
    bool flag_have_scan;
    bool flag_base_missile;
    bool flag_human;
    int16_t flag_auto; /* HACK type is for uiobj */
};

#define BATTLE_ITEM_MAX (NUM_SHIPDESIGNS * 2 + 1/*planet*/)

struct battle_s {
    struct game_s *g;
    void *uictx;
    uint8_t planet_i;
    battle_side_i_t planet_side;
    int16_t pop;
    uint16_t fact;
    uint16_t bases;
    uint16_t biodamage;
    struct battle_side_s s[2];
    bool hmm21;
    bool hmm24;
    bool hmm30;
    uint8_t num_repulsed;
    bool flag_human_att;
    bool flag_cur_item_destroyed;
    uint8_t items_num; /* in item table, not counting planet at 0 */
    uint8_t items_num2; /* in item table, not counting planet at 0 */
    uint8_t cur_item; /* in item table */
    int8_t special_button;
    uint32_t popdamage;
    uint32_t factdamage;
    uint16_t num_turn;
    bool have_subspace_int;
    uint8_t antidote;
    struct battle_item_s item[BATTLE_ITEM_MAX];
    uint8_t prio_i;
    int16_t priority[BATTLE_ITEM_MAX];
    uint8_t num_rocks;
    struct battle_rock_s rock[BATTLE_ROCK_MAX];
    uint8_t num_missile;
    struct battle_missile_s missile[BATTLE_MISSILE_MAX];
    /*
        -100 : rock
        -30 : enemy planet, out of weapon range
        -itemi : enemy item, out of weapon range
        0 : empty, too far or friendly planet
        1 : empty, can move to
        10 + itemi : friendly item
        30 + itemi : enemy item, in weapon range
    */
    int8_t area[BATTLE_AREA_H][BATTLE_AREA_W];
};

extern void game_battle_prepare(struct battle_s *bt, int party_r, int party_l, uint8_t planet_i);
extern void game_battle_finish(struct battle_s *bt);
extern void game_battle_handle_all(struct game_s *g);

#endif
