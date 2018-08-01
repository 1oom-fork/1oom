#include "config.h"

#include <stdio.h>

#include "uimsgfilter.h"
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
#include "uisound.h"

/* -------------------------------------------------------------------------- */

struct msgfilter_data_s {
    uint8_t filter;
};

static void msgfilter_draw_cb(void *vptr)
{
    struct msgfilter_data_s *d = vptr;
    const int x = 56, y = 10;
    int y0 = y + 15;
    ui_draw_filled_rect(x, y, x + 100, y + 80, 0x06, ui_scale);
    lbxfont_select(0, 0xd, 0, 0);
    lbxfont_print_str_normal(x + 10, y + 5, game_str_mf_title, UI_SCREEN_W, ui_scale);
    lbxfont_select(0, 0, 0, 0);
    for (int i = 0; i < FINISHED_NUM; ++i) {
        if (i != FINISHED_SHIP) {
            ui_draw_filled_rect(x + 10, y0 + 1, x + 13, y0 + 4, 0x01, ui_scale);
            if (d->filter & (1 << i)) {
                ui_draw_filled_rect(x + 11, y0 + 2, x + 12, y0 + 3, 0x44, ui_scale);
            }
            lbxfont_print_str_normal(x + 16, y0, game_str_tbl_mf[i], UI_SCREEN_W, ui_scale);
            y0 += 8;
        }
    }
}

/* -------------------------------------------------------------------------- */

void ui_msg_filter(struct game_s *g, player_id_t pi)
{
    struct msgfilter_data_s d;
    bool flag_done = false;
    int16_t oi_cancel, oi_accept, oi_msg[FINISHED_NUM];
    const int x = 56, y = 10;

    ui_draw_copy_buf();
    uiobj_finish_frame();
    d.filter = g->evn.msg_filter[pi][0];
    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);

    uiobj_table_clear();
    {
        const mookey_t hotkey[FINISHED_NUM] = { MOO_KEY_f, MOO_KEY_p, MOO_KEY_g, MOO_KEY_s, MOO_KEY_h, MOO_KEY_UNKNOWN, MOO_KEY_t };
        int y0 = y + 15;
        for (int i = 0; i < FINISHED_NUM; ++i) {
            if (i != FINISHED_SHIP) {
                oi_msg[i] = uiobj_add_mousearea(x, y0, x + 100, y0 + 7, hotkey[i]);
                y0 += 8;
            } else {
                oi_msg[i] = UIOBJI_INVALID;
            }
        }
    }
    oi_cancel = uiobj_add_t0(x + 10, y + 64, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
    oi_accept = uiobj_add_t0(x + 56, y + 64, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);

    uiobj_set_callback_and_delay(msgfilter_draw_cb, &d, 1);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == oi_cancel) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
        } else if (oi == oi_accept) {
            ui_sound_play_sfx_24();
            flag_done = true;
            g->evn.msg_filter[pi][0] = d.filter;
        }
        for (int i = 0; i < FINISHED_NUM; ++i) {
            if (oi == oi_msg[i]) {
                d.filter ^= (1 << i);
                ui_sound_play_sfx_24();
                break;
            }
        }
        if (!flag_done) {
            msgfilter_draw_cb(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(1);
        }
    }

    uiobj_unset_callback();
    uiobj_table_clear();
}
