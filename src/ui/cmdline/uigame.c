#include "config.h"

#include "ui.h"
#include "game.h"
#include "game_aux.h"
#include "lib.h"
#include "log.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

ui_turn_action_t ui_game_turn(struct game_s *g, int *load_game_i_ptr, int pi)
{
    return UI_TURN_ACT_QUIT_GAME;
}

void ui_game_start(struct game_s *g)
{
}

void ui_game_end(struct game_s *g)
{
}

uint8_t *ui_gfx_get_ship(int look)
{
    return 0;
}

uint8_t *ui_gfx_get_planet(int look)
{
    return 0;
}

uint8_t *ui_gfx_get_rock(int look)
{
    return 0;
}
