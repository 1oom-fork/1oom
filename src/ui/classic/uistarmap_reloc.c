#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_str.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uiobj.h"
#include "uisound.h"
#include "uistarmap_common.h"

/* -------------------------------------------------------------------------- */

static bool ui_starmap_reloc_valid_destination(const struct game_s *g, const struct starmap_data_s *d, int planet_i) {
    const planet_t *p = &g->planet[planet_i];
    return (p->within_frange[d->api] == 1) && g->planet[d->from].buildship != BUILDSHIP_STARGATE;
}

static void ui_starmap_reloc_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *pf = &g->planet[d->from];
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    char buf[0x40];
    int x0, y0;
    STARMAP_LIM_INIT();
    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    x0 = (pf->x - ui_data.starmap.x) * 2 + 8;
    y0 = (pf->y - ui_data.starmap.y) * 2 + 8;
    if (g->planet_focus_i[d->api] != d->from) {
        const uint8_t *ctbl;
        int x1, y1;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 14;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 14;
        if (!ui_starmap_reloc_valid_destination(g, d, g->planet_focus_i[d->api])) {
            ctbl = colortbl_line_red;
        } else {
            ctbl = colortbl_line_green;
        }
        ui_draw_line_limit_ctbl(x0 + 6, y0 + 6, x1, y1, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
    }
    lbxgfx_draw_frame_offs(x0, y0, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    lbxgfx_draw_frame(222, 80, ui_data.gfx.starmap.relocate, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_1(5, 5, 0, 0);
    lbxfont_print_str_center(269, 90, game_str_sm_sreloc, UI_SCREEN_W, ui_scale);
    lbxfont_select(0, 6, 0, 0);
    lbxfont_print_str_split(229, 105, 80, game_str_sm_sreloc2, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
    if (g->planet_focus_i[d->api] != d->from) {
        if (pf->have_stargate && pt->have_stargate && pf->owner == pt->owner) {
            lib_strcpy(buf, game_str_sm_stargate, sizeof(buf));
        } else {
            int eta;
            eta = game_calc_eta_ship(g, g->srd[d->api].design[pf->buildship].engine + 1, pf->x, pf->y, pt->x, pt->y);
            lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_delay, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
        }
        lbxfont_select(0, 0, 0, 0);
        lbxfont_print_str_center(268, 149, buf, UI_SCREEN_W, ui_scale);
    }
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
    lbxgfx_draw_frame(271, 163, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W, ui_scale);
}

/* -------------------------------------------------------------------------- */

static void ui_starmap_reloc_do_accept(struct game_s *g, struct starmap_data_s *d) {
    g->planet[d->from].reloc = g->planet_focus_i[d->api];
}

void ui_starmap_reloc(struct game_s *g, player_id_t active_player)
{
    struct starmap_data_s d;
    ui_starmap_init_common_data(g, &d, active_player);

    {
        uint8_t pi = g->planet_focus_i[active_player];
        g->planet_focus_i[active_player] = g->planet[pi].reloc;
        if (g->planet[g->planet[pi].reloc].owner != active_player) {
            g->planet_focus_i[active_player] = pi;
        }
    }

    d.can_select_fleet = false;
    d.controllable = true;
    d.draw_own_routes = true;
    d.is_valid_selection = ui_starmap_reloc_valid_destination;
    d.do_accept = ui_starmap_reloc_do_accept;

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(2);
    uiobj_set_callback_and_delay(ui_starmap_reloc_draw_cb, &d, STARMAP_DELAY);

    while (!d.flag_done) {
        ui_delay_prepare();
        ui_starmap_handle_common(g, &d);
        if (!d.flag_done) {
            ui_starmap_select_bottom_highlight(g, &d);
            ui_starmap_reloc_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            ui_starmap_fill_oi_common(&d);
            d.oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            if (ui_starmap_reloc_valid_destination(g, &d, g->planet_focus_i[active_player])) {
                d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.from;
}
