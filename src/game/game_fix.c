#include "config.h"

#include "game_fix.h"

/* -------------------------------------------------------------------------- */

bool game_fix_broken_garbage = false;
bool game_fix_bt_min_missile_hit = false;
bool game_fix_dead_ai_designs_ships = false;
bool game_fix_guardian_repair = false;
bool game_fix_max_factories = false;
bool game_fix_oracle_interface = false;
bool game_fix_orbital_weap_any = false;
bool game_fix_orbital_torpedo = false;
bool game_fix_orbital_comp = false;
bool game_fix_sg_finished = false;
bool game_fix_sg_maint_overflow = false;
bool game_fix_silicoid_tech = false;
bool game_fix_space_scanners = false;
bool game_fix_spy_cost = false;
bool game_ai_fix_4th_colony_curse = false;
bool game_ai_fix_cancelled_threat = false;
bool game_ai_fix_final_war_fronts = false;
bool game_ai_fix_first_tech_cost = false;
bool game_ai_fix_ship_slider = false;
bool game_ai_fix_spy_hiding = false;
bool game_ai_fix_transport_range = false;

void game_enable_unofficial_1_3a(void)
{
    game_fix_broken_garbage = true;
    game_fix_bt_min_missile_hit = true;
    game_fix_dead_ai_designs_ships = true;
    game_fix_guardian_repair = true;
    game_fix_max_factories = true;
    game_fix_oracle_interface = true;
    game_fix_orbital_torpedo = true;
    game_fix_orbital_weap_any = true;
    game_fix_sg_finished = true;
    game_fix_sg_maint_overflow = true;
    game_fix_space_scanners = true;
    game_ai_fix_4th_colony_curse = true;
    game_ai_fix_cancelled_threat = true;
    game_ai_fix_final_war_fronts = true;
    game_ai_fix_spy_hiding = true;
}

void game_enable_fix_bugs(void)
{
    game_fix_orbital_comp = true;
    game_fix_silicoid_tech = true;
    game_fix_spy_cost = true;
}

void game_enable_fair_ai(void)
{
    game_ai_fix_first_tech_cost = true;
    game_ai_fix_ship_slider = true;
    game_ai_fix_transport_range = true;
}
