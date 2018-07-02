#include "config.h"

#include <stdio.h>

#include "uigovern.h"
#include "comp.h"
#include "game.h"
#include "game_planet.h"
#include "game_str.h"
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

struct govern_data_s {
    uint16_t target;
};

static void govern_draw_cb(void *vptr)
{
    struct govern_data_s *d = vptr;
    {
        const int x = 56, y = 10;
        ui_draw_filled_rect(x, y, x + 115, y + 59, 0x06, ui_scale);
        lbxfont_select(0, 0xd, 0, 0);
        lbxfont_print_str_split(x + 10, y + 5, 105, game_str_gv_target, 0, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        lbxfont_select(2, 6, 0, 0);
        lbxfont_print_num_right(x + 83, y + 28, d->target, UI_SCREEN_W, ui_scale);
    }
    {
        const int x = 56, y = 130;
        ui_draw_filled_rect(x, y, x + 115, y + 40, 0x06, ui_scale);
        lbxfont_select(0, 0xd, 0, 0);
        lbxfont_print_str_split(x + 10, y + 5, 105, game_str_gv_adjust, 0, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
    }
}

/* -------------------------------------------------------------------------- */

void ui_govern(struct game_s *g, player_id_t pi)
{
    struct govern_data_s d;
    bool flag_done = false;
    int16_t oi_cancel, oi_accept, oi_p, oi_m, oi_p10, oi_m10, oi_adjust;
    const int x = 56, y = 10;
    planet_t *p = &(g->planet[g->planet_focus_i[pi]]);

    ui_draw_copy_buf();
    uiobj_finish_frame();
    d.target = p->target_bases;
    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);

    uiobj_table_clear();
    oi_p = uiobj_add_t0(x + 20 + 23, y + 25, "", ui_data.gfx.starmap.move_but_p, MOO_KEY_UNKNOWN);
    oi_m = uiobj_add_t0(x + 20 + 12, y + 25, "", ui_data.gfx.starmap.move_but_m, MOO_KEY_UNKNOWN);
    oi_p10 = uiobj_add_t0(x + 20 + 34, y + 25, "", ui_data.gfx.starmap.move_but_a, MOO_KEY_UNKNOWN);
    oi_m10 = uiobj_add_t0(x + 20, y + 25, "", ui_data.gfx.starmap.move_but_n, MOO_KEY_UNKNOWN);
    oi_cancel = uiobj_add_t0(x + 10, y + 41, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
    oi_accept = uiobj_add_t0(x + 66, y + 41, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
    oi_adjust = uiobj_add_t0(x + 66, y + 145, "", ui_data.gfx.screens.tech_but_ok, MOO_KEY_o);

    uiobj_set_callback_and_delay(govern_draw_cb, &d, 1);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == oi_cancel) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
        } else if (oi == oi_accept) {
            ui_sound_play_sfx_24();
            p->target_bases = d.target;
            flag_done = true;
        } else if (oi == oi_adjust) {
            ui_sound_play_sfx_24();
            game_planet_govern_all_owned_by(g, pi);
            flag_done = true;
        } else if (oi == oi_m) {
            SUBSAT0(d.target, 1);
        } else if (oi == oi_m10) {
            SUBSAT0(d.target, 10);
        } else if (oi == oi_p) {
            ADDSATT(d.target, 1, 0xffff);
        } else if (oi == oi_p10) {
            ADDSATT(d.target, 10, 0xffff);
        }
        if (!flag_done) {
            govern_draw_cb(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(3);
        }
    }

    uiobj_unset_callback();
    uiobj_table_clear();
}
