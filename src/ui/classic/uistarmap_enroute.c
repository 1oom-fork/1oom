#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_str.h"
#include "game_util.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "log.h"
#include "types.h"
#include "uicursor.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uiobj.h"
#include "uisearch.h"
#include "uisound.h"
#include "uistarmap_common.h"

/* -------------------------------------------------------------------------- */

static inline bool ui_starmap_enroute_in_frange(struct starmap_data_s *d)
{
    uint8_t pi = d->g->planet_focus_i[d->api];
    const planet_t *p = &d->g->planet[pi];
    return ((p->within_frange[d->api] == 1) || ((p->within_frange[d->api] == 2) && d->ss.sn0.have_reserve_fuel));
}

static void ui_starmap_enroute_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const fleet_enroute_t *r = &(g->enroute[ui_data.starmap.fleet_selected]);
    const empiretechorbit_t *e = &(g->eto[r->owner]);
    uint8_t pto = g->planet_focus_i[d->api];
    const planet_t *pt = &g->planet[pto];
    char buf[0x80];

    ui_starmap_draw_basic(d);
    {
        int x, y;
        x = (r->x - ui_data.starmap.x) * 2 + 5;
        y = (r->y - ui_data.starmap.y) * 2 + 5;
        lbxgfx_draw_frame_offs(x, y, ui_data.gfx.starmap.shipbord, UI_SCREEN_W);
    }
    ui_draw_filled_rect(225, 8, 314, 180, 7);
    lbxgfx_draw_frame(224, 4, ui_data.gfx.starmap.movextr2, UI_SCREEN_W);
    if ((r->owner == d->api) && (d->en.can_move != NO_MOVE)) {
        lbxgfx_draw_frame(224, 160, ui_data.gfx.starmap.movextr3, UI_SCREEN_W);
    }
    ui_draw_filled_rect(227, 8, 310, 39, 0);
    lbxgfx_set_frame_0(ui_data.gfx.starmap.scanner);
    for (int f = 0; f <= ui_data.starmap.frame_scanner; ++f) {
        lbxgfx_draw_frame(227, 8, ui_data.gfx.starmap.scanner, UI_SCREEN_W);
    }
    sprintf(buf, "%s %s", game_str_tbl_race[e->race], game_str_sm_fleet);
    lbxfont_select_set_12_4(5, tbl_banner_fontparam[e->banner], 0, 0);
    lbxfont_print_str_center(267, 10, buf, UI_SCREEN_W);
    if ((r->owner == d->api) || (g->eto[d->api].have_ia_scanner)) {
        const planet_t *pd = &(g->planet[r->dest]);
        uint8_t *gfx;
        int x0, y0, x1, y1, dist;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 8;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 8;
        lbxgfx_draw_frame_offs(x1, y1, ui_data.gfx.starmap.planbord, UI_SCREEN_W);
        x0 = (r->x - ui_data.starmap.x) * 2 + 8;
        y0 = (r->y - ui_data.starmap.y) * 2 + 8;
        {
            const uint8_t *ctbl;
            ctbl = ((r->owner == d->api) && (d->en.can_move != NO_MOVE) && (!ui_starmap_enroute_in_frange(d))) ? colortbl_line_red : colortbl_line_green;
            ui_draw_line_limit_ctbl(x0 + 4, y0 + 1, x1 + 6, y1 + 6, ctbl, 5, ui_data.starmap.line_anim_phase);
        }
        gfx = ui_data.gfx.starmap.smalship[e->banner];
        if (pd->x < r->x) {
            lbxgfx_set_new_frame(gfx, 1);
        } else {
            lbxgfx_set_frame_0(gfx);
        }
        lbxgfx_draw_frame_offs(x0, y0, gfx, UI_SCREEN_W);
        dist = game_get_min_dist(g, r->owner, g->planet_focus_i[d->api]);
        if ((r->owner == d->api) && (d->en.can_move != NO_MOVE) && !ui_starmap_enroute_in_frange(d)) {
            /* FIXME use proper positioning for varying str length */
            sprintf(buf, "  %s   %i %s.", game_str_sm_outsr, dist - e->fuel_range, game_str_sm_parsecs2);
            lbxfont_select_set_12_4(2, 0, 0, 0);
            lbxfont_set_gap_h(2);
            lbxfont_print_str_split(230, 26, 80, buf, 2, UI_SCREEN_W, UI_SCREEN_H);
        } else {
            int eta = game_calc_eta_ship(g, game_fleet_get_speed(g, r, d->en.pon, pto), pt->x, pt->y, r->x, r->y);
            sprintf(buf, "%s %i %s", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
            lbxfont_select_set_12_4(0, 0, 0, 0);
            lbxfont_print_str_center(268, 32, buf, UI_SCREEN_W);
        }
    }
    for (int i = 0; i < d->ss.sn0.num; ++i) {
        const shipdesign_t *sd = &(g->srd[r->owner].design[0]);
        uint8_t *gfx;
        int st, x, y;
        x = (i & 1) * 43 + 228;
        y = (i / 2) * 40 + 44;
        ui_draw_filled_rect(x, y, x + 38, y + 24, 0);
        ui_draw_filled_rect(x, y + 28, x + 38, y + 34, 0x1c);
        ui_draw_stars(x, y, 0, 38);
        st = d->ss.sn0.type[i];
        gfx = ui_data.gfx.ships[sd[st].look];
        lbxgfx_set_frame_0(gfx);
        for (int f = 0; f <= ui_data.starmap.frame_ship; ++f) {
            lbxgfx_draw_frame(x, y, gfx, UI_SCREEN_W);
        }
        lbxfont_select(0, 0xd, 0, 0);
        lbxfont_print_num_right(x + 35, y + 19, d->ss.sn0.ships[i], UI_SCREEN_W);
        lbxfont_select(2, 0xa, 0, 0);
        lbxfont_print_str_center(x + 19, y + 29, sd[st].name, UI_SCREEN_W);
    }
    if (ui_data.starmap.scanner_delay == 0) {
        ui_data.starmap.frame_scanner = (ui_data.starmap.frame_scanner + 1) % 20;
        ++ui_data.starmap.scanner_delay;
    } else {
        ui_data.starmap.scanner_delay = 0;
    }
    if (1
      && (r->owner == d->api)
      && (d->en.can_move != NO_MOVE)
      && (!ui_starmap_enroute_in_frange(d) || (game_num_retreat_redir_fix && r->retreat && !e->have_hyperspace_comm && (pto == d->en.pon)))
    ) {
        lbxgfx_set_new_frame(ui_data.gfx.starmap.reloc_bu_accept, 1);
        lbxgfx_draw_frame(271, 163, ui_data.gfx.starmap.reloc_bu_accept, UI_SCREEN_W);
    }
    ui_data.starmap.frame_ship = (ui_data.starmap.frame_ship + 1) % 5;
    ui_draw_set_stars_xoffs(false);
}

