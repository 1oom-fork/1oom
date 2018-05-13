#include "config.h"

#include <stdio.h>
#include <string.h>

#include "uiraces.h"
#include "boolvec.h"
#include "game.h"
#include "game_audience.h"
#include "game_diplo.h"
#include "game_str.h"
#include "uidefs.h"
#include "uiinput.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

int ui_cmd_audience(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    struct input_list_s rl_in[] = {
        { -1, "1", NULL, NULL },
        { -1, "2", NULL, NULL },
        { -1, "3", NULL, NULL },
        { -1, "4", NULL, NULL },
        { -1, "5", NULL, NULL },
        { 0, NULL, NULL, NULL },
        { 0, NULL, NULL, NULL }
    };
    int num = 0;
    if (g->end != GAME_END_NONE) {
        return 0;
    }
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        if (IN_CONTACT(g, api, pi) && IS_ALIVE(g, pi)) {
            if (!game_diplo_is_gone(g, api, pi)) {
                rl_in[num].value = pi;
                rl_in[num].display = game_str_tbl_race[g->eto[pi].race];
                ++num;
            }
        }
    }
    if (num > 0) {
        int v;
        rl_in[num].value = -1;
        rl_in[num].key = "Q";
        rl_in[num].str = "q";
        rl_in[num].display = "(quit)";
        ++num;
        rl_in[num].value = 0;
        rl_in[num].key = NULL;
        rl_in[num].str = NULL;
        rl_in[num].display = NULL;
        v = ui_input_list("Choose race to have an audience with", "> ", rl_in);
        if (v >= 0) {
            game_audience(g, api, v);
        }
    } else {
        printf("No one to have an audience with\n");
    }
    return 0;
}
