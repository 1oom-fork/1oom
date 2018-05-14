#include "config.h"

#include "ui.h"
#include "save.h"
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
#include "uiload.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "vgabuf.h"

/* -------------------------------------------------------------------------- */

struct load_game_data_s {
    int selected;
    int savenum;
    int tbl_savei[NUM_SAVES];
    uint8_t *gfx_loadgame;
    uint8_t *gfx_lg_gray;
    uint8_t *gfx_lg_green;
};

static void load_lg_data(struct load_game_data_s *d)
{
    d->gfx_loadgame = lbxfile_item_get(LBXFILE_VORTEX, 3, 0);
    d->gfx_lg_gray = lbxfile_item_get(LBXFILE_VORTEX, 7, 0);
    d->gfx_lg_green = lbxfile_item_get(LBXFILE_VORTEX, 8, 0);
}

static void free_lg_data(struct load_game_data_s *d)
{
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_loadgame);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_lg_gray);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_lg_green);
}

static void load_game_draw_cb(void *vptr)
{
    struct load_game_data_s *d = vptr;
    const int xoff = 0x76;
    const int yoff = 0xa;
    vgabuf_copy_back_from_page2();
    lbxgfx_draw_frame(0, 0, d->gfx_loadgame, UI_SCREEN_W);
    for (int i = 0; i < d->savenum; ++i) {
        int si;
        si = d->tbl_savei[i];
        lbxgfx_draw_frame(0x10 + xoff, 0x17 + yoff + 0x12 * si, (d->selected == i) ? d->gfx_lg_green : d->gfx_lg_gray, UI_SCREEN_W);
        lbxfont_select(0, 1, 0, 0);
        lbxfont_print_str_normal(0x1f + xoff, 0x19 + yoff + 0x12 * si, game_save_tbl_name[si], UI_SCREEN_W);
    }
}

/* -------------------------------------------------------------------------- */

int ui_load_game(void)
{
    struct load_game_data_s d;
    bool flag_done = false, flag_fadein = false;
    const int xoff = 0x76;
    const int yoff = 0xa;
    int16_t oi_esc, oi_cancel, oi_ok, oi_save[NUM_SAVES];

    d.savenum = 0;
    for (int i = 0; i < NUM_SAVES; ++i) {
        if (game_save_tbl_have_save[i]) {
            d.tbl_savei[d.savenum++] = i;
        }
    }

    if (d.savenum == 0) {
        return -1;
    }

    load_lg_data(&d);

    ui_palette_fadeout_19_19_1();
    lbxpal_select(2, -1, 0);

    uiobj_table_clear();
    oi_esc = uiobj_add_inputkey(MOO_KEY_ESCAPE);
    oi_cancel = uiobj_add_mousearea(0x14 + xoff, 0x87 + yoff, 0x4a + xoff, 0x98 + yoff, MOO_KEY_LEFT);
    oi_ok =     uiobj_add_mousearea(0x54 + xoff, 0x87 + yoff, 0x8a + xoff, 0x98 + yoff, MOO_KEY_SPACE);
    uiobj_set_focus(oi_ok);

    for (int i = 0; i < d.savenum; ++i) {
        int y0;
        y0 = 0x15 + yoff + d.tbl_savei[i] * 0x12;
        oi_save[i] = uiobj_add_mousearea(0xc + xoff, y0, 0x92 + xoff, y0 + 0xe, MOO_KEY_UNKNOWN);
    }

    d.selected = 0;

    ui_draw_erase_buf();
    vgabuf_copy_back_to_page2();
    uiobj_set_callback_and_delay(load_game_draw_cb, &d, 2);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        for (int i = 0; i < d.savenum; ++i) {
            if (oi == oi_save[i]) {
                d.selected = i;
                ui_sound_play_sfx_24();
            }
        }
        if ((oi == oi_cancel) || (oi == oi_esc) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx(0x06);
            d.selected = -1;
            flag_done = true;
        }
        if (oi == oi_ok) {
            ui_sound_play_sfx_24();
            flag_done = true;
        }
        load_game_draw_cb(&d);
        uiobj_finish_frame();
        if (!flag_fadein) {
            ui_palette_fadein_4b_19_1();
            flag_fadein = true;
        }
        ui_delay_ticks_or_click(2);
    }

    uiobj_unset_callback();
    free_lg_data(&d);
    return (d.selected >= 0) ? d.tbl_savei[d.selected] : -1;
}
