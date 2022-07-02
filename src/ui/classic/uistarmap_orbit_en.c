#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_save.h"
#include "game_str.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uicursor.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uiobj.h"
#include "uisound.h"
#include "uistarmap_common.h"

/* -------------------------------------------------------------------------- */

static void ui_starmap_orbit_en_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *p = &g->planet[d->from];
    const empiretechorbit_t *e = &(g->eto[d->oe.player]);
    char buf[0x80];
    STARMAP_LIM_INIT();

    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    {
        int x, y;
        x = (p->x - ui_data.starmap.x) * 2 + 23;
        y = (p->y - ui_data.starmap.y) * 2 + 5 + d->oe.yoff;
        lbxgfx_draw_frame_offs(x, y, ui_data.gfx.starmap.shipbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    }
    ui_draw_filled_rect(225, 8, 314, 180, 7, ui_scale);
    lbxgfx_draw_frame(224, 4, ui_data.gfx.starmap.movextr2, UI_SCREEN_W, ui_scale);
    ui_draw_filled_rect(227, 8, 310, 39, 0, ui_scale);
    lbxgfx_set_frame_0(ui_data.gfx.starmap.scanner);
    for (int f = 0; f <= d->oe.frame_scanner; ++f) {
        lbxgfx_draw_frame(227, 8, ui_data.gfx.starmap.scanner, UI_SCREEN_W, ui_scale);
    }
    lib_sprintf(buf, sizeof(buf), "%s %s", game_str_tbl_race[e->race], game_str_sm_fleet);
    lbxfont_select_set_12_4(5, tbl_banner_fontparam[e->banner], 0, 0);
    lbxfont_print_str_center(267, 10, buf, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_4(0, 0, 0, 0);
    lbxfont_print_str_center(268, 33, game_str_sm_inorbit, UI_SCREEN_W, ui_scale);
    for (int i = 0; i < d->oe.sn0.num; ++i) {
        const shipdesign_t *sd = &(g->srd[d->oe.player].design[0]);
        struct draw_stars_s ds;
        uint8_t *gfx;
        int st, x, y;
        x = 228 + (i % 2) * 43;
        y = 44 + (i / 2) * 40;
        ui_draw_filled_rect(x, y, x + 38, y + 24, 0, ui_scale);
        ui_draw_filled_rect(x, y + 28, x + 38, y + 34, 0x1c, ui_scale);
        ds.xoff1 = 0;
        ds.xoff2 = 0;
        ui_draw_stars(x, y, 0, 38, &ds, ui_scale);
        st = d->oe.sn0.type[i];
        gfx = ui_data.gfx.ships[sd[st].look];
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame(x, y, gfx, UI_SCREEN_W, ui_scale);
        lbxfont_select(0, 0xd, 0, 0);
        lbxfont_print_num_right(x + 35, y + 19, d->oe.ships[st], UI_SCREEN_W, ui_scale);
        lbxfont_select(2, 0xa, 0, 0);
        lbxfont_print_str_center(x + 19, y + 29, sd[st].name, UI_SCREEN_W, ui_scale);
    }
    if (d->oe.scanner_delay == 0) {
        d->oe.frame_scanner = (d->oe.frame_scanner + 1) % 20;
        ++d->oe.scanner_delay;
    } else {
        d->oe.scanner_delay = 0;
    }
}

/* -------------------------------------------------------------------------- */

void ui_starmap_orbit_en(struct game_s *g, player_id_t active_player)
{
    struct starmap_data_s d;
    d.flag_done = false;
    shipcount_t *os;

    d.scrollx = 0;
    d.scrolly = 0;
    d.scrollz = starmap_scale;
    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;
    d.gov_highlight = 0;
    d.oe.frame_scanner = 0;
    d.oe.scanner_delay = 0;
    d.from = g->planet_focus_i[active_player];

    d.controllable = false;
    d.is_valid_destination = NULL;
    d.do_accept = NULL;

    d.oe.player = ui_data.starmap.orbit_player;
    os = &(g->eto[d.oe.player].orbit[d.from].ships[0]);
    for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
        d.oe.ships[i] = os[i];
    }
    ui_starmap_sn0_setup(&d.oe.sn0, NUM_SHIPDESIGNS, d.oe.ships);
    {
        int n = 0;
        for (player_id_t i = PLAYER_0; i < d.oe.player; ++i) {
            os = &(g->eto[i].orbit[d.from].ships[0]);
            for (int j = 0; j < g->eto[i].shipdesigns_num; ++j) {
                if (os[j] != 0) {
                    ++n;
                    break;
                }
            }
        }
        d.oe.yoff = n * 6;
    }

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_callback_and_delay(ui_starmap_orbit_en_draw_cb, &d, STARMAP_DELAY);

    while (!d.flag_done) {
        ui_delay_prepare();
        ui_starmap_handle_common(g, &d);
        if (!d.flag_done) {
            ui_starmap_select_bottom_highlight(g, &d);
            ui_starmap_orbit_en_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            ui_starmap_fill_oi_common(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_table_clear();
}
