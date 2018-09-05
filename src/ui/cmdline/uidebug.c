#include "config.h"

#ifdef FEATURE_MODEBUG

#include "uidebug.h"
#include "game.h"
#include "game_debug.h"
#include "uidefs.h"

/* -------------------------------------------------------------------------- */

static int ui_cmd_debug_cb_g_f(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    void (*cb)(const struct game_s *g, bool force) = var;
    cb(g, true);
    return 0;
}

/* -------------------------------------------------------------------------- */

const struct input_cmd_s ui_cmds_debug[] = {
    { "debug_slider", NULL, "", 0, 0, 0, ui_cmd_debug_cb_g_f, game_debug_dump_sliders },
    { "debug_race_techs", NULL, "", 0, 0, 0, ui_cmd_debug_cb_g_f, game_debug_dump_race_techs },
    { "debug_race_spending", NULL, "", 0, 0, 0, ui_cmd_debug_cb_g_f, game_debug_dump_race_spending },
    { "debug_race_waste", NULL, "", 0, 0, 0, ui_cmd_debug_cb_g_f, game_debug_dump_race_waste },
    { NULL, NULL, NULL, 0, 0, 0, NULL, 0 }
};

#endif /* FEATURE_MODEBUG */
