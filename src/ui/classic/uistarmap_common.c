#include "config.h"

#include <stdio.h>

#include "uistarmap_common.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_save.h"
#include "game_num.h"
#include "game_str.h"
#include "game_tech.h"
#include "kbd.h"
#include "lbxgfx.h"
#include "lbxfont.h"
#include "lib.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "ui.h"
#include "uicursor.h"
#include "uidraw.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uimsgfilter.h"
#include "uiobj.h"
#include "uisearch.h"
#include "uisound.h"
#include "uistarmap.h"
#include "mouse.h"

/* -------------------------------------------------------------------------- */

const uint8_t colortbl_textbox[5] = { 0x18, 0x17, 0x16, 0x15, 0x14 };
const uint8_t colortbl_line_red[5] = { 0x44, 0x43, 0x42, 0x41, 0x40 };
const uint8_t colortbl_line_reloc[5] = { 0x14, 0x15, 0x16, 0x17, 0x18 };
const uint8_t colortbl_line_green[5] = { 0xb0, 0xb1, 0xb2, 0xb3, 0xb4 };

/* -------------------------------------------------------------------------- */

static void ui_starmap_draw_planetinfo_do(const struct game_s *g, player_id_t api, uint8_t planet_i, bool explored, bool show_plus)
{
    const planet_t *p = &g->planet[planet_i];
    if (explored) {
        lbxfont_set_temp_color(0x0);
        lbxfont_select_set_12_4(4, 0xf, 0, 0);
        lbxfont_print_str_center(269, 10, p->name, UI_SCREEN_W, ui_scale);
        /* stars in nebulas get a purple instead of a red frame */
        if (p->battlebg==0) {
            ui_draw_box1(228, 25, 309, 52, 0xd3, 0xd3, ui_scale);
        }
        if (p->type == PLANET_TYPE_NOT_HABITABLE) {
            lbxfont_select(0, 0xe, 0, 0);
            lbxfont_print_str_center(269, 32, game_str_sm_nohabit, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_center(269, 41, game_str_sm__planets, UI_SCREEN_W, ui_scale);
        } else {
            const char *str = NULL;
            lbxgfx_draw_frame(229, 27, ui_data.gfx.planets.planet[p->infogfx], UI_SCREEN_W, ui_scale);
            lbxfont_select(0, 0xd, 0, 0);
            lbxfont_print_str_right(305, 28, game_str_tbl_sm_pltype[p->type], UI_SCREEN_W, ui_scale);
            if (g->evn.have_plague && (g->evn.plague_planet_i == planet_i)) {
                str = game_str_sm_plague;
            } else if (g->evn.have_nova && (g->evn.nova_planet_i == planet_i)) {
                str = game_str_sm_nova;
            } else if (g->evn.have_comet && (g->evn.comet_planet_i == planet_i)) {
                str = game_str_sm_comet;
            } else if (g->evn.have_pirates && (g->evn.pirates_planet_i == planet_i)) {
                str = game_str_sm_pirates;
            } else if (p->unrest == PLANET_UNREST_REBELLION) {
                str = game_str_sm_rebellion;
            } else if (p->unrest == PLANET_UNREST_UNREST) {
                str = game_str_sm_unrest;
            } else if (g->evn.have_accident && (g->evn.accident_planet_i == planet_i)) {
                str = game_str_sm_accident;
            }
            if (str) {
                lbxfont_select(5, 5, 0, 0);
                lbxfont_print_str_right(305, 40, str, UI_SCREEN_W, ui_scale);
            } else {
                int x, xp, max_pop = p->max_pop3;
                lbxfont_select(0, 1, 0, 0);
                if (p->special == PLANET_SPECIAL_NORMAL) {
                    str = game_str_tbl_sm_pgrowth[p->growth];
                } else {
                    str = game_str_tbl_sm_pspecial[p->special];
                }
                lbxfont_print_str_right(305, 36, str, UI_SCREEN_W, ui_scale);
                lbxfont_select(2, 0xe, 0, 0);
                if (show_plus && (p->owner == api) && game_planet_can_terraform(g, p)) {
                    lbxfont_print_str_normal(289, 44, "+", UI_SCREEN_W, ui_scale);
                    x = 287;
                    xp = x - 22;
                } else {
                    x = 291;
                    xp = x - 23;
                }
                lbxfont_print_str_normal(xp, 45, game_str_sm_pop, UI_SCREEN_W, ui_scale);
                lbxfont_print_str_normal(295, 45, game_str_sm_max, UI_SCREEN_W, ui_scale);
                if ((!show_plus) || (g->eto[api].race != RACE_SILICOID)) {
                    max_pop -= p->waste;
                }
                SETMAX(max_pop, 10);
                lbxfont_print_num_right(x, 45, max_pop, UI_SCREEN_W, ui_scale);
            }
        }
    } else {
        lbxfont_select(5, 0xe, 0, 0);
        lbxfont_print_str_center(269, show_plus ? 27 : 35, game_str_sm_unexplored, UI_SCREEN_W, ui_scale);
    }
}

static void ui_starmap_draw_range_parsec(struct starmap_data_s *d, int y)
{
    const struct game_s *g = d->g;
    int dist = game_get_min_dist(g, d->api, g->planet_focus_i[d->api]);
    char buf[64];
    lib_sprintf(buf, sizeof(buf), "%s %i %s", game_str_sm_range, dist, (dist == 1) ? game_str_sm_parsec : game_str_sm_parsecs);
    lbxfont_select_set_12_4(0, 4, 0, 0);
    lbxfont_print_str_center(269, y, buf, UI_SCREEN_W, ui_scale);
}

void ui_starmap_draw_sliders_and_prod(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    const planet_t *p = &g->planet[g->planet_focus_i[d->api]];
    int x = 311;
    char buf[64];
    int gvcol = ui_draw_govern_color(p, d->api);

    for (planet_slider_i_t i = PLANET_SLIDER_SHIP; i < PLANET_SLIDER_NUM; ++i) {
        ui_draw_filled_rect(227, 81 + 11 * i, 244, 90 + 11 * i, p->slider_lock[i] ? 0x22 : 0, ui_scale);
    }
    if (gvcol && BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOV_BOOST_BUILD)) {
        ui_draw_filled_rect( 228, 71, 312, 77, BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOV_BOOST_PROD) ? 0x81 : 0xc3, ui_scale);
    }
    lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.yourplnt, UI_SCREEN_W, ui_scale);
    if (gvcol) {
        /* highlight governor with a border; use a color based on mode */
        ui_draw_box1(227, 8, 310, 20, gvcol, gvcol, ui_scale);
    }
    if (gvcol &&  d->gov_highlight) {
        uint8_t const gov_boost_col[3] = { 0x00, 0xc3, 0x81 };
        int i = 0;
        if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOV_BOOST_BUILD)) ++i;
        if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOV_BOOST_PROD)) ++i;
        ui_draw_line1( 273, 61, 312, 61, d->gov_highlight == 4 ? 5 : 251, ui_scale);
        ui_draw_filled_rect( 273, 62, 312, 66, d->gov_highlight == 4 ? 5 : 7, ui_scale);
        ui_draw_line1( 228, 72, 312, 72, d->gov_highlight == 5 ? 5 : 251, ui_scale);
        ui_draw_filled_rect( 228, 73, 312, 77, d->gov_highlight == 5 ? 5 : 7, ui_scale);
        lbxfont_select(2, 0, 0, 0);
        lbxfont_print_str_normal( 274, 61, "Plan", UI_SCREEN_W, ui_scale);
        lbxfont_print_str_normal( 229, 72, "Reserve boost", UI_SCREEN_W, ui_scale);
        if (p->target_bases) {
            lbxfont_select(2, p->missile_bases < p->target_bases ? 0xb : 0xd, 0, 0);
            lbxfont_print_num_right(x, 61, p->target_bases, UI_SCREEN_W, ui_scale);
        } else {
            lbxfont_select(2, 6, 0, 0);
            lbxfont_print_str_right(x, 61, "none", UI_SCREEN_W, ui_scale);
        }
        lbxfont_set_color0(gov_boost_col[i]);
        lbxfont_print_str_right(x, 72, game_str_tbl_gv_reserve[i], UI_SCREEN_W, ui_scale); 
    } else {
        lbxfont_select(2, !ui_extra_enabled || !p->reserve ? 0xd : 2 * p->reserve < p->prod_after_maint ? 0xb : 0x0, 0xe, 0);
        lib_sprintf(buf, sizeof(buf), "%i \x02(%i)\x01", p->prod_after_maint, p->total_prod);
        lbxfont_print_str_right(x, 72, buf, UI_SCREEN_W, ui_scale);
        lbxfont_select(2, ui_extra_enabled && p->missile_bases < p->target_bases ? 0xb : 0xd, 0, 0);
        lbxfont_print_num_right(x, 61, p->missile_bases, UI_SCREEN_W, ui_scale);
    }
    lbxfont_select(2, 0xd, 0, 0);
    lbxfont_print_num_right(265, 61, p->pop, UI_SCREEN_W, ui_scale);

    for (planet_slider_i_t i = PLANET_SLIDER_SHIP; i < PLANET_SLIDER_NUM; ++i) {
        ui_draw_filled_rect(253, 84 + 11 * i, 278, 84 + 11 * i + 3, 0x2f, ui_scale);
        if (p->slider[i] != 0) {
            ui_draw_slider(253, 84 + 11 * i + 1, p->slider[i], 4, -1, p->slider_lock[i] ? 0x22 : 0x73, ui_scale);
        }
    }

    lbxfont_select(2, 0xa, 0, 0);
    if (p->buildship == BUILDSHIP_STARGATE) {
        ui_draw_filled_rect(229, 141, 274, 166, 0, ui_scale);
        lbxgfx_draw_frame(229, 141, ui_data.gfx.starmap.stargate, UI_SCREEN_W, ui_scale);
        lbxfont_print_str_center(251, 169, game_str_sm_stargate, UI_SCREEN_W, ui_scale);
    } else {
        const shipdesign_t *sd = &g->srd[d->api].design[p->buildship];
        uint8_t *gfx = ui_data.gfx.ships[sd->look];
        lbxgfx_set_frame_0(gfx);
        lbxgfx_draw_frame(236, 142, gfx, UI_SCREEN_W, ui_scale);
        lbxfont_print_str_center(252, 169, sd->name, UI_SCREEN_W, ui_scale);
    }
    if (ui_extra_enabled && kbd_is_modifier(MOO_MOD_CTRL)) {
        lbxfont_select(2, 0xd, 0, 0);
        lbxfont_print_str_center(252, 143, "Build", UI_SCREEN_W, ui_scale);
        lbxfont_print_str_center(252, 149, "everywhere", UI_SCREEN_W, ui_scale);
    }

    lbxfont_select(2, 6, 0, 0);
    {
        int v;
        v = game_planet_get_slider_text(g, p, d->api, PLANET_SLIDER_SHIP, buf, sizeof(buf));
        lbxfont_print_str_right(x, 83, buf, UI_SCREEN_W, ui_scale);
        lbxfont_select(0, 0xd, 0, 0);
        if (v >= 0) {
            lbxfont_print_num_right(271, 160, v, UI_SCREEN_W, ui_scale);
        }
    }
    lbxfont_select(2, 6, 0, 0);
    game_planet_get_slider_text(g, p, d->api, PLANET_SLIDER_DEF, buf, sizeof(buf));
    lbxfont_print_str_right(x, 94, buf, UI_SCREEN_W, ui_scale);
    game_planet_get_slider_text(g, p, d->api, PLANET_SLIDER_IND, buf, sizeof(buf));
    lbxfont_print_str_right(x, 105, buf, UI_SCREEN_W, ui_scale);
    {
        int v;
        v = game_planet_get_slider_text_eco(g, p, d->api, true, buf, sizeof(buf));
        lbxfont_print_str_right(x, 116, buf, UI_SCREEN_W, ui_scale);
        if (v >= 0) {
            if (v < 100) {
                lib_sprintf(buf, sizeof(buf), "%i.%i", v / 10, v % 10); /* "+0.X" does not fit the box */
            } else {
                lib_sprintf(buf, sizeof(buf), "+%i", v / 10);
            }
            lbxfont_print_str_right(297, 116, buf, UI_SCREEN_W, ui_scale);
        }
    }
    {
        int v;
        v = game_planet_get_slider_text(g, p, d->api, PLANET_SLIDER_TECH, buf, sizeof(buf));
        if (v > 9999) {
            ui_draw_filled_rect(288, 127, 312, 132, 7, ui_scale);
        } else {
            x -= 9;
        }
        lbxfont_print_str_right(x, 127, buf, UI_SCREEN_W, ui_scale);
    }
    if (d->gov_highlight >=1 && d->gov_highlight<=3) {
        uint8_t const gov_col[4] = { 0, 0x55, 0x46, 0x71 };
        ui_draw_filled_rect( 288, 60+22*d->gov_highlight, 312, 66+22*d->gov_highlight, gov_col[d->gov_highlight], ui_scale);
        lbxfont_select(2, 0, 0, 0);
        lbxfont_print_str_center(300, 61+22*d->gov_highlight, "gov", UI_SCREEN_W, ui_scale);
    }
}

