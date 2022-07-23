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

static bool ui_starmap_enroute_within_frange(const struct game_s *g, const struct starmap_data_s *d, int planet_i) {
    const planet_t *p = &g->planet[planet_i];
    return p->within_frange[d->api] == 1 || (p->within_frange[d->api] == 2 && d->en.sn0.have_reserve_fuel);
}

static bool ui_starmap_enroute_valid_destination(const struct game_s *g, const struct starmap_data_s *d, int planet_i) {
    const fleet_enroute_t *r = &g->enroute[ui_data.starmap.fleet_selected];
    return 1
      && ui_starmap_enroute_within_frange(g, d, planet_i)
      && (!game_num_retreat_redir_fix || !r->retreat || d->en.can_move == GOT_HYPERCOMM || planet_i != d->en.pon);
}

static void ui_starmap_enroute_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const fleet_enroute_t *r = &(g->enroute[ui_data.starmap.fleet_selected]);
    const empiretechorbit_t *e = &(g->eto[r->owner]);
    uint8_t pto = g->planet_focus_i[d->api];
    const planet_t *pt = &g->planet[pto];
    bool in_frange = ui_starmap_enroute_within_frange(g, d, g->planet_focus_i[d->api]);
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
    lbxgfx_draw_frame(224, 4, ui_data.gfx.starmap.movextr2, UI_SCREEN_W, ui_scale);
    if (d->controllable) {
        lbxgfx_draw_frame(224, 160, ui_data.gfx.starmap.movextr3, UI_SCREEN_W, ui_scale);
    }
    ui_draw_filled_rect(227, 8, 310, 39, 0, ui_scale);
    lbxgfx_set_frame_0(ui_data.gfx.starmap.scanner);
    for (int f = 0; f <= d->en.frame_scanner; ++f) {
        lbxgfx_draw_frame(227, 8, ui_data.gfx.starmap.scanner, UI_SCREEN_W, ui_scale);
    }
    lib_sprintf(buf, sizeof(buf), "%s %s", game_str_tbl_race[e->race], game_str_sm_fleet);
    lbxfont_select_set_12_4(5, tbl_banner_fontparam[e->banner], 0, 0);
    lbxfont_print_str_center(267, 10, buf, UI_SCREEN_W, ui_scale);
    if ((r->owner == d->api) || (g->eto[d->api].have_ia_scanner)) {
        const planet_t *pd = &(g->planet[r->dest]);
        uint8_t *gfx;
        int x0, y0, x1, y1, dist;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 8;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 8;
        lbxgfx_draw_frame_offs(x1, y1, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        x0 = (r->x - ui_data.starmap.x) * 2 + 8;
        y0 = (r->y - ui_data.starmap.y) * 2 + 8;
        {
            const uint8_t *ctbl;
            ctbl = (d->controllable && !in_frange) ? colortbl_line_red : colortbl_line_green;
            ui_draw_line_limit_ctbl(x0 + 4, y0 + 1, x1 + 6, y1 + 6, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
        }
        gfx = ui_data.gfx.starmap.smalship[e->banner];
        if (pd->x < r->x) {
            lbxgfx_set_new_frame(gfx, 1);
        } else {
            lbxgfx_set_frame_0(gfx);
        }
        lbxgfx_draw_frame_offs(x0, y0, gfx, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        dist = game_get_min_dist(g, r->owner, g->planet_focus_i[d->api]);
        if (d->controllable && !in_frange) {
            /* FIXME use proper positioning for varying str length */
            lib_sprintf(buf, sizeof(buf), "  %s   %i %s.", game_str_sm_outsr, dist - e->fuel_range, game_str_sm_parsecs2);
            lbxfont_select_set_12_4(2, 0, 0, 0);
            lbxfont_set_gap_h(2);
            lbxfont_print_str_split(230, 26, 80, buf, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else {
            int eta = game_calc_eta_ship(g, game_fleet_get_speed(g, r, d->en.pon, pto), pt->x, pt->y, r->x, r->y);
            lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
            lbxfont_select_set_12_4(0, 0, 0, 0);
            lbxfont_print_str_center(268, 32, buf, UI_SCREEN_W, ui_scale);
        }
    } else {
        /*d->en.in_frange = false;*/
    }
    for (int i = 0; i < d->en.sn0.num; ++i) {
        const shipdesign_t *sd = &(g->srd[r->owner].design[0]);
        uint8_t *gfx;
        int st, x, y;
        x = (i & 1) * 43 + 228;
        y = (i / 2) * 40 + 44;
        ui_draw_filled_rect(x, y, x + 38, y + 24, 0, ui_scale);
        ui_draw_filled_rect(x, y + 28, x + 38, y + 34, 0, ui_scale);
        ui_draw_stars(x, y, 0, 32, &(d->en.ds), ui_scale);
        st = d->en.sn0.type[i];
        gfx = ui_data.gfx.ships[sd[st].look];
        lbxgfx_set_frame_0(gfx);
        for (int f = 0; f <= d->en.frame_ship; ++f) {
            lbxgfx_draw_frame(x, y, gfx, UI_SCREEN_W, ui_scale);
        }
        lbxfont_select(0, 0xd, 0, 0);
        lbxfont_print_num_right(x + 35, y + 19, d->en.sn0.ships[i], UI_SCREEN_W, ui_scale);
        lbxfont_select(2, 0xa, 0, 0);
        lbxfont_print_str_center(x + 19, y + 29, sd[st].name, UI_SCREEN_W, ui_scale);
    }
    if (d->en.scanner_delay == 0) {
        d->en.frame_scanner = (d->en.frame_scanner + 1) % 20;
        ++d->en.scanner_delay;
    } else {
        d->en.scanner_delay = 0;
    }
    if (1
      && d->controllable
      && (!ui_starmap_enroute_valid_destination(g, d, g->planet_focus_i[d->api]))
    ) {
        lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
        lbxgfx_draw_frame(271, 163, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W, ui_scale);
    }
    d->en.frame_ship = (d->en.frame_ship + 1) % 5;
    ui_draw_set_stars_xoffs(&d->en.ds, false);
}

/* -------------------------------------------------------------------------- */

static void ui_starmap_enroute_do_accept(struct game_s *g, struct starmap_data_s *d) {
    fleet_enroute_t *r = &g->enroute[ui_data.starmap.fleet_selected];
    game_fleet_redirect(g, r, d->en.pon, g->planet_focus_i[d->api]);
}

void ui_starmap_enroute(struct game_s *g, player_id_t active_player)
{
    struct starmap_data_s d;
    ui_starmap_init_common_data(g, &d, active_player);

    fleet_enroute_t *r;

    r = &(g->enroute[ui_data.starmap.fleet_selected]);
    d.en.can_move = g->eto[active_player].have_hyperspace_comm ? GOT_HYPERCOMM : NO_MOVE;
    d.en.pon = PLANET_NONE;
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p;
        p = &g->planet[i];
        if ((p->x == r->x) && (p->y == r->y)) {
            if (d.en.can_move == NO_MOVE) {
                d.en.can_move = ON_PLANET;
            }
            d.en.pon = i;
            break;
        }
    }

    d.controllable = r->owner == active_player && d.en.can_move != NO_MOVE;
    d.draw_own_routes = (r->owner == active_player) || g->eto[d.api].have_ia_scanner;
    d.is_valid_selection = d.controllable ? ui_starmap_enroute_valid_destination : NULL;
    d.do_accept = ui_starmap_enroute_do_accept;

    d.en.frame_scanner = 0;
    d.en.scanner_delay = 0;
    d.en.frame_ship = 0;
    d.en.ds.xoff1 = 0;
    d.en.ds.xoff2 = 0;
    g->planet_focus_i[active_player] = r->dest;
    ui_starmap_sn0_setup(&d.en.sn0, g->eto[r->owner].shipdesigns_num, r->ships);
    ui_starmap_update_reserve_fuel(g, &d.en.sn0, r->ships, active_player);

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(3);
    uiobj_set_callback_and_delay(ui_starmap_enroute_draw_cb, &d, STARMAP_DELAY);

    while (!d.flag_done) {
        ui_delay_prepare();
        ui_starmap_handle_common(g, &d);
        if (!d.flag_done) {
            ui_starmap_select_bottom_highlight(g, &d);
            ui_starmap_enroute_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            ui_starmap_fill_oi_common(&d);
            if (d.controllable) {
                d.oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            }
            if (d.controllable && ui_starmap_enroute_valid_destination(g, &d, g->planet_focus_i[active_player])) {
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
