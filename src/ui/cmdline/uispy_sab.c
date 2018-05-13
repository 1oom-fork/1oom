#include "config.h"

#include <stdio.h>

#include "ui.h"
#include "game.h"
#include "game_str.h"

/* -------------------------------------------------------------------------- */

ui_sabotage_t ui_spy_sabotage_ask(struct game_s *g, int spy, int target, uint8_t *planetptr)
{
    return UI_SABOTAGE_NONE;
}

int ui_spy_sabotage_done(struct game_s *g, int pi, int spy, int target, ui_sabotage_t act, int other1, int other2, uint8_t planet, int snum)
{
    return PLAYER_NONE;
}
