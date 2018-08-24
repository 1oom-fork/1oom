#include "config.h"

#include <stdio.h>

#include "game_turn_start.h"
#include "boolvec.h"
#include "game.h"
#include "game_str.h"
#include "types.h"
#include "ui.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

void game_turn_start_messages(struct game_s *g, player_id_t pi)
{
    char *buf = ui_get_strbuf();
    {
        empiretechorbit_t *e = &(g->eto[pi]);
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            if (BOOLVEC_IS1(e->contact_broken, i)) {
                BOOLVEC_SET0(e->contact_broken, i);
                sprintf(buf, "%s %s %s", game_str_tr_cont1, game_str_tbl_race[g->eto[i].race], game_str_tr_cont2);
                ui_turn_msg(g, pi, buf);
            }
        }
    }
    for (uint8_t pli = 0; pli < g->galaxy_stars; ++pli) {
        planet_t *p = &(g->planet[pli]);
        if (BOOLVEC_IS1(p->unrefuel, pi)) {
            char *b, *q;
            BOOLVEC_SET0(p->unrefuel, pi);
            b = buf;
            b += sprintf(buf, "%s ", game_str_tr_fuel1);
            q = b;
            b += sprintf(b, "%s", p->name);
            util_str_tolower(q + 1);
            sprintf(b, " %s", game_str_tr_fuel2);
            g->planet_focus_i[pi] = pli;
            ui_turn_msg(g, pi, buf);
        }
    }
    {
        bool any_new;
        any_new = false;
        for (int si = 0; si < NUM_SHIPDESIGNS; ++si) {
            if (g->evn.new_ships[pi][si] != 0) {
                any_new = true;
                break;
            }
        }
        if (any_new) {
            ui_newships(g, pi);
        }
        for (int si = 0; si < NUM_SHIPDESIGNS; ++si) {
            g->evn.new_ships[pi][si] = 0;
        }
    }
}
