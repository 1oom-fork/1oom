#include "config.h"

#include <stdio.h>

#include "uifinished.h"
#include "game.h"
#include "game_planet.h"
#include "game_str.h"
#include "ui.h"
#include "uidefs.h"

/* -------------------------------------------------------------------------- */

void ui_finished_print_all(struct game_s *g, int api)
{
    if (g->evn.build_finished_num[api] > 0) {
        g->evn.build_finished_num[api] = 0;
        for (uint8_t pli = 0; pli < g->galaxy_stars; ++pli) {
            planet_t *p = &(g->planet[pli]);
            if ((p->owner == api) && (p->finished[0] & (~(1 << FINISHED_SHIP)))) {
                for (planet_finished_t type = 0; type < FINISHED_NUM; ++type) {
                    if ((type != FINISHED_SHIP) && BOOLVEC_IS1(p->finished, type)) {
                        BOOLVEC_SET0(p->finished, type);
                        printf("- %s\n", game_planet_get_finished_text(g, p, type, ui_data.strbuf, UI_STRBUF_SIZE));
                    }
                }
            }
        }
    }
}

int ui_cmd_msg_filter(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    const char msgchars[FINISHED_NUM] = "fpgsh t";
    if (num_param == 0) {
        puts(game_str_mf_title);
        for (planet_finished_t type = 0; type < FINISHED_NUM; ++type) {
            if (type != FINISHED_SHIP) {
                printf("%c %c %s\n", msgchars[type], BOOLVEC_TBL_IS1(g->evn.msg_filter, api, type) ? '+' : '-', game_str_tbl_mf[type]);
            }
        }
    } else {
        const char *p = param->str;
        char c;
        while ((c = *p++) != '\0') {
            planet_finished_t type;
            const char *q;
            q = strchr(msgchars, c);
            if ((!q) || (c == ' ')) {
                printf("Invalid filter char '%c'\n", c);
                return -1;
            }
            type = q - msgchars;
            BOOLVEC_TBL_TOGGLE(g->evn.msg_filter, api, type);
        }
    }
    return 0;
}
