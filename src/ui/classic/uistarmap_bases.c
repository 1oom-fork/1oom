#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_str.h"
#include "game_tech.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uicursor.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"

/* -------------------------------------------------------------------------- */

struct starmap_bases_data_s {
    struct game_s *g;
    uint8_t *gfx;
    player_id_t api;
    int16_t slider_var;
};

static void ui_starmap_bases_load_data(struct starmap_bases_data_s *d)
{
    d->gfx = lbxfile_item_get(LBXFILE_BACKGRND, 0x1e);
}

static void ui_starmap_bases_free_data(struct starmap_bases_data_s *d)
{
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx);
}

static void ui_starmap_bases_draw_cb1(void *vptr)
{
    struct starmap_bases_data_s *bd = vptr;
    struct game_s *g = bd->g;
    const int x = 56, y = 50;
    const planet_t *p = &(g->planet[g->planet_focus_i[bd->api]]);
    lbxgfx_draw_frame(x, y, bd->gfx, UI_SCREEN_W, ui_scale);
    ui_draw_filled_rect(x + 14, y + 35, x + 64, y + 38, 0x2f, ui_scale);
    if (bd->slider_var > 0) {
        ui_draw_slider(x + 14, y + 36, bd->slider_var, 2, -1, 0x74, ui_scale);
    }
    lbxfont_select(0, 0xd, 0, 0);
    lbxfont_print_str_center(x + 57, y + 11, game_str_bs_line1, UI_SCREEN_W, ui_scale);
    lbxfont_print_str_center(x + 57, y + 20, game_str_bs_line2, UI_SCREEN_W, ui_scale);
    lbxfont_select(2, 6, 0, 0);
    {
        int n = (p->missile_bases * bd->slider_var) / 100;
        lbxfont_print_str_right(x + 104, y + 35, (n == 1) ? game_str_bs_base : game_str_bs_bases, UI_SCREEN_W, ui_scale);
        lbxfont_print_num_right(x + 83, y + 35, n, UI_SCREEN_W, ui_scale);
    }
}

/* -------------------------------------------------------------------------- */

void ui_starmap_bases(struct game_s *g, player_id_t active_player)
{
    struct starmap_bases_data_s bd;
    bool flag_done = false;
    int16_t oi_cancel, oi_accept, oi_plus, oi_minus;
    const int x = 56, y = 50;
    planet_t *p = &(g->planet[g->planet_focus_i[active_player]]);

    bd.g = g;
    bd.api = active_player;
    ui_starmap_bases_load_data(&bd);
    ui_draw_copy_buf();
    uiobj_finish_frame();
    bd.slider_var = 0;
    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);

    uiobj_table_clear();
    oi_cancel = uiobj_add_t0(x + 10, y + 47, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
    oi_accept = uiobj_add_t0(x + 66, y + 47, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
    uiobj_add_slider_int(x + 14, y + 35, 0, 100, 50, 9, &bd.slider_var);
    oi_minus = uiobj_add_mousearea(x + 10, y + 33, x + 12, y + 41, MOO_KEY_UNKNOWN);
    oi_plus = uiobj_add_mousearea(x + 66, y + 33, x + 70, y + 41, MOO_KEY_UNKNOWN);

    uiobj_set_callback_and_delay(ui_starmap_bases_draw_cb1, &bd, 1);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == oi_cancel) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
        } else if (oi == oi_accept) {
            int n;
            ui_sound_play_sfx_24();
            n = (p->missile_bases * bd.slider_var) / 100;
            p->missile_bases -= n;
            g->eto[active_player].reserve_bc += (n * game_get_base_cost(g, active_player)) / 4;
            flag_done = true;
            SETMAX(p->missile_bases, 0);
        } else if (oi == oi_minus) {
            bd.slider_var -= 2;
            SETMAX(bd.slider_var, 0);
        } else if (oi == oi_plus) {
            bd.slider_var += 2;
            SETMIN(bd.slider_var, 100);
        }
        if (!flag_done) {
            ui_starmap_bases_draw_cb1(&bd);
            ui_draw_finish();
            ui_delay_ticks_or_click(3);
        }
    }

    uiobj_unset_callback();
    uiobj_table_clear();
    ui_starmap_bases_free_data(&bd);
}