static void ui_starmap_draw_textbox_finished(const struct game_s *g, player_id_t api, int pi)
{
    const planet_t *p = &g->planet[pi];
    char *buf = ui_data.strbuf;
    planet_finished_t i;
    for (i = 0; i < FINISHED_NUM; ++i) {
        if (BOOLVEC_IS1(p->finished, i)) {
            break;
        }
    }
    game_planet_get_finished_text(g, p, i, buf, UI_STRBUF_SIZE);
    ui_draw_textbox_2str("", buf, 54, ui_scale);
    ui_draw_textbox_2str("", game_str_sm_planratio, 110, ui_scale);
}

static void ui_starmap_add_oi_enroute(struct starmap_data_s *d, bool want_prio)
{
    const struct game_s *g = d->g;
    const int x = ui_data.starmap.x;
    const int y = ui_data.starmap.y;
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        if (BOOLVEC_IS1(r->visible, d->api) && (BOOLVEC_IS1(ui_data.starmap.select_prio_fleet, i) == want_prio)) {
            int x0 = (r->x - x) * 2 + 8;
            int y0 = (r->y - y) * 2 + 8;
            d->oi_tbl_enroute[i] = uiobj_add_mousearea_limited(x0, y0, x0 + 8, y0 + 4, starmap_scale, MOO_KEY_UNKNOWN);
        }
    }
    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &(g->transport[i]);
        if (BOOLVEC_IS1(r->visible, d->api) && (BOOLVEC_IS1(ui_data.starmap.select_prio_trans, i) == want_prio)) {
            int x0 = (r->x - x) * 2 + 8;
            int y0 = (r->y - y) * 2 + 8;
            d->oi_tbl_transport[i] = uiobj_add_mousearea_limited(x0, y0, x0 + 8, y0 + 4, starmap_scale, MOO_KEY_UNKNOWN);
        }
    }
}

/* -------------------------------------------------------------------------- */

void ui_starmap_draw_basic(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    const planet_t *p = &g->planet[g->planet_focus_i[d->api]];

    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    ui_draw_filled_rect(224, 5, 314, 178, 0, ui_scale);
    if (BOOLVEC_IS0(p->explored, d->api)) {
        lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.unexplor, UI_SCREEN_W, ui_scale);
        lbxfont_select_set_12_4(5, 1, 0, 0);
        lbxfont_print_str_split(232, 74, 76, game_str_tbl_sm_stinfo[p->star_type], 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        ui_starmap_draw_range_parsec(d, 165);
    } else {
        player_id_t owner = p->owner;
        int pi = g->planet_focus_i[d->api];
        if (BOOLVEC_IS0(p->within_srange, d->api) && ((owner == PLAYER_NONE) || BOOLVEC_IS0(g->eto[d->api].contact, owner))) {
            owner = g->seen[d->api][pi].owner;
        }
        if (owner == PLAYER_NONE) {
            lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.no_colny, UI_SCREEN_W, ui_scale);
            ui_data.gfx.colonies.current = ui_data.gfx.colonies.d[p->type * 2];
            lbxgfx_draw_frame(227, 73, ui_data.gfx.colonies.current, UI_SCREEN_W, ui_scale);
            ui_draw_box1(227, 73, 310, 174, 0, 0, ui_scale);
            ui_starmap_draw_range_parsec(d, 80);
        } else if ((owner != d->api) || (p->unrest == PLANET_UNREST_REBELLION)) {
            char buf[64];
            int pop, bases, range_y;
            lbxgfx_draw_frame(224, 5, ui_data.gfx.starmap.en_colny, UI_SCREEN_W, ui_scale);
            ui_data.gfx.colonies.current = ui_data.gfx.colonies.d[p->type * 2 + 1];
            lbxgfx_draw_frame(227, 73, ui_data.gfx.colonies.current, UI_SCREEN_W, ui_scale);
            ui_draw_box1(227, 73, 310, 174, 0, 0, ui_scale);
            lib_sprintf(buf, sizeof(buf), "%s %s", game_str_tbl_race[g->eto[owner].race], game_str_sm_colony);
            if (BOOLVEC_IS1(p->within_srange, d->api)) {
                lbxfont_select_set_12_4(5, tbl_banner_fontparam[g->eto[owner].banner], 0, 0);
                lbxfont_print_str_center(270, 84, buf, UI_SCREEN_W, ui_scale);
                pop = p->pop;
                bases = p->missile_bases;
                range_y = 95;
            } else {
                lbxfont_select_set_12_4(0, tbl_banner_fontparam[g->eto[owner].banner], 0, 0);
                lbxfont_print_str_center(269, 75, game_str_sm_lastrep, UI_SCREEN_W, ui_scale);
                lbxfont_print_str_center(268, 83, buf, UI_SCREEN_W, ui_scale);    /* TODO combine with above */
                pop = g->seen[d->api][pi].pop;
                bases = g->seen[d->api][pi].bases;
                range_y = 92;
            }
            lbxfont_select(0, 0xd, 0, 0);
            lbxfont_print_num_right(265, 61, pop, UI_SCREEN_W, ui_scale);
            lbxfont_print_num_right(310, 61, bases, UI_SCREEN_W, ui_scale);
            ui_starmap_draw_range_parsec(d, range_y);
        } else {
            ui_starmap_draw_sliders_and_prod(d);
            lbxgfx_set_frame_0(ui_data.gfx.starmap.col_butt_ship);
            lbxgfx_set_frame_0(ui_data.gfx.starmap.col_butt_reloc);
            if (p->buildship == BUILDSHIP_STARGATE) {
                lbxgfx_set_frame(ui_data.gfx.starmap.col_butt_reloc, 1);
            }
            if ((g->evn.have_plague == 0) || (g->evn.plague_planet_i != g->planet_focus_i[d->api])) {
                lbxgfx_set_frame_0(ui_data.gfx.starmap.col_butt_trans);
            } else {
                lbxgfx_set_frame(ui_data.gfx.starmap.col_butt_trans, 1);
            }
            lbxgfx_draw_frame(282, 140, ui_data.gfx.starmap.col_butt_ship, UI_SCREEN_W, ui_scale);
            lbxgfx_draw_frame(282, 152, ui_data.gfx.starmap.col_butt_reloc, UI_SCREEN_W, ui_scale);
            lbxgfx_draw_frame(282, 164, ui_data.gfx.starmap.col_butt_trans, UI_SCREEN_W, ui_scale);
        }
    }
    ui_starmap_draw_planetinfo(g, d->api, g->planet_focus_i[d->api]);
    if (g->evn.build_finished_num[d->api]) {
        ui_starmap_draw_textbox_finished(g, d->api, g->planet_focus_i[d->api]);
    }
}

