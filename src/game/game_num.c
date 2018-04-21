#include "config.h"

#include <string.h>

#include "game_num.h"
#include "gameapi.h"
#include "game_shiptech.h"
#include "lib.h"
#include "log.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

#define DEFNUMITEMLSTBL(id, name, field, type, vmin, vmax) { #id, &(name->field), type, sizeof(*name), sizeof(name) / sizeof(*name), vmin, vmax }
#define DEFNUMITEMLTBL(id, name, type, vmin, vmax)  { #id, &name, type, sizeof(*name), sizeof(name) / sizeof(*name), vmin, vmax }
#define DEFNUMITEMLTBLL(name, type, vmin, vmax)     { #name, game_num_##name, type, sizeof(*game_num_##name), sizeof(game_num_##name) / sizeof(*game_num_##name), vmin, vmax }
#define DEFNUMITEM(id, name, type, vmin, vmax)      { #id, &name, type, sizeof(name), 1, vmin, vmax }
#define DEFNUMITEML(name, type, vmin, vmax)         DEFNUMITEM(name, game_num_##name, type, vmin, vmax)
#define DEFNUMEND           { NULL, NULL, NUMTYPE_S, 0, 0, 0, 0 }

#define NUMTYPE_GUESS(z)    ((sizeof(z) == 4) ? NUMTYPE_U32 : ((sizeof(z) == 2) ? NUMTYPE_U16 : NUMTYPE_U8))
#define NUMTYPE_IS_S(t)     (!(t & 1))

