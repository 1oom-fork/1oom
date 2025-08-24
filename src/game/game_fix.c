#include "config.h"

#include "game_fix.h"

/* -------------------------------------------------------------------------- */

bool game_fix_orbital_weap_any = false;
bool game_fix_orbital_weap_4 = false;
bool game_fix_orbital_torpedo = false;
bool game_fix_orbital_comp = false;
bool game_fix_space_scanners = false;

void game_enable_fix_bugs(void)
{
    game_fix_orbital_weap_any = true;
    game_fix_orbital_weap_4 = true;
    game_fix_orbital_torpedo = true;
    game_fix_orbital_comp = true;
    game_fix_space_scanners = true;
}