void ui_starmap_draw_starmap(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    int x, y, tx, ty;
    STARMAP_LIM_INIT();
    {
        int v, step;
        step = ui_sm_scroll_speed;
        v = ui_data.starmap.x2;
        x = ui_data.starmap.x;
        if (v != x) {
            if (step == 0) {
                x = v;
            } else {
                if (v < x) {
                    x -= ui_sm_scroll_speed;
                    SETMAX(x, v);
                } else {
                    x += ui_sm_scroll_speed;
                    SETMIN(x, v);
                }
            }
            ui_data.starmap.x = x;
        }
        v = ui_data.starmap.y2;
        y = ui_data.starmap.y;
        if (v != y) {
            if (step == 0) {
                y = v;
            } else {
                if (v < y) {
                    y -= ui_sm_scroll_speed;
                    SETMAX(y, v);
                } else {
                    y += ui_sm_scroll_speed;
                    SETMIN(y, v);
                }
            }
            ui_data.starmap.y = y;
        }
    }
    if (++d->anim_delay >= STARMAP_ANIM_DELAY) {
        d->anim_delay = 0;
    }
    ui_draw_filled_rect(STARMAP_LIMITS, 0, starmap_scale);
    lbxgfx_draw_frame(0, 0, ui_data.gfx.starmap.mainview, UI_SCREEN_W, ui_scale);
    uiobj_set_limits(STARMAP_LIMITS);
    {
        int x0, y0, x1, y1, lx, ly;
        x0 = (-x / 4) + 6;
        y0 = (-y / 4) + 6;
        x1 = ((-x + 1) / 2) + 6;
        y1 = ((-y + 1) / 2) + 6;
        lx = 222 * ui_scale - 1;
        ly = 178 * ui_scale - 1;
        for (int yb = y0; yb < ly; yb += 200) {
            for (int xb = x0; xb < lx; xb += 320) {
                lbxgfx_draw_frame_offs(xb, yb, ui_data.gfx.starmap.starbak2, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
            }
        }
        for (int yb = y1; yb < ly; yb += 200) {
            for (int xb = x1; xb < lx; xb += 320) {
                lbxgfx_draw_frame_offs(xb, yb, ui_data.gfx.starmap.starback, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
            }
        }
    }
    for (int i = 0; i < g->nebula_num; ++i) {
        int tx, ty;
        tx = (g->nebula_x[i] - x) * 2 + 7;
        ty = (g->nebula_y[i] - y) * 2 + 7;
        lbxgfx_draw_frame_offs(tx, ty, ui_data.gfx.starmap.nebula[i], STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        if (0) { /* code for drawing the nebula bounding boxes in the starmap */
            for (int j = 0; j < 4; ++j) {
                int x0, x1, y0, y1;
                x0 = 2 * (g->nebula_x0[i][j] - x) + 6;
                x1 = 2 * (g->nebula_x1[i][j] - x) + 8;
                y0 = 2 * (g->nebula_y0[i][j] - y) + 6;
                y1 = 2 * (g->nebula_y1[i][j] - y) + 8;
                ui_draw_line_limit(tx - 2, ty, tx + 2, ty, 4, starmap_scale);
                ui_draw_line_limit(tx, ty - 2, tx, ty + 2, 4, starmap_scale);
                ui_draw_line_limit(x0, y0, x1, y0, 4, starmap_scale);
                ui_draw_line_limit(x0, y1, x1, y1, 4, starmap_scale);
                ui_draw_line_limit(x0, y0, x0, y1, 4, starmap_scale);
                ui_draw_line_limit(x1, y0, x1, y1, 4, starmap_scale);
            }
        }
    }
    if (ui_data.starmap.flag_show_grid) {
        int x0, y0, x1, y1;
        for (y0 = 10; y0 < (g->galaxy_maxy * 2); y0 += 50) {
            int ty;
            x0 = (-x) * 2 + 6;
            x1 = (g->galaxy_maxx - x) * 2 + 6;
            ty = (y0 - y) * 2 + 6;
            ui_draw_line_limit(x0, ty, x1, ty, 4, starmap_scale);
        }
        for (x0 = 10; x0 < (g->galaxy_maxx * 2); x0 += 50) {
            int tx;
            y0 = (-y) * 2 + 6;
            y1 = (g->galaxy_maxy - y) * 2 + 6;
            tx = (x0 - x) * 2 + 6;
            ui_draw_line_limit(tx, y0, tx, y1, 4, starmap_scale);
        }
    }
    for (int pi = 0; pi < g->galaxy_stars; ++pi) {
        const planet_t *p = &g->planet[pi];
        if ((p->owner == d->api) && (p->reloc != pi)) {
            const planet_t *p2 = &g->planet[p->reloc];
            int x0, y0, x1, y1;
            x0 = (p->x - x) * 2 + 14;
            x1 = (p2->x - x) * 2 + 14;
            y0 = (p->y - y) * 2 + 14;
            y1 = (p2->y - y) * 2 + 14;
            ui_draw_line_limit_ctbl(x0, y0, x1, y1, colortbl_line_reloc, 5, ui_data.starmap.line_anim_phase, starmap_scale);
        }
    }
    for (int pi = 0; pi < g->galaxy_stars; ++pi) {
        const planet_t *p = &g->planet[pi];
        uint8_t *gfx = ui_data.gfx.starmap.stars[p->star_type + p->look];
        uint8_t anim_frame = ui_data.star_frame[pi];
        lbxgfx_set_new_frame(gfx, (anim_frame < 4) ? anim_frame : 0);
        gfx_aux_draw_frame_to(gfx, &ui_data.starmap.star_aux);
        if (p->look > 0) {
            tx = (p->x - x) * 2 + 8;
            ty = (p->y - y) * 2 + 9;
        } else {
            tx = (p->x - x) * 2 + 11;
            ty = (p->y - y) * 2 + 11;
        }
        gfx_aux_draw_frame_from_limit(tx, ty, &ui_data.starmap.star_aux, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        if (d->anim_delay == 0) {
            if (anim_frame == 4) {
                anim_frame = rnd_0_nm1(50, &ui_data.seed);
            } else {
                anim_frame = (anim_frame + 1) % 50;
            }
            ui_data.star_frame[pi] = anim_frame;
        }
        if (p->owner != PLAYER_NONE) {
            bool do_print;
            do_print = BOOLVEC_IS1(p->within_srange, d->api);
            if (do_print || (pi == g->evn.planet_orion_i)) {
                do_print = true;
                lbxfont_select(2, tbl_banner_fontparam[g->eto[p->owner].banner], 0, 0);
            } else if (BOOLVEC_IS1(g->eto[d->api].contact, p->owner) || (p->within_frange[d->api] == 1)) {
                do_print = true;
                lbxfont_select(2, 0, 0, 0);
                lbxfont_set_color0(tbl_banner_color[g->eto[p->owner].banner]);
            }
            if (do_print) {
                tx = (p->x - x) * 2 + 14;
                ty = (p->y - y) * 2 + 22;
                if (ui_show_populations && (p->owner == d->api || BOOLVEC_IS1(p->within_srange, d->api))) {
                    char str[9];
                    int max_pop;
                    max_pop = p->max_pop3;
                    if (g->eto[d->api].race != RACE_SILICOID) {
                        max_pop -= p->waste;
                    }
                    if (game_planet_can_terraform(g, p)) {
                        lib_sprintf(str, sizeof(str), "%i/%i+", p->pop, max_pop);
                    } else {
                        lib_sprintf(str, sizeof(str), "%i/%i", p->pop, max_pop);
                    }
                    lbxfont_print_str_center_limit(tx, ty, str, STARMAP_TEXT_LIMITS, UI_SCREEN_W, starmap_scale);
                } else {
                    lbxfont_print_str_center_limit(tx, ty, p->name, STARMAP_TEXT_LIMITS, UI_SCREEN_W, starmap_scale);
                }
            }
        }
    }
    if (1
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_RELOC)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_TRANS)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_ORBIT_OWN_SEL)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_TRANSPORT_SEL)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_ENROUTE_SEL)
      && (ui_data.ui_main_loop_action != UI_MAIN_LOOP_ORBIT_EN_SEL)
    ) {
        const planet_t *p = &g->planet[g->planet_focus_i[d->api]];
        tx = (p->x - x) * 2 + 8;
        ty = (p->y - y) * 2 + 8;
        lbxgfx_draw_frame_offs(tx, ty, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    }
    if (d->anim_delay == 0) {
        if (--ui_data.starmap.line_anim_phase < 0) {
            ui_data.starmap.line_anim_phase = 4;
        }
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &g->enroute[i];
        if (BOOLVEC_IS1(r->visible, d->api)) {
            uint8_t *gfx = ui_data.gfx.starmap.smalship[g->eto[r->owner].banner];
            const planet_t *p = &g->planet[r->dest];
            tx = (r->x - x) * 2 + 8;
            ty = (r->y - y) * 2 + 8;
            if (p->x < r->x) {
                lbxgfx_set_new_frame(gfx, 1);
            } else {
                lbxgfx_set_frame_0(gfx);
            }
            p = &g->planet[g->planet_focus_i[d->api]];
            if (g->eto[d->api].have_ia_scanner && (p->owner == d->api) && (r->owner != d->api) && (r->dest == g->planet_focus_i[d->api])) {
                ui_draw_line_limit_ctbl(tx + 5, ty + 2, (p->x - x) * 2 + 14, (p->y - y) * 2 + 14, colortbl_line_red, 5, ui_data.starmap.line_anim_phase, starmap_scale);
            } else if (ui_show_routes && d->draw_own_routes) {
                if (r->owner == d->api && r->dest == g->planet_focus_i[d->api]) {
                    ui_draw_line_limit_ctbl(tx + 5, ty + 2, (p->x - x) * 2 + 14, (p->y - y) * 2 + 14, colortbl_line_green, 5, ui_data.starmap.line_anim_phase, starmap_scale);
                }
            }
            lbxgfx_draw_frame_offs(tx, ty, gfx, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        }
    }
    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &g->transport[i];
        if (BOOLVEC_IS1(r->visible, d->api)) {
            uint8_t *gfx = ui_data.gfx.starmap.smaltran[g->eto[r->owner].banner];
            const planet_t *p = &g->planet[r->dest];
            tx = (r->x - x) * 2 + 8;
            ty = (r->y - y) * 2 + 8;
            if (p->x < r->x) {
                lbxgfx_set_new_frame(gfx, 1);
            } else {
                lbxgfx_set_frame_0(gfx);
            }
            p = &g->planet[g->planet_focus_i[d->api]];
            if (g->eto[d->api].have_ia_scanner && (p->owner == d->api) && (r->owner != d->api) && (r->dest == g->planet_focus_i[d->api])) {
                ui_draw_line_limit_ctbl(tx + 5, ty + 2, (p->x - x) * 2 + 14, (p->y - y) * 2 + 14, colortbl_line_red, 5, ui_data.starmap.line_anim_phase, starmap_scale);
            } else if (ui_show_routes && d->draw_own_routes) {
                if (r->owner == d->api && r->dest == g->planet_focus_i[d->api]) {
                    ui_draw_line_limit_ctbl(tx + 5, ty + 2, (p->x - x) * 2 + 14, (p->y - y) * 2 + 14, colortbl_line_green, 5, ui_data.starmap.line_anim_phase, starmap_scale);
                }
            }
            lbxgfx_draw_frame_offs(tx, ty, gfx, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        }
    }
    if (g->evn.crystal.exists && (g->evn.crystal.killer == PLAYER_NONE)) {
        tx = (g->evn.crystal.x - x) * 2 + 8;
        ty = (g->evn.crystal.y - y) * 2 + 8;
        lbxgfx_draw_frame_offs(tx, ty, ui_data.gfx.planets.smonster, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        lbxfont_select(2, 8, 0, 0);
        lbxfont_print_str_center_limit(tx + 2, ty + 5, game_str_sm_crystal, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    }
    if ((g->evn.amoeba.exists != 0) && (g->evn.amoeba.killer == PLAYER_NONE)) {
        tx = (g->evn.amoeba.x - x) * 2 + 8;
        ty = (g->evn.amoeba.y - y) * 2 + 8;
        lbxgfx_draw_frame_offs(tx, ty, ui_data.gfx.planets.smonster, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
        lbxfont_select(2, 8, 0, 0);
        lbxfont_print_str_center_limit(tx + 2, ty + 5, game_str_sm_amoeba, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    }
    for (int pi = 0; pi < g->galaxy_stars; ++pi) {
        const planet_t *p = &g->planet[pi];
        if (BOOLVEC_IS1(p->within_srange, d->api) || BOOLVEC_IS1(g->eto[d->api].orbit[pi].visible, d->api)) {
            player_id_t tblorbit[PLAYER_NUM];
            player_id_t num;
            num = 0;
            for (player_id_t i = PLAYER_0; i < g->players; ++i) {
                const empiretechorbit_t *e = &g->eto[i];
                if (BOOLVEC_IS0(p->within_srange, d->api) && (i != d->api)) {
                    continue;
                }
                for (int j = 0; j < e->shipdesigns_num; ++j) {
                    if (e->orbit[pi].ships[j]) {
                        tblorbit[num++] = i;
                        break;
                    }
                }
            }
            tx = (p->x - x) * 2 + 25;
            if (p->have_stargate) {
                lbxgfx_draw_frame_offs(tx, (p->y - y) * 2 + 7, ui_data.gfx.starmap.stargate2, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
            }
            ++tx;
            for (player_id_t i = PLAYER_0; i < num; ++i) {
                player_id_t i2 = tblorbit[i];
                uint8_t *gfx = ui_data.gfx.starmap.smalship[g->eto[i2].banner];
                lbxgfx_set_frame_0(gfx);
                ty = (p->y - y) * 2 + i * 6 + 8;
                lbxgfx_draw_frame_offs(tx, ty, gfx, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
            }
        }
    }
    if (ui_extra_enabled && d->oi2 > 0 && ui_data.ui_main_loop_action == UI_MAIN_LOOP_STARMAP && !g->evn.build_finished_num[d->api]) {
        int ruler_from_i, ruler_to_i;
        ruler_from_i = g->planet_focus_i[d->api];
        ruler_to_i = ui_starmap_cursor_on_star(g, d, d->oi2, d->api);
        if (ruler_to_i != PLANET_NONE && ruler_from_i != ruler_to_i) {
            const planet_t *p = &g->planet[ruler_to_i], *q = &g->planet[ruler_from_i];
            int from_x, from_y, to_x, to_y;
            from_x = 2 * (q->x - x) + 14;
            from_y = 2 * (q->y - y) + 14;
            to_x = 2 * (p->x - x) + 14;
            to_y = 2 * (p->y - y) + 14;

            if ( !(from_x < slx0 || from_x > slx1 || from_y < sly0 || from_y > sly1 || to_x < slx0 || to_x > slx1 || to_y < sly0 || to_y > sly1) ) {
                ui_draw_line1(from_x, from_y, to_x, to_y, 0x06, starmap_scale);
                lbxfont_select(0, 0, 0, 0);
                int l = g->gaux->star_dist[ruler_from_i][ruler_to_i];
                lbxfont_print_num_center( (from_x + to_x) / 2, (from_y + to_y) / 2 - 2, l, UI_SCREEN_W, starmap_scale);
            }
        }
    }
}

void ui_starmap_draw_button_text(struct starmap_data_s *d, bool highlight)
{
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 0)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(10, 184, game_str_sm_game, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 1)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(44, 184, game_str_sm_design, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 2)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(83, 184, game_str_sm_fleet, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 3)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(119, 184, game_str_sm_map, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 4)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(147, 184, game_str_sm_races, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 5)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(184, 184, game_str_sm_planets, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 6)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(230, 184, game_str_sm_tech, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_4(5, (highlight && (d->bottom_highlight == 7)) ? 0 : 2, 0, 0);
    lbxfont_print_str_normal(263, 184, game_str_sm_next_turn, UI_SCREEN_W, ui_scale);
}

static bool ui_starmap_remove_build_finished(struct game_s *g, player_id_t api)
{
    planet_t *p = &g->planet[g->planet_focus_i[api]];
    int num = g->evn.build_finished_num[api];
    if (num) {
        g->evn.build_finished_num[api] = --num;
        for (planet_finished_t i = 0; i < FINISHED_NUM; ++i) {
            if ((i != FINISHED_SHIP) && BOOLVEC_IS1(p->finished, i)) {
                BOOLVEC_SET0(p->finished, i);
                break;
            }
        }
        if (!num) {
            return true;
        }
    }
    return false;
}

static void ui_starmap_handle_oi_ctrl(struct starmap_data_s *d, int16_t oi)
{
#define XSTEP   0x1b
#define YSTEP   0x15
    const struct game_s *g = d->g;
    bool changed = false;
    int x, y;
    x = ui_data.starmap.x;
    y = ui_data.starmap.y;
    if (oi == d->oi_ctrl_ul) {
        x -= XSTEP;
        y -= YSTEP;
        changed = true;
    } else if ((oi == d->oi_ctrl_up) || (oi == d->oi_ctrl_u2)) {
        y -= YSTEP;
        changed = true;
    } else if (oi == d->oi_ctrl_ur) {
        x += XSTEP;
        y -= YSTEP;
        changed = true;
    } else if ((oi == d->oi_ctrl_left) || (oi == d->oi_ctrl_l2)) {
        x -= XSTEP;
        changed = true;
    } else if ((oi == d->oi_ctrl_right) || (oi == d->oi_ctrl_r2)) {
        x += XSTEP;
        changed = true;
    } else if (oi == d->oi_ctrl_dl) {
        x -= XSTEP;
        y += YSTEP;
        changed = true;
    } else if ((oi == d->oi_ctrl_down) || (oi == d->oi_ctrl_d2)) {
        y += YSTEP;
        changed = true;
    } else if (oi == d->oi_ctrl_dr) {
        x += XSTEP;
        y += YSTEP;
        changed = true;
    }
    if (changed) {
        SETRANGE(x, 0, g->galaxy_maxx - ((108 * ui_scale) / starmap_scale));
        SETRANGE(y, 0, g->galaxy_maxy - ((86 * ui_scale) / starmap_scale));
        ui_data.starmap.x2 = x;
        ui_data.starmap.y2 = y;
    }
#undef XSTEP
#undef YSTEP
}

static void ui_starmap_handle_scrollkeys(struct starmap_data_s *d, int16_t oi)
{
    const struct game_s *g = d->g;
    int x, y, xh, yh;
    if (oi != 0) {
        return;
    }
    x = ui_data.starmap.x;
    y = ui_data.starmap.y;
    if (kbd_is_pressed(MOO_KEY_u, 0, MOO_MOD_SHIFT | MOO_MOD_ALT | MOO_MOD_CTRL) || moouse_y <= 0) {
        yh = -ui_sm_scroll_speed;
    } else if (kbd_is_pressed(MOO_KEY_j, 0, MOO_MOD_SHIFT | MOO_MOD_ALT | MOO_MOD_CTRL) || moouse_y >= UI_SCREEN_H - 1) {
        yh = ui_sm_scroll_speed;
    } else {
        yh = 0;
    }
    if (yh) {
        y += yh;
    }
    if (kbd_is_pressed(MOO_KEY_h, 0, MOO_MOD_SHIFT | MOO_MOD_ALT | MOO_MOD_CTRL) || moouse_x <= 0) {
        xh = -ui_sm_scroll_speed;
    } else if (kbd_is_pressed(MOO_KEY_k, 0, MOO_MOD_SHIFT | MOO_MOD_ALT | MOO_MOD_CTRL) || moouse_x >= UI_SCREEN_W - 1) {
        xh = ui_sm_scroll_speed;
    } else {
        xh = 0;
    }
    if (xh) {
        x += xh;
    }
    if (xh || yh) {
        SETRANGE(x, 0, g->galaxy_maxx - ((108 * ui_scale) / starmap_scale));
        SETRANGE(y, 0, g->galaxy_maxy - ((86 * ui_scale) / starmap_scale));
        ui_data.starmap.x2 = x;
        ui_data.starmap.y2 = y;
        ui_data.starmap.x = x;
        ui_data.starmap.y = y;
    }
}

static uint8_t ui_starmap_handle_tag(struct starmap_data_s *d, int16_t oi)
{
    for (int i = 0; i < PLANET_TAG_NUM; ++i) {
        if (oi == d->oi_tag_set[i]) {
            ui_data.starmap.tag[d->api][i] = d->g->planet_focus_i[d->api];
            break;
        }
        if (oi == d->oi_tag_get[i]) {
            uint8_t pli;
            pli = ui_data.starmap.tag[d->api][i];
            if (pli < d->g->galaxy_stars) {
                return pli;
            }
            break;
        }
    }
    return PLANET_NONE;
}

static void ui_starmap_add_oi_bottom_buttons(struct starmap_data_s *d)
{
    d->oi_gameopts = uiobj_add_mousearea(5, 181, 36, 194, MOO_KEY_g);
    d->oi_design = uiobj_add_mousearea(40, 181, 75, 194, MOO_KEY_d);
    d->oi_fleet = uiobj_add_mousearea(79, 181, 111, 194, MOO_KEY_f);
    d->oi_map = uiobj_add_mousearea(115, 181, 139, 194, MOO_KEY_m);
    d->oi_races = uiobj_add_mousearea(143, 181, 176, 194, MOO_KEY_e);
    d->oi_planets = uiobj_add_mousearea(180, 181, 221, 194, MOO_KEY_p);
    if (ui_data.ui_main_loop_action != UI_MAIN_LOOP_ORBIT_OWN_SEL) {
        d->oi_tech = uiobj_add_mousearea(225, 181, 254, 194, MOO_KEY_t);
        d->oi_next_turn = uiobj_add_mousearea(258, 181, 314, 194, MOO_KEY_n);
    }
}

static void ui_starmap_add_oi_hotkeys(struct starmap_data_s *d)
{
    d->oi_f2 = uiobj_add_inputkey(MOO_KEY_F2);
    d->oi_f3 = uiobj_add_inputkey(MOO_KEY_F3);
    d->oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
    d->oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
    d->oi_f6 = uiobj_add_inputkey(MOO_KEY_F6);
    d->oi_f7 = uiobj_add_inputkey(MOO_KEY_F7);
    d->oi_f8 = uiobj_add_inputkey(MOO_KEY_F8);
    d->oi_f9 = uiobj_add_inputkey(MOO_KEY_F9);
    d->oi_f10 = uiobj_add_inputkey(MOO_KEY_F10);
    d->oi_caught = uiobj_add_inputkey(MOO_KEY_c);
    d->oi_filter = uiobj_add_inputkey(MOO_KEY_i);
    d->oi_alt_m = uiobj_add_inputkey(MOO_KEY_m | MOO_MOD_ALT);
    d->oi_alt_c = uiobj_add_inputkey(MOO_KEY_c | MOO_MOD_ALT);
    d->oi_alt_o = uiobj_add_inputkey(MOO_KEY_o | MOO_MOD_ALT);
    d->oi_alt_r = uiobj_add_inputkey(MOO_KEY_r | MOO_MOD_ALT);
    d->oi_ctrl_r = uiobj_add_inputkey(MOO_KEY_r | MOO_MOD_CTRL);
    d->oi_alt_f = uiobj_add_inputkey(MOO_KEY_f | MOO_MOD_ALT);
    d->oi_search = uiobj_add_inputkey(MOO_KEY_SLASH);
}

static void ui_starmap_fill_oi_tbls(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    const int x = ui_data.starmap.x;
    const int y = ui_data.starmap.y;
    STARMAP_LIM_INIT();
    uiobj_set_limits(STARMAP_LIMITS);
    UIOBJI_SET_TBL_INVALID(d->oi_tbl_enroute);
    UIOBJI_SET_TBL_INVALID(d->oi_tbl_transport);
    ui_starmap_add_oi_enroute(d, false);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        for (int j = 0; j < g->players; ++j) {
            d->oi_tbl_pl_stars[j][i] = UIOBJI_INVALID;
        }
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if (BOOLVEC_IS1(p->within_srange, d->api) || BOOLVEC_IS1(g->eto[d->api].orbit[i].visible, d->api)) {
            int numorbits, x0, y0;
            player_id_t tblpl[PLAYER_NUM];
            numorbits = 0;
            for (int j = 0; j < g->players; ++j) {
                const fleet_orbit_t *r = &(g->eto[j].orbit[i]);
                if (BOOLVEC_IS0(p->within_srange, d->api) && (j != d->api)) {
                    continue;
                }
                for (int k = 0; k < g->eto[j].shipdesigns_num; ++k) {
                    if (r->ships[k]) {
                        tblpl[numorbits++] = j;
                        break;
                    }
                }
            }
            x0 = (p->x - x) * 2 + 26;
            y0 = (p->y - y) * 2 + 8;
            for (int j = 0; j < numorbits; ++j, y0 += 6) {
                d->oi_tbl_pl_stars[tblpl[j]][i] = uiobj_add_mousearea_limited(x0, y0, x0 + 8, y0 + 4, starmap_scale, MOO_KEY_UNKNOWN);
            }
        }
    }
    ui_starmap_add_oi_enroute(d, true);
}

static void ui_starmap_fill_oi_tbl_stars(struct starmap_data_s *d)
{
    const struct game_s *g = d->g;
    const int x = ui_data.starmap.x;
    const int y = ui_data.starmap.y;
    STARMAP_LIM_INIT();
    uiobj_set_limits(STARMAP_LIMITS);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        int x0, y0;
        x0 = (p->x - x) * 2 + 8;
        y0 = (p->y - y) * 2 + 8;
        d->oi_tbl_stars[i] = uiobj_add_mousearea_limited(x0, y0, x0 + 13, y0 + 13, starmap_scale, MOO_KEY_UNKNOWN);
    }
}

void ui_starmap_clear_oi_ctrl(struct starmap_data_s *d)
{
    d->oi_ctrl_left = UIOBJI_INVALID;
    d->oi_ctrl_l2 = UIOBJI_INVALID;
    d->oi_ctrl_right = UIOBJI_INVALID;
    d->oi_ctrl_r2 = UIOBJI_INVALID;
    d->oi_ctrl_ul = UIOBJI_INVALID;
    d->oi_ctrl_ur = UIOBJI_INVALID;
    d->oi_ctrl_up = UIOBJI_INVALID;
    d->oi_ctrl_u2 = UIOBJI_INVALID;
    d->oi_ctrl_dl = UIOBJI_INVALID;
    d->oi_ctrl_down = UIOBJI_INVALID;
    d->oi_ctrl_d2 = UIOBJI_INVALID;
    d->oi_ctrl_dr = UIOBJI_INVALID;
    UIOBJI_SET_TBL_INVALID(d->oi_tag_set);
    UIOBJI_SET_TBL_INVALID(d->oi_tag_get);
}

static void ui_starmap_fill_oi_ctrl(struct starmap_data_s *d)
{
    d->oi_ctrl_left = uiobj_add_inputkey(MOO_KEY_LEFT | MOO_MOD_CTRL);
    if (MOO_KEY_LEFT != MOO_KEY_KP4) {
        d->oi_ctrl_l2 = uiobj_add_inputkey(MOO_KEY_KP4 | MOO_MOD_CTRL);
    }
    d->oi_ctrl_right = uiobj_add_inputkey(MOO_KEY_RIGHT | MOO_MOD_CTRL);
    if (MOO_KEY_RIGHT != MOO_KEY_KP6) {
        d->oi_ctrl_r2 = uiobj_add_inputkey(MOO_KEY_KP6 | MOO_MOD_CTRL);
    }
    d->oi_ctrl_ul = uiobj_add_inputkey(MOO_KEY_KP7 | MOO_MOD_CTRL);
    d->oi_ctrl_ur = uiobj_add_inputkey(MOO_KEY_KP9 | MOO_MOD_CTRL);
    d->oi_ctrl_up = uiobj_add_inputkey(MOO_KEY_UP | MOO_MOD_CTRL);
    if (MOO_KEY_UP != MOO_KEY_KP8) {
        d->oi_ctrl_u2 = uiobj_add_inputkey(MOO_KEY_KP8 | MOO_MOD_CTRL);
    }
    d->oi_ctrl_dl = uiobj_add_inputkey(MOO_KEY_KP1 | MOO_MOD_CTRL);
    d->oi_ctrl_down = uiobj_add_inputkey(MOO_KEY_DOWN | MOO_MOD_CTRL);
    if (MOO_KEY_DOWN != MOO_KEY_KP2) {
        d->oi_ctrl_d2 = uiobj_add_inputkey(MOO_KEY_KP2 | MOO_MOD_CTRL);
    }
    d->oi_ctrl_dr = uiobj_add_inputkey(MOO_KEY_KP3 | MOO_MOD_CTRL);
    if (d->tags_enabled) {
        for (int i = 0; i < PLANET_TAG_NUM; ++i) {
            d->oi_tag_set[i] = uiobj_add_inputkey((MOO_KEY_1 + i) | MOO_MOD_CTRL);
            d->oi_tag_get[i] = uiobj_add_inputkey((MOO_KEY_1 + i));
        }
    }
}

void ui_starmap_sn0_setup(struct shipnon0_s *sn0, int sd_num, const shipcount_t *ships)
{
    int num = 0;
    for (int i = 0; i < sd_num; ++i) {
        shipcount_t n;
        n = ships[i];
        sn0->ships[num] = n;
        if (n) {
            sn0->type[num++] = i;
        }
    }
    sn0->num = num;
}

void ui_starmap_update_reserve_fuel(struct game_s *g, struct shipnon0_s *sn0, const shipcount_t *ships, player_id_t pi)
{
    const bool *hrf = &(g->srd[pi].have_reserve_fuel[0]);
    for (int i = 0; i < sn0->num; ++i) {
        int st;
        st = sn0->type[i];
        if ((!hrf[st]) && (ships[st] != 0)) {
            sn0->have_reserve_fuel = false;
            return;
        }
    }
    sn0->have_reserve_fuel = true;
}

void ui_starmap_draw_planetinfo(const struct game_s *g, player_id_t api, int planet_i)
{
    const planet_t *p = &(g->planet[planet_i]);
    ui_starmap_draw_planetinfo_do(g, api, planet_i, BOOLVEC_IS1(p->explored, api), true);
}

void ui_starmap_draw_planetinfo_2(const struct game_s *g, int p1, int p2, int planet_i)
{
    const planet_t *p = &(g->planet[planet_i]);
    player_id_t api = (p1 < PLAYER_NUM) ? p1 : p2;
    bool explored = true;
    if (0
      || ((p1 < PLAYER_NUM) && IS_HUMAN(g, p1) && BOOLVEC_IS0(p->explored, p1))
      || ((p2 < PLAYER_NUM) && IS_HUMAN(g, p2) && BOOLVEC_IS0(p->explored, p2))
    ) {
        explored = false;
    }
    ui_starmap_draw_planetinfo_do(g, api, planet_i, explored, false);
}

static bool ui_starmap_star_have_fleet(const struct game_s *g, player_id_t active_player, int star) {
    for (int j = 0; j < g->eto[active_player].shipdesigns_num; ++j) {
        if (g->eto[active_player].orbit[star].ships[j]) {
            return true;
        }
    }
    return false;
}

static int ui_starmap_newship_next(const struct game_s *g, player_id_t pi, int i)
{
    int t = i;
    const planet_t *p;
    do {
        i = (i + 1) % g->galaxy_stars;
        p = &(g->planet[i]);
        if (p->owner == pi && BOOLVEC_IS1(p->finished, FINISHED_SHIP) && ui_starmap_star_have_fleet(g, pi, i)) {
            return i;
        }
    } while (i != t);
    return PLANET_NONE;
}

static int ui_starmap_newship_prev(const struct game_s *g, player_id_t pi, int i)
{
    int t = i;
    const planet_t *p;
    do {
        if (--i < 0) { i = g->galaxy_stars - 1; }
        p = &(g->planet[i]);
        if (p->owner == pi && BOOLVEC_IS1(p->finished, FINISHED_SHIP) && ui_starmap_star_have_fleet(g, pi, i)) {
            return i;
        }
    } while (i != t);
    return PLANET_NONE;
}

static int ui_starmap_enemy_incoming(const struct game_s *g, player_id_t pi, int i, bool next)
{
    int t = i;
    do {
        if (next) {
            i = (i + 1) % g->galaxy_stars;
        } else {
            if (--i < 0) { i = g->galaxy_stars - 1; }
        }
        if (g->planet[i].owner == pi) {
           for (int j = 0; j < g->enroute_num; ++j) {
               const fleet_enroute_t *r = &(g->enroute[j]);
               if (BOOLVEC_IS1(r->visible, pi) && (r->owner != pi) && (r->dest == i)) {
                    return i;
                }
            }
            for (int j = 0; j < g->transport_num; ++j) {
                const transport_t *r = &(g->transport[j]);
                if (BOOLVEC_IS1(r->visible, pi) && (r->owner != pi) && (r->dest == i)) {
                    return i;
                }
            }
        }
    } while (i != t);
    return PLANET_NONE;
}

static void ui_starmap_scroll(const struct game_s *g, int scrollx, int scrolly, uint8_t scrollz)
{
    int x, y;
    if (scrollx >= 0) {
        x = ui_data.starmap.x + scrollx - 54;
        y = ui_data.starmap.y + scrolly - 43;
        SETRANGE(x, 0, g->galaxy_maxx - ((108 * ui_scale) / starmap_scale));
        SETRANGE(y, 0, g->galaxy_maxy - ((86 * ui_scale) / starmap_scale));
        ui_data.starmap.x2 = x;
        ui_data.starmap.y2 = y;
    } else {
        starmap_scale = scrollz;
        ui_starmap_set_pos_focus(g, g->active_player);
    }
}

/* compute maximum starmap_scale such that map is as big as possible and not clipped */
void ui_starmap_compute_scale(const struct game_s *g)
{
    int x = ((222 / 2 * ui_scale) / g->galaxy_maxx);
    int y = ((178 / 2 * ui_scale) / g->galaxy_maxy);
    starmap_scale = MIN(x, y);
    SETRANGE(starmap_scale, 1, ui_scale);
}

int ui_starmap_cursor_on_star(const struct game_s *g, const struct starmap_data_s *d, int16_t oi2, player_id_t active_player)
{
    if (oi2 == 0) {
        return PLANET_NONE;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        if (oi2 == d->oi_tbl_stars[i]) {
            return i;
        }
    }
    return PLANET_NONE;
}

int ui_starmap_cursor_on_enroute(const struct game_s *g, const struct starmap_data_s *d, int16_t oi2) {
    if (oi2 == 0) {
        return -1;
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        if (oi2 == d->oi_tbl_enroute[i]) {
            return i;
        }
    }
    return -1;
}

int ui_starmap_cursor_on_transport(const struct game_s *g, const struct starmap_data_s *d, int16_t oi2) {
    if (oi2 == 0) {
        return -1;
    }
    for (int i = 0; i < g->transport_num; ++i) {
        if (oi2 == d->oi_tbl_transport[i]) {
            return i;
        }
    }
    return -1;
}

int ui_starmap_cursor_on_orbit(const struct game_s *g, const struct starmap_data_s *d, int16_t oi2, player_id_t orbit_owner) {
    if (oi2 == 0) {
        return -1;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        if (oi2 == d->oi_tbl_pl_stars[orbit_owner][i]) {
            return i;
        }
    }
    return -1;
}

static int ui_starmap_bottom_menu_action(const struct game_s *g, const struct starmap_data_s *d, int16_t oi) {
    ui_main_loop_action_t action = -1;
    if (oi == d->oi_gameopts) {
        action = UI_MAIN_LOOP_GAMEOPTS;
    } else if (oi == d->oi_design) {
        action = UI_MAIN_LOOP_DESIGN;
    } else if (oi == d->oi_fleet) {
        action = UI_MAIN_LOOP_FLEET;
    } else if (oi == d->oi_map) {
        action = UI_MAIN_LOOP_MAP;
    } else if (oi == d->oi_races) {
        action = UI_MAIN_LOOP_RACES;
    } else if (oi == d->oi_planets) {
        action = UI_MAIN_LOOP_PLANETS;
    } else if (oi == d->oi_tech) {
        action = UI_MAIN_LOOP_TECH;
    } else if (oi == d->oi_next_turn) {
        action = UI_MAIN_LOOP_NEXT_TURN;
    }
    return action;
}

void ui_starmap_select_bottom_highlight(const struct game_s *g, struct starmap_data_s *d) {
    d->bottom_highlight = ui_starmap_bottom_menu_action(g, d, d->oi2) - 1;
}

static void ui_starmap_reloc_reloc(struct game_s *g, player_id_t active_player)
{
    uint8_t target = g->planet_focus_i[active_player];
    if (g->planet[target].within_frange[active_player] != 1) {
        return;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if ((p->owner == active_player) && (p->reloc != i)) {
            p->reloc = target;
        }
    }
}

static int ui_starmap_reloc_all(struct game_s *g, player_id_t active_player)
{
    uint8_t target = g->planet_focus_i[active_player];
    if (g->planet[target].within_frange[active_player] != 1) {
        return -1;
    }
    int count = 0;
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if (p->owner == active_player) {
            if (p->reloc != target) {
                ++count;
            }
            p->reloc = target;
        }
    }
    return count;
}

static int ui_starmap_reloc_un(struct game_s *g, player_id_t active_player)
{
    int count = 0;
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if (p->owner == active_player) {
            if (p->reloc != i) {
                ++count;
            }
            p->reloc = i;
        }
    }
    return count;
}

static int ui_starmap_previous_star_i(struct game_s *g, player_id_t active_player, int from) {
    int i;
    i = from;
    do {
        if (--i < 0) { i = g->galaxy_stars - 1; }
        if (g->planet[i].owner == active_player) {
            return i;
        }
    } while (i != from);
    return PLANET_NONE;
}

static int ui_starmap_next_star_i(struct game_s *g, player_id_t active_player, int from) {
    int i;
    i = from;
    do {
        i = (i + 1) % g->galaxy_stars;
        if (g->planet[i].owner == active_player) {
            return i;
        }
    } while (i != from);
    return PLANET_NONE;
}

static int ui_starmap_previous_fleet_i(struct game_s *g, player_id_t active_player, int from) {
    int i;
    i = from;
    do {
        i = (i + 1) % g->galaxy_stars;
        if (ui_starmap_star_have_fleet(g, active_player, i)) {
            return i;
        }
    } while (i != from);
    return PLANET_NONE;
}

static int ui_starmap_next_fleet_i(struct game_s *g, player_id_t active_player, int from) {
    int i;
    i = from;
    do {
        if (--i < 0) { i = g->galaxy_stars - 1; }
        if (ui_starmap_star_have_fleet(g, active_player, i)) {
            return i;
        }
    } while (i != from);
    return PLANET_NONE;
}

static void ui_starmap_reset_main_loop_action(struct game_s *g, struct starmap_data_s *d) {
    if (ui_data.ui_main_loop_action != UI_MAIN_LOOP_STARMAP) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        d->flag_done = true;
    }
}

static void ui_starmap_select_planet(struct game_s *g, struct starmap_data_s *d, int i) {
    if (i == PLANET_NONE) {
        return;
    }
    g->planet_focus_i[d->api] = i;
    d->from = i;
    if (d->is_valid_selection == NULL || !d->is_valid_selection(g, d, i)) {
        ui_starmap_reset_main_loop_action(g, d);
    } else {
        d->flag_done = true;
    }
}

static void ui_starmap_focus_planet(struct game_s *g, struct starmap_data_s *d, int i) {
    if (i == PLANET_NONE) {
        return;
    }
    d->lock_mouse_focus = true;
    g->planet_focus_i[d->api] = i;
    ui_starmap_set_pos_focus(g, d->api);
    ui_sound_play_sfx_24();
    if (!d->controllable) {
        ui_starmap_select_planet(g, d, i);
    }
}

static int ui_starmap_f4_f7_helper(struct starmap_data_s *d) {
    if (ui_data.ui_main_loop_action != UI_MAIN_LOOP_TRANS
      &&ui_data.ui_main_loop_action != UI_MAIN_LOOP_RELOC) {
        return d->from;
    } else {
        return d->g->planet_focus_i[d->api];
    }
}

static void ui_starmap_focus_own_fleet(struct game_s *g, struct starmap_data_s *d, int i) {
    if (i == PLANET_NONE) {
        return;
    }
    d->lock_mouse_focus = true;
    g->planet_focus_i[d->api] = i;
    ui_starmap_set_pos_focus(g, d->api);
    ui_sound_play_sfx_24();
    if (ui_data.ui_main_loop_action != UI_MAIN_LOOP_RELOC
      &&ui_data.ui_main_loop_action != UI_MAIN_LOOP_TRANS
      &&BOOLVEC_IS1(g->eto[d->api].orbit[i].visible, d->api))
    {
        d->from = i;
        ui_data.starmap.orbit_player = d->api;
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_ORBIT_OWN_SEL;
        d->flag_done = true;
    }
}

void ui_starmap_init_common_data(struct game_s *g, struct starmap_data_s *d, player_id_t active_player) {
    d->lock_mouse_focus = false;
    d->flag_done = false;
    d->scrollx = 0;
    d->scrolly = 0;
    d->scrollz = starmap_scale;
    d->g = g;
    d->api = active_player;
    d->focus_lock_planet_i = PLANET_NONE;
    d->anim_delay = 0;
    d->bottom_highlight = -1;
    d->gov_highlight = 0;
    d->from = g->planet_focus_i[active_player];

    d->can_select_fleet = true;
    d->tags_enabled = true;
    d->is_valid_selection = NULL;
}

bool ui_starmap_handle_common(struct game_s *g, struct starmap_data_s *d) {
    d->oi1 = uiobj_handle_input_cond();
    d->oi2 = uiobj_at_cursor();
    if (g->evn.build_finished_num[d->api]) {
        if ((d->oi1 == d->oi_build_finished) || ((d->oi1 == UIOBJI_ESC) && (d->oi2 == d->oi_build_finished))) {
            if (ui_starmap_remove_build_finished(g, d->api)) {
                g->planet_focus_i[d->api] = ui_data.start_planet_focus_i;
                ui_starmap_set_pos_focus(g, d->api);
            }
            ui_sound_play_sfx_24();
            d->flag_done = true;
            ui_delay_1();
            d->oi1 = 0;
            return true;
        } else if (d->oi1 == d->oi_cancel || d->oi1 == UIOBJI_ESC) {
            if (ui_data.ui_main_loop_action != UI_MAIN_LOOP_STARMAP) {
                ui_sound_play_sfx_06();
                ui_starmap_reset_main_loop_action(g, d);
                return true;
            }
        }
        return false;
    }
    if (d->oi1 == d->oi_f10) {
        game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", g);
        return true;
    }
    ui_starmap_handle_scrollkeys(d, d->oi1);
    if (d->oi1 == d->oi_scroll) {
        ui_starmap_scroll(g, d->scrollx, d->scrolly, d->scrollz);
        return true;
    }
    if (d->oi1 == d->oi_alt_c) {
        ui_starmap_set_pos_focus(g, d->api);
        ui_sound_play_sfx_24();
        return true;
    }
    if (d->oi1 == d->oi_alt_m) {
        ui_data.starmap.flag_show_grid = !ui_data.starmap.flag_show_grid;
        ui_sound_play_sfx_24();
        return true;
    }
    if (d->oi1 == d->oi_alt_r || d->oi1 == d->oi_ctrl_r) {
        if (ui_data.ui_main_loop_action == UI_MAIN_LOOP_ENROUTE_SEL &&
                g->enroute[ui_data.starmap.fleet_selected].owner != d->api) {
            return true;
        }
        if (ui_data.ui_main_loop_action == UI_MAIN_LOOP_TRANSPORT_SEL &&
                g->transport[ui_data.starmap.fleet_selected].owner != d->api) {
            return true;
        }
        if (d->oi1 == d->oi_alt_r) {
            ui_starmap_reloc_reloc(g, d->api);
        } else {
            ui_starmap_reloc_all(g, d->api) ||
            ui_starmap_reloc_un(g, d->api);
        }
        return true;
    }
    if (d->oi1 == d->oi_alt_f) {
        ui_show_routes = !ui_show_routes;
        return true;
    }
    if (d->oi1 == d->oi_alt_o) {
        ui_show_populations = !ui_show_populations;
        return true;
    }
    if (d->oi1 == d->oi_cancel || d->oi1 == UIOBJI_ESC) {
        if (ui_data.ui_main_loop_action != UI_MAIN_LOOP_STARMAP) {
            ui_sound_play_sfx_06();
            ui_starmap_reset_main_loop_action(g, d);
            return true;
        }
    }
    if (d->oi1 == d->oi_search) {
        ui_sound_play_sfx_24();
        int i = ui_search(g, d->api);
        if (i < 0) {
            d->flag_done = true;
            return true;
        }
        ui_starmap_focus_planet(g, d, i);
        d->flag_done = true;
        return true;
    }
    if (d->oi1 == d->oi_caught) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_SPIES_CAUGHT;
        d->flag_done = true;
        ui_sound_play_sfx_24();
        return true;
    }
    if (ui_extra_enabled && d->oi1 == d->oi_filter) {
        ui_data.ui_main_loop_action = UI_MAIN_LOOP_MSGFILTER;
        d->flag_done = true;
        ui_sound_play_sfx_24();
        return true;
    }
    if (d->oi1 == d->oi_f2) {
        int i = ui_starmap_previous_star_i(g, d->api, g->planet_focus_i[d->api]);
        ui_starmap_focus_planet(g, d, i);
        return true;
    }
    if (d->oi1 == d->oi_f3) {
        int i = ui_starmap_next_star_i(g, d->api, g->planet_focus_i[d->api]);
        ui_starmap_focus_planet(g, d, i);
        return true;
    }
    if (d->oi1 == d->oi_f8 && g->eto[d->api].have_ia_scanner) {
        int i = ui_starmap_enemy_incoming(g, d->api, g->planet_focus_i[d->api], true);
        ui_starmap_focus_planet(g, d, i);
        return true;
    }
    if (d->oi1 == d->oi_f9 && g->eto[d->api].have_ia_scanner) {
        int i = ui_starmap_enemy_incoming(g, d->api, g->planet_focus_i[d->api], false);
        ui_starmap_focus_planet(g, d, i);
        return true;
    }
    if (d->oi1 == d->oi_f4) {
        int i = ui_starmap_previous_fleet_i(g, d->api, ui_starmap_f4_f7_helper(d));
        ui_starmap_focus_own_fleet(g, d, i);
        return true;
    }
    if (d->oi1 == d->oi_f5) {
        int i = ui_starmap_next_fleet_i(g, d->api, ui_starmap_f4_f7_helper(d));
        ui_starmap_focus_own_fleet(g, d, i);
        return true;
    }
    if (d->oi1 == d->oi_f6) {
        int i = ui_starmap_newship_next(g, d->api, ui_starmap_f4_f7_helper(d));
        ui_starmap_focus_own_fleet(g, d, i);
        return true;
    }
    if (d->oi1 == d->oi_f7) {
        int i = ui_starmap_newship_prev(g, d->api, ui_starmap_f4_f7_helper(d));
        ui_starmap_focus_own_fleet(g, d, i);
        return true;
    }
    {
        int i = ui_starmap_handle_tag(d, d->oi1);
        if (i != PLANET_NONE) {
            ui_starmap_focus_planet(g, d, i);
            return true;
        }
    }
    ui_main_loop_action_t action = ui_starmap_bottom_menu_action(g, d, d->oi1);
    if (action != -1) {
        ui_data.ui_main_loop_action = action;
        ui_sound_play_sfx_24();
        d->flag_done = true;
        return true;
    }
    if (d->can_select_fleet) {
        int i = ui_starmap_cursor_on_enroute(g, d, d->oi1);
        if (i != -1) {
            ui_data.starmap.fleet_selected = i;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_ENROUTE_SEL;
            ui_sound_play_sfx_24();
            d->flag_done = true;
            return true;
        }
        i = ui_starmap_cursor_on_transport(g, d, d->oi1);
        if (i != -1) {
            ui_data.starmap.fleet_selected = i;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_TRANSPORT_SEL;
            ui_sound_play_sfx_24();
            d->flag_done = true;
            return true;
        }
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            i = ui_starmap_cursor_on_orbit(g, d, d->oi1, j);
            if (i != -1) {
                g->planet_focus_i[d->api] = i;  /* not done on MOO1! */
                d->from = i;
                ui_data.starmap.orbit_player = j;
                ui_data.ui_main_loop_action = (j == d->api) ? UI_MAIN_LOOP_ORBIT_OWN_SEL : UI_MAIN_LOOP_ORBIT_EN_SEL;
                ui_sound_play_sfx_24();
                d->flag_done = true;
                return true;
            }
        }
    }
    ui_starmap_handle_oi_ctrl(d, d->oi1);
    if (d->oi1 == d->oi_accept && d->controllable) {
        ui_sound_play_sfx_24();
        if (d->is_valid_selection == NULL || d->is_valid_selection(g, d, g->planet_focus_i[d->api])) {
            d->do_accept(g, d);
            ui_starmap_reset_main_loop_action(g, d);
            return true;
        }
    }
    if (d->controllable) {
        int i;
        i = ui_starmap_cursor_on_star(g, d, d->oi1, d->api);
        if ((ui_modern_starmap_controls || i == g->planet_focus_i[d->api]) && i != PLANET_NONE) {
            if (d->is_valid_selection == NULL || d->is_valid_selection(g, d, i)) {
                g->planet_focus_i[d->api] = i;
                d->do_accept(g, d);
                ui_starmap_reset_main_loop_action(g, d);
            }
            ui_sound_play_sfx_24();
            return true;
        } else if (i != PLANET_NONE) {
            ui_sound_play_sfx_24();
            g->planet_focus_i[d->api] = i;
            return true;
        }
    }
    if (!d->controllable) {
        if (ui_data.ui_main_loop_action == UI_MAIN_LOOP_STARMAP
          &&d->oi1 == d->oi_tbl_stars[g->planet_focus_i[d->api]]) {
            return false;   //TODO: accept handler for uistarmap.c
        }
        int i;
        i = ui_starmap_cursor_on_star(g, d, d->oi1, d->api);
        if (i != PLANET_NONE) {
            ui_sound_play_sfx_24();
            ui_starmap_select_planet(g, d, i);
            return true;
        }
    }
    if (ui_modern_starmap_controls && d->controllable) {
        int i;
        i = ui_starmap_cursor_on_star(g, d, d->oi2, d->api);
        if (i != d->focus_lock_planet_i) {
            d->focus_lock_planet_i = PLANET_NONE;
            if (i != PLANET_NONE) {
                g->planet_focus_i[d->api] = i;
            }
        }
    }
    return false;
}

