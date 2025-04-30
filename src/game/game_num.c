#include "config.h"

#include <string.h>

#include "game_num.h"
#include "game_shiptech.h"
#include "lib.h"
#include "log.h"

/* -------------------------------------------------------------------------- */

int game_num_bt_turn_max = 50;
int game_num_stargate_cost = 3000;
int game_num_weapon_list_max = 30;
unsigned int game_num_limit_ships = 32000;
unsigned int game_num_limit_ships_all = 32000;
int game_num_max_pop = 300;
int game_num_max_factories = 2500;
int game_num_max_factories_1_3a = 2700;
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
unsigned int game_num_max_ship_maint = 32000;
int game_num_event_roll = 512;
int game_num_council_years = 25;
int game_num_eco_slider_slack = 7;
int game_num_race_bonus_alkari = 3;
int game_num_race_bonus_bulrathi = 25;
int game_num_race_bonus_mrrshan = 4;

uint8_t game_num_tbl_hull_w[4] = { 1, 5, 25, 125 };

uint16_t game_num_base_hp[BASE_HP_TBL_NUM] = { 50, 75, 100, 125, 150, 175, 200 };
uint16_t game_num_pshield_cost[PSHIELD_NUM] = { 0, 500, 1000, 1500, 2000 };

uint8_t game_num_tech_costmuld[DIFFICULTY_NUM] = { 20, 25, 30, 35, 40 };
uint8_t game_num_tech_costmula[DIFFICULTY_NUM] = { 20, 20, 20, 20, 20 };

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
