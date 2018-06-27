#ifndef INC_1OOM_GAME_NUM_H
#define INC_1OOM_GAME_NUM_H

#include "game_types.h"
#include "types.h"

extern int game_num_bt_turn_max;
extern bool game_num_bt_wait_no_reload;
extern bool game_num_bt_precap_tohit;
extern bool game_num_bt_no_tohit_acc;
extern int game_num_stargate_cost;
extern int game_num_weapon_list_max;
extern unsigned int game_num_limit_ships;
extern unsigned int game_num_limit_ships_all;
extern int game_num_max_pop;
extern int game_num_max_factories;
extern int game_num_max_inbound;
extern int game_num_atmos_cost;
extern int game_num_soil_cost;
extern int game_num_adv_soil_cost;
extern int game_num_adv_scan_range;
extern int game_num_pop_hp;
extern int game_num_fact_hp;
extern int game_num_max_bomb_dmg;
extern int game_num_max_bio_dmg;
extern int game_num_max_trans_dmg;
extern int game_num_max_ship_maint;
extern int game_num_event_roll;
extern int game_num_council_years;
extern int game_num_eco_slider_slack;
extern int game_num_race_bonus_alkari;
extern int game_num_race_bonus_bulrathi;
extern int game_num_race_bonus_mrrshan;
extern bool game_num_news_orion;
extern bool game_num_aud_bounty_give;
extern uint8_t game_num_tbl_hull_w[4];
extern uint8_t game_num_tech_costmuld[DIFFICULTY_NUM];
extern uint8_t game_num_tech_costmulr[RACE_NUM][TECH_FIELD_NUM];
#define BASE_HP_TBL_NUM 7
extern uint16_t game_num_base_hp[BASE_HP_TBL_NUM];
#define TRAIT1_TBL_NUM 10
#define TRAIT2_TBL_NUM 12
extern uint8_t game_num_tbl_trait1[RACE_NUM][TRAIT1_TBL_NUM];
extern uint8_t game_num_tbl_trait2[RACE_NUM][TRAIT2_TBL_NUM];
#define PSHIELD_NUM 5
extern uint16_t game_num_pshield_cost[PSHIELD_NUM];
extern uint8_t game_num_tbl_tech_autoadj[4];

#endif