/* -------------------------------------------------------------------------- */

void ui_starmap_enroute(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_scroll, oi_cancel, oi_accept, oi_search;
    int16_t scrollx = 0, scrolly = 0;
    struct starmap_data_s d;
    fleet_enroute_t *r;

    d.g = g;
    d.api = active_player;
    d.anim_delay = 0;

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
    d.from = g->planet_focus_i[active_player];
    ui_data.starmap.frame_scanner = 0;
    ui_data.starmap.scanner_delay = 0;
    ui_data.starmap.frame_ship = 0;
    g->planet_focus_i[active_player] = r->dest;
    ui_starmap_sn0_setup(&d.ss.sn0, g->eto[r->owner].shipdesigns_num, r->ships);
    ui_starmap_update_reserve_fuel(g, &d.ss.sn0, r->ships, active_player);

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        oi_accept = UIOBJI_INVALID; \
        oi_cancel = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(3);
    uiobj_set_callback_and_delay(ui_starmap_enroute_draw_cb, &d, STARMAP_DELAY);

    while (!flag_done) {
        planet_t *p;
        int16_t oi1, oi2;
        p = &g->planet[g->planet_focus_i[active_player]];
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        ui_starmap_handle_scrollkeys(&d, oi1);
        if (oi1 == d.oi_gameopts) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_GAMEOPTS;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_design) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_DESIGN;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_fleet) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_FLEET;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_map) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_MAP;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_races) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_RACES;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_planets) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_PLANETS;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_tech) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_TECH;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == d.oi_next_turn) {
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_NEXT_TURN;
            flag_done = true;
            ui_sound_play_sfx_24();
        } else if (oi1 == oi_search) {
            ui_sound_play_sfx_24();
            if (ui_search_set_pos(g, active_player)) {
                if ((r->owner != active_player) || (d.can_move == NO_MOVE)) {
                    d.from = g->planet_focus_i[active_player];
                    flag_done = true;
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                }
            }
        } else if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi1 == oi_accept) {
            ui_sound_play_sfx_24();
            if (p->within_frange[active_player] != 0) {
                game_fleet_redirect(g, r, d.en.pon, g->planet_focus_i[active_player]);
                flag_done = true;
            }
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        }
        for (int i = 0; i < g->enroute_num; ++i) {
            if (oi1 == d.oi_tbl_enroute[i]) {
                ui_data.starmap.fleet_selected = i;
                ui_sound_play_sfx_24();
                flag_done = true;
                break;
            }
        }
        if ((r->owner != active_player) || (d.en.can_move == NO_MOVE)) {
            for (int i = 0; i < g->transport_num; ++i) {
                if (oi1 == d.oi_tbl_transport[i]) {
                    ui_data.starmap.fleet_selected = i;
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_TRANSPORT_SEL;
                    ui_sound_play_sfx_24();
                    flag_done = true;
                    break;
                }
            }
            for (int i = 0; i < g->galaxy_stars; ++i) {
                for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                    if (oi1 == d.oi_tbl_pl_stars[j][i]) {
                        g->planet_focus_i[active_player] = i;
                        d.from = i;
                        ui_data.starmap.orbit_player = j;
                        ui_data.ui_main_loop_action = (j == active_player) ? UI_MAIN_LOOP_ORBIT_OWN_SEL : UI_MAIN_LOOP_ORBIT_EN_SEL;
                        ui_sound_play_sfx_24();
                        flag_done = true;
                        j = g->players; i = g->galaxy_stars;
                    }
                }
            }
        }
        if (oi1 == oi_scroll) {
            ui_starmap_scroll(g, scrollx, scrolly);
        }
        ui_starmap_handle_oi_ctrl(&d, oi1);
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if (oi1 == d.oi_tbl_stars[i]) {
                g->planet_focus_i[active_player] = i;
                if ((r->owner != active_player) || (d.en.can_move == NO_MOVE)) {
                    d.from = i;
                    flag_done = true;
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
                }
                break;
            }
        }
        if (!flag_done) {
            d.bottom_highlight = -1;
            if (oi2 == d.oi_gameopts) {
                d.bottom_highlight = 0;
            } else if (oi2 == d.oi_design) {
                d.bottom_highlight = 1;
            } else if (oi2 == d.oi_fleet) {
                d.bottom_highlight = 2;
            } else if (oi2 == d.oi_map) {
                d.bottom_highlight = 3;
            } else if (oi2 == d.oi_races) {
                d.bottom_highlight = 4;
            } else if (oi2 == d.oi_planets) {
                d.bottom_highlight = 5;
            } else if (oi2 == d.oi_tech) {
                d.bottom_highlight = 6;
            } else if (oi2 == d.oi_next_turn) {
                d.bottom_highlight = 7;
            }
            ui_starmap_enroute_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            /* uiobj_set_limits(STARMAP_LIMITS); */
            ui_starmap_fill_oi_tbls(&d);
            ui_starmap_fill_oi_tbl_stars(&d);
            if ((r->owner == active_player) && (d.en.can_move != NO_MOVE)) {
                oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
                if (ui_starmap_enroute_in_frange(&d) && ((!game_num_retreat_redir_fix) || (!r->retreat) || g->eto[active_player].have_hyperspace_comm || (g->planet_focus_i[active_player] != d.en.pon))) {
                    oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
                }
            }
            oi_scroll = uiobj_add_tb(6, 6, 2, 2, 108, 86, &scrollx, &scrolly);
            oi_search = uiobj_add_inputkey(MOO_KEY_SLASH);
            ui_starmap_fill_oi_ctrl(&d);
            ui_starmap_add_oi_bottom_buttons(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_table_clear();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.from;
}