static const struct numtbl_s {
    const char *numid;
    void *ptr;
    enum {
        NUMTYPE_S = 0,
        NUMTYPE_U,
        NUMTYPE_S8,
        NUMTYPE_U8,
        NUMTYPE_S16,
        NUMTYPE_U16,
        NUMTYPE_S32,
        NUMTYPE_U32,
        NUMTYPE_BOOL
    } numtype;
    int tstep;
    int size;
    int vmin;
    uint32_t vmax;
} game_num_id_tbl[] = {
    DEFNUMITEMLSTBL(st_weap_dmgmin, tbl_shiptech_weap, damagemin, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_weap_dmgmax, tbl_shiptech_weap, damagemax, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_weap_range, tbl_shiptech_weap, range, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_weap_extraacc, tbl_shiptech_weap, extraacc, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_weap_halveshield, tbl_shiptech_weap, halveshield, NUMTYPE_BOOL, 0, 1),
    DEFNUMITEMLSTBL(st_weap_is_bomb, tbl_shiptech_weap, is_bomb, NUMTYPE_BOOL, 0, 1),
    DEFNUMITEMLSTBL(st_weap_dmgfade, tbl_shiptech_weap, damagefade, NUMTYPE_BOOL, 0, 1),
    DEFNUMITEMLSTBL(st_weap_misstype, tbl_shiptech_weap, misstype, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_weap_dmgmul, tbl_shiptech_weap, damagemul, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_weap_numfire, tbl_shiptech_weap, numfire, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_weap_numshots, tbl_shiptech_weap, numshots, NUMTYPE_S8, -1, 0x7f),
    DEFNUMITEMLSTBL(st_weap_cost, tbl_shiptech_weap, cost, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_weap_space, tbl_shiptech_weap, space, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_weap_power, tbl_shiptech_weap, power, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_weap_is_bio, tbl_shiptech_weap, is_bio, NUMTYPE_BOOL, 0, 1),
    DEFNUMITEMLSTBL(st_comp_tech, tbl_shiptech_weap, tech_i, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_v24, tbl_shiptech_weap, v24, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_dtbl0, tbl_shiptech_weap, dtbl[0], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_dtbl1, tbl_shiptech_weap, dtbl[1], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_dtbl2, tbl_shiptech_weap, dtbl[2], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_dtbl3, tbl_shiptech_weap, dtbl[3], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_dtbl4, tbl_shiptech_weap, dtbl[4], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_dtbl4, tbl_shiptech_weap, dtbl[5], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_dtbl6, tbl_shiptech_weap, dtbl[6], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_sound, tbl_shiptech_weap, v24, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_nummiss, tbl_shiptech_weap, nummiss, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_cost0, tbl_shiptech_comp, cost[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_cost1, tbl_shiptech_comp, cost[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_cost2, tbl_shiptech_comp, cost[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_cost3, tbl_shiptech_comp, cost[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_space0, tbl_shiptech_comp, space[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_space1, tbl_shiptech_comp, space[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_space2, tbl_shiptech_comp, space[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_space3, tbl_shiptech_comp, space[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_power0, tbl_shiptech_comp, power[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_power1, tbl_shiptech_comp, power[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_power2, tbl_shiptech_comp, power[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_power3, tbl_shiptech_comp, power[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_comp_tech, tbl_shiptech_comp, tech_i, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_comp_level, tbl_shiptech_comp, level, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_jamm_cost0, tbl_shiptech_jammer, cost[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_cost1, tbl_shiptech_jammer, cost[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_cost2, tbl_shiptech_jammer, cost[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_cost3, tbl_shiptech_jammer, cost[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_space0, tbl_shiptech_jammer, space[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_space1, tbl_shiptech_jammer, space[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_space2, tbl_shiptech_jammer, space[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_space3, tbl_shiptech_jammer, space[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_power0, tbl_shiptech_jammer, power[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_power1, tbl_shiptech_jammer, power[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_power2, tbl_shiptech_jammer, power[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_power3, tbl_shiptech_jammer, power[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_jamm_tech, tbl_shiptech_jammer, tech_i, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_jamm_level, tbl_shiptech_jammer, level, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_engn_cost, tbl_shiptech_engine, cost, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_engn_space, tbl_shiptech_engine, space, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_engn_power, tbl_shiptech_engine, power, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_engn_warp, tbl_shiptech_engine, warp, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_engn_tech, tbl_shiptech_engine, tech_i, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_armr_cost0, tbl_shiptech_armor, cost[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_armr_cost1, tbl_shiptech_armor, cost[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_armr_cost2, tbl_shiptech_armor, cost[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_armr_cost3, tbl_shiptech_armor, cost[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_armr_space0, tbl_shiptech_armor, space[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_armr_space1, tbl_shiptech_armor, space[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_armr_space2, tbl_shiptech_armor, space[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_armr_space3, tbl_shiptech_armor, space[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_armr_tech, tbl_shiptech_armor, tech_i, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_armr_armor, tbl_shiptech_armor, armor, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_cost0, tbl_shiptech_shield, cost[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_cost1, tbl_shiptech_shield, cost[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_cost2, tbl_shiptech_shield, cost[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_cost3, tbl_shiptech_shield, cost[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_space0, tbl_shiptech_shield, space[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_space1, tbl_shiptech_shield, space[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_space2, tbl_shiptech_shield, space[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_space3, tbl_shiptech_shield, space[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_power0, tbl_shiptech_shield, power[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_power1, tbl_shiptech_shield, power[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_power2, tbl_shiptech_shield, power[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_power3, tbl_shiptech_shield, power[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_absorb, tbl_shiptech_shield, absorb, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_shld_tech, tbl_shiptech_shield, tech_i, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_cost0, tbl_shiptech_special, cost[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_cost1, tbl_shiptech_special, cost[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_cost2, tbl_shiptech_special, cost[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_cost3, tbl_shiptech_special, cost[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_space0, tbl_shiptech_special, space[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_space1, tbl_shiptech_special, space[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_space2, tbl_shiptech_special, space[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_space3, tbl_shiptech_special, space[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_power0, tbl_shiptech_special, power[0], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_power1, tbl_shiptech_special, power[1], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_power2, tbl_shiptech_special, power[2], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_power3, tbl_shiptech_special, power[3], NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_spec_tech, tbl_shiptech_special, tech_i, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_field, tbl_shiptech_special, field, NUMTYPE_GUESS(tbl_shiptech_special[0].field), 0, 5),
    DEFNUMITEMLSTBL(st_spec_type, tbl_shiptech_special, type, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_repair, tbl_shiptech_special, repair, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_extraman, tbl_shiptech_special, extraman, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_misshield, tbl_shiptech_special, misshield, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_extrarange, tbl_shiptech_special, extrarange, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_pulsar, tbl_shiptech_special, pulsar, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_stream, tbl_shiptech_special, stream, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLSTBL(st_spec_boolmask, tbl_shiptech_special, boolmask, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_hull_cost, tbl_shiptech_hull, cost, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_hull_space, tbl_shiptech_hull, space, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_hull_hits, tbl_shiptech_hull, hits, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_hull_power, tbl_shiptech_hull, power, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLSTBL(st_hull_defense, tbl_shiptech_hull, defense, NUMTYPE_S16, -0x8000, 0x7fff),
    DEFNUMITEML(bt_turn_max, NUMTYPE_S, 1, 0xffff),
    DEFNUMITEML(stargate_cost, NUMTYPE_S, 1, 0xffff),
    DEFNUMITEML(weapon_list_max, NUMTYPE_S, 0, WEAPON_NUM),
    DEFNUMITEML(limit_ships, NUMTYPE_S, 1, 0xffff),
    DEFNUMITEML(limit_ships_all, NUMTYPE_S, 1, 0xffff),
    DEFNUMITEML(max_pop, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(max_factories, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(max_inbound, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(atmos_cost, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(soil_cost, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(adv_soil_cost, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(adv_scan_range, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(pop_hp, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(fact_hp, NUMTYPE_S, 1, 0x7fff),
    DEFNUMITEML(max_bomb_dmg, NUMTYPE_S, 1, 0x7fffffff),
    DEFNUMITEML(max_bio_dmg, NUMTYPE_S, 1, 0x7fffffff),
    DEFNUMITEML(max_trans_dmg, NUMTYPE_S, 1, 0x7fffffff),
    DEFNUMITEML(event_roll, NUMTYPE_S, 1, 0x7fffffff),
    DEFNUMITEMLTBLL(tbl_hull_w, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBLL(tech_costmuld, NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_human, game_num_tech_costmulr[RACE_HUMAN], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_mrrshan, game_num_tech_costmulr[RACE_MRRSHAN], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_silicoid, game_num_tech_costmulr[RACE_SILICOID], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_sakkra, game_num_tech_costmulr[RACE_SAKKRA], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_psilon, game_num_tech_costmulr[RACE_PSILON], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_alkari, game_num_tech_costmulr[RACE_ALKARI], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_klackon, game_num_tech_costmulr[RACE_KLACKON], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_bulrathi, game_num_tech_costmulr[RACE_BULRATHI], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_meklar, game_num_tech_costmulr[RACE_MEKLAR], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBL(tcostm_darlok, game_num_tech_costmulr[RACE_DARLOK], NUMTYPE_U8, 0, 0xff),
    DEFNUMITEMLTBLL(base_hp, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLTBLL(pshield_cost, NUMTYPE_U16, 0, 0xffff),
    DEFNUMITEMLTBL(trait1_human, game_num_tbl_trait1[RACE_HUMAN], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_mrrshan, game_num_tbl_trait1[RACE_MRRSHAN], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_silicoid, game_num_tbl_trait1[RACE_SILICOID], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_sakkra, game_num_tbl_trait1[RACE_SAKKRA], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_psilon, game_num_tbl_trait1[RACE_PSILON], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_alkari, game_num_tbl_trait1[RACE_ALKARI], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_klackon, game_num_tbl_trait1[RACE_KLACKON], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_bulrathi, game_num_tbl_trait1[RACE_BULRATHI], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_meklar, game_num_tbl_trait1[RACE_MEKLAR], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait1_darlok, game_num_tbl_trait1[RACE_DARLOK], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_human, game_num_tbl_trait2[RACE_HUMAN], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_mrrshan, game_num_tbl_trait2[RACE_MRRSHAN], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_silicoid, game_num_tbl_trait2[RACE_SILICOID], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_sakkra, game_num_tbl_trait2[RACE_SAKKRA], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_psilon, game_num_tbl_trait2[RACE_PSILON], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_alkari, game_num_tbl_trait2[RACE_ALKARI], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_klackon, game_num_tbl_trait2[RACE_KLACKON], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_bulrathi, game_num_tbl_trait2[RACE_BULRATHI], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_meklar, game_num_tbl_trait2[RACE_MEKLAR], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBL(trait2_darlok, game_num_tbl_trait2[RACE_DARLOK], NUMTYPE_U8, 0, 5),
    DEFNUMITEMLTBLL(tbl_tech_autoadj, NUMTYPE_U8, 0, 100),
    /* TODO starting ships, monster ships, ... */
    DEFNUMEND
};

/* -------------------------------------------------------------------------- */

int game_num_bt_turn_max = 50;
int game_num_stargate_cost = 3000;
int game_num_weapon_list_max = 30;
int game_num_limit_ships = 32000;
int game_num_limit_ships_all = 32000;
int game_num_max_pop = 300;
int game_num_max_factories = 2500;
int game_num_max_inbound = 300;
int game_num_atmos_cost = 200;
int game_num_soil_cost = 150;
int game_num_adv_soil_cost = 300;
int game_num_adv_scan_range = 110;
int game_num_pop_hp = 200;
int game_num_fact_hp = 50;
int game_num_max_bomb_dmg = 100000;
int game_num_max_bio_dmg = 10000;
int game_num_max_trans_dmg = 32000;
int game_num_event_roll = 512;

uint8_t game_num_tbl_hull_w[4] = { 1, 5, 25, 125 };

uint16_t game_num_base_hp[BASE_HP_TBL_NUM] = { 50, 75, 100, 125, 150, 175, 200 };
uint16_t game_num_pshield_cost[PSHIELD_NUM] = { 0, 500, 1000, 1500, 2000 };

uint8_t game_num_tech_costmuld[DIFFICULTY_NUM] = { 20, 25, 30, 35, 40 };

uint8_t game_num_tech_costmulr[RACE_NUM][TECH_FIELD_NUM] = {
    { 100, 100, 60, 80, 80, 100 },
    { 100, 125, 100, 100, 100, 60 },
    { 80, 125, 125, 125, 125, 125 },
    { 100, 100, 100, 60, 100, 100 },
    { 80, 80, 80, 80, 80, 80 },
    { 100, 100, 125, 100, 60, 100 },
    { 100, 60, 100, 100, 125, 100 },
    { 125, 80, 100, 100, 100, 80 },
    { 60, 100, 100, 125, 100, 100 },
    { 80, 100, 100, 100, 100, 100 }
};

uint8_t game_num_tbl_trait1[RACE_NUM][TRAIT1_TBL_NUM] = {
    { 4, 4, 4, 4, 4, 4, 4, 5, 5, 3 },
    { 1, 1, 1, 1, 1, 1, 1, 2, 2, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 2, 2, 3 },
    { 2, 2, 2, 2, 2, 2, 2, 3, 3, 1 },
    { 5, 5, 5, 5, 5, 5, 5, 4, 3, 3 },
    { 4, 4, 4, 4, 4, 4, 4, 5, 5, 3 },
    { 0, 0, 0, 0, 0, 0, 0, 2, 2, 1 },
    { 2, 2, 2, 2, 2, 2, 2, 3, 3, 1 },
    { 3, 3, 3, 3, 3, 3, 3, 2, 2, 0 },
    { 2, 2, 2, 2, 2, 2, 2, 1, 1, 0 }
};

uint8_t game_num_tbl_trait2[RACE_NUM][TRAIT2_TBL_NUM] = {
    { 0, 0, 0, 0, 1, 2, 3, 4, 4, 0, 3, 5 },
    { 0, 1, 1, 1, 1, 3, 2, 2, 3, 3, 4, 5 },
    { 0, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5 },
    { 0, 3, 1, 5, 5, 3, 4, 2, 2, 2, 2, 2 },
    { 0, 0, 3, 1, 2, 3, 3, 3, 3, 4, 4, 5 },
    { 0, 1, 1, 1, 2, 2, 2, 3, 4, 2, 3, 5 },
    { 0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 5, 5 },
    { 0, 5, 5, 5, 5, 2, 2, 3, 3, 3, 4, 5 },
    { 0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5 },
    { 0, 0, 3, 3, 1, 2, 0, 3, 3, 4, 4, 5 }
};

uint8_t game_num_tbl_tech_autoadj[4] = { 0, 25, 50, 75 };

/* -------------------------------------------------------------------------- */

static const struct numtbl_s *find_match(const char *numid, int i, int num)
{
    const struct numtbl_s *s = &game_num_id_tbl[0];
    while (s->numid) {
        if (strcmp(s->numid, numid) == 0) {
            if ((num > 0) && ((i + num) <= s->size)) {
                return s;
            } else {
                log_error("NUM: numid '%s' index %i+%i=%i > size %i\n", numid, i, num, i + num, s->size);
                return NULL;
            }
        }
        ++s;
    }
    log_error("NUM: unknown numid '%s'\n", numid);
    return NULL;
}

/* -------------------------------------------------------------------------- */


bool game_num_patch(const char *numid, const int32_t *patchnums, int first, int num)
{
    const struct numtbl_s *s = find_match(numid, first, num);
    if (s) {
        void *p;
        bool is_signed;
        p = s->ptr + first * s->tstep;
        is_signed = NUMTYPE_IS_S(s->numtype);
        for (int i = 0; i < num; ++i) {
            int32_t v;
            v = patchnums[i];
            if (0
              || ((is_signed) && ((v < s->vmin) || (v > s->vmax)))
              || ((!is_signed) && ((((uint32_t)v) < s->vmin) || (((uint32_t)v) > s->vmax)))
            ) {
                log_error("NUM: numid '%s' %ssigned value %i (%u) outside range %i..%u\n", numid, is_signed ? "" : "un", v, (uint32_t)v, s->vmin, s->vmax);
                return false;
            }
            switch (s->numtype) {
                default:
                case NUMTYPE_S: *((int *)p) = v; break;
                case NUMTYPE_U: *((unsigned int *)p) = (unsigned int)v; break;
                case NUMTYPE_S8: *((int8_t *)p) = (int8_t)v; break;
                case NUMTYPE_U8: *((uint8_t *)p) = (uint8_t)v; break;
                case NUMTYPE_S16: *((int16_t *)p) = (int16_t)v; break;
                case NUMTYPE_U16: *((uint16_t *)p) = (uint16_t)v; break;
                case NUMTYPE_S32: *((int32_t *)p) = v; break;
                case NUMTYPE_U32: *((uint32_t *)p) = (uint32_t)v; break;
                case NUMTYPE_BOOL: *((bool *)p) = (v != 0); break;
            }
            p += s->tstep;
        }
        return true;
    } else {
        return false;
    }
}

void game_num_dump(void)
{
    const struct numtbl_s *s = &game_num_id_tbl[0];
    int prevmin = 1, prevmax = 0;
    log_message("# dump of all patchable game numbers\n");
    while (s->numid) {
        void *p;
        p = s->ptr;
        if ((s->vmin != prevmin) || (s->vmax != prevmax)) {
            prevmin = s->vmin;
            prevmax = s->vmax;
            log_message("# %i..%u\n", prevmin, prevmax);
        }
        log_message("4,%s,0", s->numid);
        for (int i = 0; i < s->size; ++i) {
            int vs = 0;
            unsigned int vu = 0;
            switch (s->numtype) {
                default:
                case NUMTYPE_S: vs = *((int *)p); break;
                case NUMTYPE_U: vu = *((unsigned int *)p); break;
                case NUMTYPE_S8: vs = *((int8_t *)p); break;
                case NUMTYPE_U8: vu = *((uint8_t *)p); break;
                case NUMTYPE_S16: vs = *((int16_t *)p); break;
                case NUMTYPE_U16: vu = *((uint16_t *)p); break;
                case NUMTYPE_S32: vs = *((int32_t *)p); break;
                case NUMTYPE_U32: vu = *((uint32_t *)p); break;
                case NUMTYPE_BOOL: vs = *((bool *)p); break;
            }
            if (NUMTYPE_IS_S(s->numtype)) {
                log_message(",%i", vs);
            } else {
                log_message(",%u", vu);
            }
            p += s->tstep;
        }
        log_message("\n");
        ++s;
    }
}
