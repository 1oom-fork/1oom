#include "config.h"

#include <stdio.h>

#include "uiswitch.h"
#include "comp.h"
#include "game.h"
#include "game_str.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "types.h"
#include "ui.h"
#include "uicursor.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"
#include "uisound.h"

/* -------------------------------------------------------------------------- */

struct switch_data_s {
    struct game_s *g;
    player_id_t tbl_pi[PLAYER_NUM];
    int num_pi;
    int bottom_highlight;
    bool allow_opts;
};

static void switch_draw_cb(void *vptr)
{
    struct switch_data_s *d = vptr;
    struct game_s *g = d->g;
    void *ctx;
    char buf[32];

    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, ui_data.gfx.starmap.mainview, UI_SCREEN_W);

    ctx = ui_gmap_basic_init(g, true);
    ui_gmap_basic_draw_frame(ctx, -1);

    lbxfont_select(3, 2, 0, 0);
    lbxfont_print_num_normal(230, 16, g->year + YEAR_BASE, UI_SCREEN_W);

    for (int i = 0; i < d->num_pi; ++i) {
        empiretechorbit_t *e;
        player_id_t pi;
        uint8_t color;
        pi = d->tbl_pi[i];
        e = &(g->eto[pi]);
        color = tbl_banner_fontparam[e->banner];
        lbxfont_select(3, color, 0, 0);
        sprintf(buf, "%s %i", game_str_player, pi + 1);
        lbxfont_print_str_normal(230, i * 24 + 32, buf, UI_SCREEN_W);
        lbxfont_select(0, color, 0, 0);
        lbxfont_print_str_normal(230, i * 24 + 32 + 10, g->emperor_names[pi], UI_SCREEN_W);
    }

    if (d->allow_opts) {
        lbxfont_select_set_12_4(5, (d->bottom_highlight == 0) ? 0 : 2, 0, 0);
        lbxfont_print_str_normal(10, 184, game_str_sm_game, UI_SCREEN_W);
    }

    ui_gmap_basic_shutdown(ctx);
}

/* -------------------------------------------------------------------------- */

bool ui_switch(struct game_s *g, player_id_t *tbl_pi, int num_pi, bool allow_opts)
{
    struct switch_data_s d;
    bool flag_done = false, flag_opts = false;
    int16_t oi_ma = UIOBJI_INVALID, oi_gameopts = UIOBJI_INVALID;

    d.g = g;
    d.num_pi = num_pi;
    d.bottom_highlight = -1;
    d.allow_opts = allow_opts;
    for (int i = 0; i < num_pi; ++i) {
        d.tbl_pi[i] = tbl_pi[i];
    }

    ui_delay_1();
    ui_sound_stop_music();  /* or fade? */
    uiobj_set_downcount(1);

    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);
    uiobj_table_clear();
    uiobj_set_callback_and_delay(switch_draw_cb, &d, 1);

    while (!flag_done) {
        int16_t oi1, oi2;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if ((oi1 == oi_ma) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_24();
            flag_done = true;
        } else if (oi1 == oi_gameopts) {
            ui_sound_play_sfx_24();
            flag_done = true;
            flag_opts = true;
        }
        d.bottom_highlight = -1;
        if (oi2 == oi_gameopts) {
            d.bottom_highlight = 0;
        }
        if (!flag_done) {
            switch_draw_cb(&d);
            uiobj_table_clear();
            if (allow_opts) {
                oi_gameopts = uiobj_add_mousearea(5, 181, 36, 194, MOO_KEY_g);
            }
            oi_ma = uiobj_add_mousearea_all(MOO_KEY_SPACE);
            ui_draw_finish();
            ui_delay_ticks_or_click(1);
        }
    }

    uiobj_unset_callback();
    return flag_opts;
}
