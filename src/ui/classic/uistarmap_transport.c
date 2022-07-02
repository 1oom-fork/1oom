#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_num.h"
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
#include "util.h"

/* -------------------------------------------------------------------------- */

static bool ui_starmap_transport_within_frange(const struct game_s *g, const struct starmap_data_s *d, int planet_i) {
    const planet_t *p = &g->planet[planet_i];
    if (!game_num_trans_redir_fix) {
        return (p->within_frange[d->api] != 0); /* WASBUG MOO1 allows redirection almost anywhere */
    } else {
        return (p->within_frange[d->api] == 1);
    }
}

static bool ui_starmap_transport_valid_destination(const struct game_s *g, const struct starmap_data_s *d, int planet_i) {
    if (!game_num_trans_redir_fix) {
        return ui_starmap_transport_within_frange(g, d, planet_i);
    }
    const planet_t *p = &g->planet[planet_i];
    return game_transport_dest_ok(g, p, d->api);
}

static void ui_starmap_transport_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const transport_t *r = &(g->transport[ui_data.starmap.fleet_selected]);
    const empiretechorbit_t *e = &(g->eto[r->owner]);
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    bool in_frange = false;
    char buf[0x80];
    STARMAP_LIM_INIT();

    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    {
        int x, y;
        x = (r->x - ui_data.starmap.x) * 2 + 5;
        y = (r->y - ui_data.starmap.y) * 2 + 5;
        lbxgfx_draw_frame_offs(x, y, ui_data.gfx.starmap.shipbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    }
    ui_draw_filled_rect(225, 8, 314, 180, 7, ui_scale);
    lbxgfx_draw_frame(224, 4, ui_data.gfx.starmap.tranbord, UI_SCREEN_W, ui_scale);
    if (d->controllable) {
        lbxgfx_draw_frame(224, 159, ui_data.gfx.starmap.tranxtra, UI_SCREEN_W, ui_scale);
    }
    ui_draw_filled_rect(227, 8, 310, 39, 0, ui_scale);
    lbxgfx_set_frame_0(ui_data.gfx.starmap.scanner);
    for (int f = 0; f <= d->ts.frame_scanner; ++f) {
        lbxgfx_draw_frame(227, 8, ui_data.gfx.starmap.scanner, UI_SCREEN_W, ui_scale);
    }
    lib_sprintf(buf, sizeof(buf), "%s %s", game_str_tbl_race[e->race], game_str_sm_fleet);
    lbxfont_select_set_12_4(5, tbl_banner_fontparam[e->banner], 0, 0);
    lbxfont_print_str_center(267, 10, buf, UI_SCREEN_W, ui_scale);
    if ((r->owner == d->api) || (g->eto[d->api].have_ia_scanner)) {
        const planet_t *pd = &(g->planet[r->dest]);
        uint8_t *gfx;
        int x0, y0, x1, y1, dist;
        bool dest_ok = true;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 8;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 8;
        lbxgfx_draw_frame_offs(x1, y1, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        x0 = (r->x - ui_data.starmap.x) * 2 + 8;
        y0 = (r->y - ui_data.starmap.y) * 2 + 8;
        if (d->controllable) {
            dest_ok = game_transport_dest_ok(g, pt, d->api);
        }
        {
            const uint8_t *ctbl;
            ctbl = dest_ok ? colortbl_line_green : colortbl_line_red;
            ui_draw_line_limit_ctbl(x0 + 4, y0 + 1, x1 + 6, y1 + 6, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
        }
        gfx = ui_data.gfx.starmap.smaltran[e->banner];
        if (pd->x < r->x) {
            lbxgfx_set_new_frame(gfx, 1);
        } else {
            lbxgfx_set_frame_0(gfx);
        }
        lbxgfx_draw_frame_offs(x0, y0, gfx, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        dist = game_get_min_dist(g, r->owner, g->planet_focus_i[d->api]);
        if (d->controllable && !ui_starmap_transport_within_frange(g, d, g->planet_focus_i[d->api])) {
            /* FIXME use proper positioning for varying str length */
            in_frange = false;
            lib_sprintf(buf, sizeof(buf), "  %s   %i %s.", game_str_sm_outsr, dist - e->fuel_range, game_str_sm_parsecs2);
            lbxfont_select_set_12_4(2, 0, 0, 0);
            lbxfont_set_gap_h(2);
            lbxfont_print_str_split(230, 26, 80, buf, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else {
            int eta = game_calc_eta_trans(g, r->speed, pt->x, pt->y, r->x, r->y);
            in_frange = true;
            lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
            lbxfont_select_set_12_4(0, 0, 0, 0);
            lbxfont_print_str_center(268, 32, buf, UI_SCREEN_W, ui_scale);
        }
        if (!dest_ok) {
            in_frange = false;
        }
    } else {
        /*6a51c*/
        in_frange = false;
    }
    {
        int x = 228, y = 73;
        ui_draw_filled_rect(x, y, x + 81, y + 25, 0, ui_scale);
        ui_draw_stars(x, y, 0, 80, &(d->ts.ds), ui_scale);
        lbxgfx_set_frame_0(ui_data.gfx.starmap.tranship);
        for (int f = 0; f <= d->ts.frame_ship; ++f) {
            lbxgfx_draw_frame(x + 7, y + 3, ui_data.gfx.starmap.tranship, UI_SCREEN_W, ui_scale);
        }
    }
    lbxfont_select_set_12_4(0, 5, 0, 0);
    lib_sprintf(buf, sizeof(buf), "%i %s", r->pop, game_str_sm_colony);
    lbxfont_print_str_center(267, 48, buf, UI_SCREEN_W, ui_scale);
    lbxfont_print_str_center(267, 57, (r->pop == 1) ? game_str_sm_trans1 : game_str_sm_transs, UI_SCREEN_W, ui_scale);
    if ((r->owner == d->api) || (g->eto[d->api].have_ia_scanner)) {
        const planet_t *pd = &(g->planet[r->dest]);
        lbxfont_print_str_center(267, 110, game_str_sm_tdest, UI_SCREEN_W, ui_scale);
        lbxfont_print_str_center(267, 120, pd->name, UI_SCREEN_W, ui_scale);
    }
    if (d->ts.scanner_delay == 0) {
        d->ts.frame_scanner = (d->ts.frame_scanner + 1) % 20;
        ++d->ts.scanner_delay;
    } else {
        d->ts.scanner_delay = 0;
    }
    d->ts.frame_ship = (d->ts.frame_ship + 1) % 5;
    if (d->controllable && (!in_frange)) {
        lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
        lbxgfx_draw_frame(271, 163, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W, ui_scale);
    }
    ui_draw_set_stars_xoffs(&d->ts.ds, false);
}

/* -------------------------------------------------------------------------- */

static void ui_starmap_transport_do_accept(struct game_s *g, struct starmap_data_s *d) {
    transport_t *r = &g->transport[ui_data.starmap.fleet_selected];
    r->dest = g->planet_focus_i[d->api];
    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
}

void ui_starmap_transport(struct game_s *g, player_id_t active_player)
{
    struct starmap_data_s d;
    d.flag_done = false;
    transport_t *r = &(g->transport[ui_data.starmap.fleet_selected]);

    d.scrollx = 0;
    d.scrolly = 0;
    d.scrollz = starmap_scale;
    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;
    d.gov_highlight = 0;
    d.ts.frame_ship = 0;
    d.ts.frame_scanner = 0;
    d.ts.scanner_delay = 0;
    d.from = g->planet_focus_i[active_player];
    g->planet_focus_i[active_player] = r->dest;
    d.ts.ds.xoff1 = 0;
    d.ts.ds.xoff2 = 0;

    d.controllable = g->eto[active_player].have_hyperspace_comm && r->owner == active_player;
    d.is_valid_destination = ui_starmap_transport_valid_destination;
    d.do_accept = ui_starmap_transport_do_accept;

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(3);
    uiobj_set_callback_and_delay(ui_starmap_transport_draw_cb, &d, STARMAP_DELAY);

    while (!d.flag_done) {
        ui_delay_prepare();
        ui_starmap_handle_common(g, &d);
        if (!d.flag_done) {
            ui_starmap_select_bottom_highlight(g, &d);
            ui_starmap_transport_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            ui_starmap_fill_oi_common(&d);
            if (d.controllable) {
                d.oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            }
            if (d.controllable && ui_starmap_transport_valid_destination(g, &d, g->planet_focus_i[active_player])) {
                d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_table_clear();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.from;
}