void ui_starmap_fill_oi_common(struct starmap_data_s *d) {
    if (d->g->evn.build_finished_num[d->api]) {
        d->oi_build_finished = uiobj_add_mousearea(6, 6, 225, 180, MOO_KEY_SPACE);
        ui_cursor_setup_area(2, &ui_cursor_area_tbl[0]);
        return;
    } else if (d->controllable) {
        ui_cursor_setup_area(2, &ui_cursor_area_tbl[1]);
    } else {
        ui_cursor_setup_area(2, &ui_cursor_area_tbl[3]);
    }
    ui_starmap_add_oi_hotkeys(d);
    if (ui_data.ui_main_loop_action != UI_MAIN_LOOP_RELOC
      &&ui_data.ui_main_loop_action != UI_MAIN_LOOP_TRANS)
    {
        ui_starmap_fill_oi_tbls(d);
    }
    ui_starmap_fill_oi_tbl_stars(d);
    d->oi_scroll = uiobj_add_tb(6, 6, 2, 2, 108, 86, &d->scrollx, &d->scrolly, &d->scrollz, ui_scale);
    ui_starmap_fill_oi_ctrl(d);
    ui_starmap_add_oi_bottom_buttons(d);
    if (d->lock_mouse_focus) {
        d->focus_lock_planet_i = ui_starmap_cursor_on_star(d->g, d, uiobj_at_cursor(), d->api);
        d->lock_mouse_focus = false;
    }
}
