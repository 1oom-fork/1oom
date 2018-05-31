#include "config.h"

#include <stdio.h>

#include "uiempirestatus.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_stat.h"
#include "game_str.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "types.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"

/* -------------------------------------------------------------------------- */

struct empirestatus_data_s {
    struct game_s *g;
    uint8_t *gfx;
    player_id_t api;
    int num;
    uint8_t tbl_ei[PLAYER_NUM];
};

static void empirestatus_data_load(struct empirestatus_data_s *d)
{
    d->gfx = lbxfile_item_get(LBXFILE_BACKGRND, 2, 0);
}

static void empirestatus_data_free(struct empirestatus_data_s *d)
{
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx);
}

static void empirestatus_draw_cb(void *vptr)
{
    struct empirestatus_data_s *d = vptr;
    struct game_s *g = d->g;
    char buf[0x40];
    uint8_t tbl_stat[6][PLAYER_NUM];
    int tbl_sum[PLAYER_NUM];

    ui_draw_color_buf(0x3a);
    lbxgfx_draw_frame(0, 0, d->gfx, UI_SCREEN_W);

    lbxfont_select_set_12_4(4, 0xf, 0, 0);
    lbxfont_print_str_center(160, 9, game_str_ra_stats, UI_SCREEN_W);
    lbxfont_select_set_12_4(5, 5, 0, 0);
    sprintf(buf, "%s: %i", game_str_year, g->year + YEAR_BASE);
    lbxfont_print_str_normal(15, 11, buf, UI_SCREEN_W);

    for (int i = 0; i < d->num; ++i) {
        tbl_sum[i] = 0;
    }
    for (int s = 0; s < 5; ++s) {
        for (int i = 0; i < d->num; ++i) {
            player_id_t pi;
            int v;
            pi = d->tbl_ei[i];
            switch (s) {
                case 0:
                    v = game_stat_fleet(g, pi);
                    break;
                case 1:
                    v = game_stat_tech(g, pi) / 3;
                    break;
                case 2:
                    v = game_stat_prod(g, pi);
                    break;
                case 3:
                    v = game_stat_pop(g, pi);
                    break;
                case 4:
                    v = game_stat_planets(g, pi);
                    break;
            }
            SETRANGE(v, 0, 100);
            tbl_stat[s][i] = v;
            tbl_sum[i] += v;
        }
    }
    {
        int maxstats = 0;
        for (int i = 0; i < d->num; ++i) {
            SETMAX(maxstats, tbl_sum[i]);
        }
        for (int i = 0; i < d->num; ++i) {
            tbl_stat[5][i] = maxstats ? ((tbl_sum[i] * 100) / maxstats) : 100;
        }
    }
    lbxfont_select(2, 6, 0, 0);
    for (int s = 0; s < 6; ++s) {
        for (int i = 0; i < d->num; ++i) {
            player_id_t pi;
            const empiretechorbit_t *e;
            int x, y;
            uint8_t v;
            x = (s / 3) * 156 + 11;
            y = (s % 3) * 57 + i * 7 + 38;
            pi = d->tbl_ei[i];
            e = (&g->eto[pi]);
            lbxfont_print_str_normal(x, y, game_str_tbl_race[e->race], UI_SCREEN_W);
            v = tbl_stat[s][i];
            if (v) {
                ui_draw_filled_rect(x + 35, y + 1, x + 34 + v, y + 2, tbl_banner_color2[e->banner]);
                if (v > 1) {
                    ui_draw_line1(x + 35, y + 3, x + 33 + v, y + 3, 0);
                }
            }
        }
    }
}

/* -------------------------------------------------------------------------- */

void ui_empirestatus(struct game_s *g, player_id_t active_player)
{
    struct empirestatus_data_s d;
    bool flag_done = false;

    empirestatus_data_load(&d);
    d.g = g;
    d.api = active_player;

    game_update_production(g);
    game_update_empire_within_range(g);
    game_update_maint_costs(g);

    d.num = 1;
    d.tbl_ei[0] = active_player;
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        if ((pi != active_player) && BOOLVEC_IS1(g->eto[active_player].within_frange, pi)) {
            d.tbl_ei[d.num++] = pi;
        }
    }

    uiobj_table_clear();
    uiobj_set_help_id(15);
    uiobj_set_callback_and_delay(empirestatus_draw_cb, &d, 1);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if (oi != UIOBJI_NONE) {
            flag_done = true;
        }
        if (!flag_done) {
            empirestatus_draw_cb(&d);
            uiobj_table_clear();
            uiobj_add_mousearea(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, MOO_KEY_UNKNOWN, -1);
            ui_draw_finish();
            ui_delay_ticks_or_click(1);
        }
    }

    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    empirestatus_data_free(&d);
}
