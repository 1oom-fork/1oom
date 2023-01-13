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

static bool ui_starmap_transport_in_frange(const struct starmap_data_s *d, uint8_t planet_i)
{
    struct planet_s *p = &d->g->planet[planet_i];
    return p->within_frange[d->api] == 1;
}

static void ui_starmap_transport_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const transport_t *r = &(g->transport[ui_data.starmap.fleet_selected]);
    const empiretechorbit_t *e = &(g->eto[r->owner]);
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
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
    if (!d->hide_focus) {
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
            dest_ok = d->valid_target_cb(d, g->planet_focus_i[d->api]);
        }
        {
            const uint8_t *ctbl;
            ctbl = dest_ok ? colortbl_line_green : colortbl_line_red;
            if (ui_modern_controls && d->controllable) {
                int x2, y2;
                x2 = (pd->x - ui_data.starmap.x) * 2 + 8;
                y2 = (pd->y - ui_data.starmap.y) * 2 + 8;
                ui_draw_planet_frame_limit_ctbl(x1, y1, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
                ui_draw_line_limit_ctbl(x0 + 4, y0 + 1, x2 + 6, y2 + 6, colortbl_line_green, 5, ui_data.starmap.line_anim_phase, starmap_scale);
            }
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
        if (d->controllable && !ui_starmap_transport_in_frange(d, g->planet_focus_i[d->api])) {
            /* FIXME use proper positioning for varying str length */
            lib_sprintf(buf, sizeof(buf), "  %s   %i %s.", game_str_sm_outsr, dist - e->fuel_range, game_str_sm_parsecs2);
            lbxfont_select_set_12_4(2, 0, 0, 0);
            lbxfont_set_gap_h(2);
            lbxfont_print_str_split(230, 26, 80, buf, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        } else {
            int eta = game_calc_eta_trans(g, r->speed, pt->x, pt->y, r->x, r->y);
            lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
            lbxfont_select_set_12_4(0, 0, 0, 0);
            lbxfont_print_str_center(268, 32, buf, UI_SCREEN_W, ui_scale);
        }
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
    if (!d->hide_focus) {
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
    if (d->controllable && !d->valid_target_cb(d, g->planet_focus_i[d->api])) {
        lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
        lbxgfx_draw_frame(271, 163, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W, ui_scale);
    }
    ui_draw_set_stars_xoffs(&d->ts.ds, false);
}

/* -------------------------------------------------------------------------- */

static bool ui_starmap_transport_valid_destination(const struct starmap_data_s *d, int planet_i)
{
    const struct game_s *g = d->g;
    return game_transport_dest_ok(g, &(g->planet[planet_i]), d->api);
}

static void ui_starmap_transport_do_accept(struct starmap_data_s *d)
{
    struct game_s *g = d->g;
    uint8_t planet_i = g->planet_focus_i[d->api];
    planet_t *p = &g->planet[planet_i];
    transport_t *r = &(g->transport[ui_data.starmap.fleet_selected]);

    if (p->within_frange[d->api] == 1) {
        r->dest = planet_i;
    }
}

static int ui_starmap_transport_next(const struct game_s *g, player_id_t pi, int i)
{
    const transport_t *r = &g->transport[i];
    player_id_t owner = r->owner;
    int start = i;
    do {
        i = (i + 1) % g->transport_num;
        r = &g->transport[i];
        if (r->owner == owner && BOOLVEC_IS1(r->visible, pi)) {
            return i;
        }
    } while (i != start);
    return g->transport_num;
}

static int ui_starmap_transport_prev(const struct game_s *g, player_id_t pi, int i)
{
    const transport_t *r = &g->transport[i];
    player_id_t owner = r->owner;
    int start = i;
    do {
        if (--i < 0) { i = g->transport_num - 1; }
        r = &g->transport[i];
        if (r->owner == owner && BOOLVEC_IS1(r->visible, pi)) {
            return i;
        }
    } while (i != start);
    return g->transport_num;
}

static void ui_starmap_transport_set_pos_focus(struct starmap_data_s *d)
{
    const transport_t *r = &d->g->transport[ui_data.starmap.fleet_selected];
    ui_starmap_set_pos(d->g, r->x, r->y);
}

void ui_starmap_transport(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_cancel, oi_f4, oi_f5;
    struct starmap_data_s d;
    transport_t *r = &(g->transport[ui_data.starmap.fleet_selected]);

    ui_starmap_common_init(g, &d, active_player);
    d.valid_target_cb = ui_starmap_transport_valid_destination;
    d.on_accept_cb = ui_starmap_transport_do_accept;
    d.on_pos_focus_cb = ui_starmap_transport_set_pos_focus;

    d.ts.frame_ship = 0;
    d.ts.frame_scanner = 0;
    d.ts.scanner_delay = 0;
    g->planet_focus_i[active_player] = r->dest;
    d.ts.can_move = g->eto[active_player].have_hyperspace_comm ? GOT_HYPERCOMM : NO_MOVE;
    d.ts.ds.xoff1 = 0;
    d.ts.ds.xoff2 = 0;

    ui_starmap_common_late_init(&d, ui_starmap_transport_draw_cb,
                                (d.ts.can_move != NO_MOVE) && (r->owner == active_player));
    d.hide_focus = ((r->owner != d.api) && (!g->eto[d.api].have_ia_scanner));

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        oi_cancel = UIOBJI_INVALID; \
        oi_f4 = UIOBJI_INVALID; \
        oi_f5 = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(3);

    while (!flag_done) {
        int16_t oi1, oi2;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if (ui_starmap_common_handle_oi(g, &d, &flag_done, oi1, oi2)) {
        } else if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_f4) {
            int i = ui_starmap_transport_next(g, active_player, ui_data.starmap.fleet_selected);
            if (i != g->transport_num) {
                ui_data.starmap.fleet_selected = i;
                d.on_pos_focus_cb(&d);
                flag_done = true;
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f5) {
            int i = ui_starmap_transport_prev(g, active_player, ui_data.starmap.fleet_selected);
            if (i != g->transport_num) {
                ui_data.starmap.fleet_selected = i;
                d.on_pos_focus_cb(&d);
                flag_done = true;
                ui_sound_play_sfx_24();
            }
        }
        if (!flag_done) {
            ui_starmap_select_bottom_highlight(&d, oi2);
            ui_starmap_transport_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
            oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
            ui_starmap_common_fill_oi(&d);
            if (d.controllable) {
                oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
                if (d.valid_target_cb(&d, g->planet_focus_i[active_player])) {
                    d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
                }
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_table_clear();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.from_i;
}
