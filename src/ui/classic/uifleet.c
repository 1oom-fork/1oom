#include "config.h"

#include <stdio.h>

#include "uifleet.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_misc.h"
#include "game_str.h"
#include "hw.h"
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

#define FLEET_LINES 5

struct fleet_data_s {
    struct game_s *g;
    player_id_t api;
    int selected;
    int pos;
    int num;
    int lines;
    int frame;
    uint8_t planet[FLEET_ENROUTE_MAX];
    uint16_t enroute[FLEET_ENROUTE_MAX];
    BOOLVEC_DECLARE(is_enroute, FLEET_ENROUTE_MAX);
    uint8_t *gfx_fleetbrb;
};

static void load_fl_data(struct fleet_data_s *d)
{
    d->gfx_fleetbrb = lbxfile_item_get(LBXFILE_BACKGRND, 0);
}

static void free_fl_data(struct fleet_data_s *d)
{
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_fleetbrb);
}

static void fleet_draw_cb(void *vptr)
{
    struct fleet_data_s *d = vptr;
    struct game_s *g = d->g;
    empiretechorbit_t *e = &(g->eto[d->api]);
    shipdesign_t *sd = &(g->srd[d->api].design[0]);
    int num;

    ui_draw_filled_rect(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, 0x3a);
    ui_draw_filled_rect(5, 15, 40, 190, 0);
    lbxgfx_draw_frame(0, 0, d->gfx_fleetbrb, UI_SCREEN_W);
    lbxgfx_set_new_frame(ui_data.gfx.starmap.fleetbut_scrap, 1);
    lbxgfx_draw_frame(224, 181, ui_data.gfx.starmap.fleetbut_scrap, UI_SCREEN_W);
    lbxgfx_set_new_frame(ui_data.gfx.starmap.fleetbut_up, 1);
    lbxgfx_set_new_frame(ui_data.gfx.starmap.fleetbut_down, 1);
    lbxgfx_draw_frame(307, 24, ui_data.gfx.starmap.fleetbut_up, UI_SCREEN_W);
    lbxgfx_draw_frame(307, 156, ui_data.gfx.starmap.fleetbut_down, UI_SCREEN_W);

    lbxfont_select_set_12_4(0, 0, 0, 0);
    lbxfont_print_str_normal(7, 6, game_str_fl_station, UI_SCREEN_W);
    num = d->num - d->pos;
    SETMIN(num, 5);

    for (int i = 0; i < e->shipdesigns_num; ++i) {
        int x0;
        x0 = 44 * i + 48;
        lbxfont_select(2, 0xd, 0, 0);
        lbxfont_print_str_center(x0 + 19, 6, sd[i].name, UI_SCREEN_W);
    }
    for (int i = 0; i < num; ++i) {
        planet_t *p;
        uint16_t *s;
        int x0 = 5, y0, pi, fi;
        y0 = 33 * i + 17;
        fi = i + d->pos;
        pi = d->planet[fi];
        p = &(g->planet[pi]);
        if (BOOLVEC_IS1(p->explored, d->api)) {
            player_id_t seenowner;
            uint8_t a2;
            lbxgfx_draw_frame(x0 + 1, y0, ui_data.gfx.planets.planet[p->infogfx], UI_SCREEN_W);
            seenowner = g->seen[d->api][pi].owner;
            if (seenowner != PLAYER_NONE) {
                a2 = tbl_banner_fontparam[g->eto[seenowner].banner];
            } else {
                a2 = 4;
            }
            lbxfont_select(2, a2, 0, 0);
            lbxfont_print_str_center(x0 + 19, y0 + 12, BOOLVEC_IS1(d->is_enroute, fi) ? game_str_fl_moving : game_str_fl_inorbit, UI_SCREEN_W);
            lbxfont_print_str_center(x0 + 19, y0 + 19, p->name, UI_SCREEN_W);
        } else {
            lbxfont_select_set_12_4(2, 0xe, 0, 0);
            lbxfont_print_str_center(x0 + 19, y0 + 7, game_str_fl_unknown, UI_SCREEN_W);
            lbxfont_print_str_center(x0 + 19, y0 + 14, game_str_fl_system, UI_SCREEN_W);
        }
        s = (BOOLVEC_IS1(d->is_enroute, fi)) ? g->enroute[d->enroute[fi]].ships : e->orbit[pi].ships;
        for (int j = 0; j < e->shipdesigns_num; ++j) {
            int ships;
            ships = s[j];
            if (ships) {
                uint8_t *gfx_ship;
                x0 = 44 * j + 48;
                ui_draw_filled_rect(x0, y0, x0 + 36, y0 + 25, 0);
                if (BOOLVEC_IS0(d->is_enroute, fi)) {
                    int tmp_xoff1 = ui_data.starmap.stars_xoff1;
                    int tmp_xoff2 = ui_data.starmap.stars_xoff2;
                    ui_data.starmap.stars_xoff1 = 0;
                    ui_data.starmap.stars_xoff2 = 0;
                    ui_draw_stars(x0, y0 + 1, j * 10, 37);
                    ui_data.starmap.stars_xoff1 = tmp_xoff1;
                    ui_data.starmap.stars_xoff2 = tmp_xoff2;
                } else {
                    ui_draw_stars(x0, y0 + 1, j * 5, 37);
                }
                gfx_ship = ui_data.gfx.ships[sd[j].look];
                lbxgfx_set_frame_0(gfx_ship);
                if (BOOLVEC_IS0(d->is_enroute, fi)) {
                    lbxgfx_draw_frame(x0, y0 + 1, gfx_ship, UI_SCREEN_W);
                } else {
                    for (int f = 0; f <= d->frame; ++f) {
                        lbxgfx_draw_frame(x0, y0 + 1, gfx_ship, UI_SCREEN_W);
                    }
                }
                lbxfont_select(0, 0xd, 0, 0);
                lbxfont_print_num_right(x0 + 31, y0 + 19, ships, UI_SCREEN_W);
            }
        }
    }
    for (int i = num; i < FLEET_LINES; ++i) {
        ui_draw_filled_rect(7, i * 33 + 17, 40, i * 33 + 42, 0x3a);
    }
    d->frame = (d->frame + 1) % 5;
    ui_draw_set_stars_xoffs(false);
    lbxfont_select(2, 6, 0, 0);
    lbxfont_print_num_right(137, 185, e->ship_maint_bc, UI_SCREEN_W);
}

static void ui_fleet_sub(struct fleet_data_s *d)
{
    struct game_s *g = d->g;
    empiretechorbit_t *e = &(g->eto[d->api]);
    int num = 0, sd_num = e->shipdesigns_num;
    for (int i = 0; i < g->galaxy_stars; ++i) {
        fleet_orbit_t *r = &(e->orbit[i]);
        for (int j = 0; j < sd_num; ++j) {
            if (r->ships[j] != 0) {
                d->planet[num] = i;
                BOOLVEC_SET0(d->is_enroute, num);
                ++num;
                break;
            }
        }
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        fleet_enroute_t *r = &(g->enroute[i]);
        if (r->owner == d->api) {
            for (int j = 0; j < sd_num; ++j) {
                if (r->ships[j] != 0) {
                    d->enroute[num] = i;
                    d->planet[num] = r->dest;
                    BOOLVEC_SET1(d->is_enroute, num);
                    ++num;
                    break;
                }
            }
        }
    }
    d->num = num;
}

/* -------------------------------------------------------------------------- */

int ui_fleet(struct game_s *g, player_id_t active_player)
{
    struct fleet_data_s d;
    bool flag_done = false, flag_scrap = false;
    int16_t oi_up, oi_down, oi_wheel, oi_ok, oi_scrap, oi_view, oi_tbl_ship[NUM_SHIPDESIGNS], oi_tbl_line[FLEET_LINES];
    int ret = -1;
    int16_t scroll = 0;

    load_fl_data(&d);

    d.g = g;
    d.api = active_player;
    d.pos = 0;
    d.num = 0;
    game_update_maint_costs(g);
    ui_fleet_sub(&d);
    d.frame = 0;
    d.lines = 0;

    uiobj_table_clear();

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        oi_up = UIOBJI_INVALID; \
        oi_down = UIOBJI_INVALID; \
        oi_wheel = UIOBJI_INVALID; \
        oi_ok = UIOBJI_INVALID; \
        oi_view = UIOBJI_INVALID; \
        oi_scrap = UIOBJI_INVALID; \
        for (int i = 0; i < FLEET_LINES; ++i) { \
            oi_tbl_line[i] = UIOBJI_INVALID; \
        } \
        for (int i = 0; i < NUM_SHIPDESIGNS; ++i) { \
            oi_tbl_ship[i] = UIOBJI_INVALID; \
        } \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(8);
    uiobj_set_callback_and_delay(fleet_draw_cb, &d, 3);

    while (!flag_done) {
        int16_t oi;
        ui_cursor_setup_area(1, &ui_cursor_area_tbl[flag_scrap ? 0xa : 0]);
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == oi_ok) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_24();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        } else if (oi == oi_view) {
            ui_sound_play_sfx_24();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_SPECS;
        } else if (oi == oi_scrap) {
            ui_sound_play_sfx_24();
            flag_scrap = true;
        }
        for (int i = 0; i < g->eto[active_player].shipdesigns_num; ++i) {
            if (oi == oi_tbl_ship[i]) {
                ui_sound_play_sfx_24();
                ret = i;
                ui_data.ui_main_loop_action = UI_MAIN_LOOP_MUSTSCRAP;
                ui_data.ui_main_loop_action_prev = UI_MAIN_LOOP_FLEET;
                ui_data.ui_main_loop_action_next = UI_MAIN_LOOP_FLEET;
                flag_done = true;
                break;
            }
        }
        if (oi == oi_up) {
            ui_sound_play_sfx_24();
            d.pos -= FLEET_LINES;
            SETMAX(d.pos, 0);
        } else if (oi == oi_down) {
            ui_sound_play_sfx_24();
            d.pos += FLEET_LINES;
            if ((d.pos + FLEET_LINES) > d.num) {
                d.pos = d.num - FLEET_LINES;
            }
            SETMAX(d.pos, 0);
        } else if (oi == oi_wheel) {
            d.pos += scroll;
            scroll = 0;
            SETRANGE(d.pos, 0, d.num - FLEET_LINES);
        }
        for (int i = 0; i < d.lines; ++i) {
            if (oi == oi_tbl_line[i]) {
                ui_sound_play_sfx_24();
                if (BOOLVEC_IS0(d.is_enroute, d.pos + i)) {
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_ORBIT_OWN_SEL;
                    g->planet_focus_i[active_player] = d.planet[d.pos + i];
                } else {
                    ui_data.ui_main_loop_action = UI_MAIN_LOOP_ENROUTE_SEL;
                    ui_data.starmap.fleet_selected = d.enroute[d.pos + i];
                }
                flag_done = true;
                break;
            }
        }
        if (!flag_done) {
            if ((d.pos + FLEET_LINES) > d.num) {
                d.pos = d.num - FLEET_LINES;
            }
            SETMAX(d.pos, 0);
            d.lines = d.num - d.pos;
            SETMIN(d.lines, FLEET_LINES);

            fleet_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();

            oi_ok = uiobj_add_t0(268, 181, "", ui_data.gfx.starmap.fleetbut_ok, MOO_KEY_SPACE);
            oi_view = uiobj_add_t0(180, 181, "", ui_data.gfx.starmap.fleetbut_view, MOO_KEY_v);
            if (d.pos > 0) {
                oi_up = uiobj_add_t0(307, 24, "", ui_data.gfx.starmap.fleetbut_up, MOO_KEY_COMMA);
            }
            if ((d.num - 5) > d.pos) {
                oi_down = uiobj_add_t0(307, 156, "", ui_data.gfx.starmap.fleetbut_down, MOO_KEY_PERIOD);
            }
            if ((g->eto[active_player].shipdesigns_num > 1) && !flag_scrap) {
                oi_scrap = uiobj_add_t0(224, 181, "", ui_data.gfx.starmap.fleetbut_scrap, MOO_KEY_s);
            } else {
                lbxgfx_set_new_frame(ui_data.gfx.starmap.fleetbut_scrap, 1);
                lbxgfx_draw_frame(224, 181, ui_data.gfx.starmap.fleetbut_scrap, UI_SCREEN_W);
            }
            if (flag_scrap) {
                for (int i = 0; i < g->eto[active_player].shipdesigns_num; ++i) {
                    int x0;
                    x0 = 44 * i + 48;
                    oi_tbl_ship[i] = uiobj_add_mousearea(x0, 0, x0 + 38, 180, MOO_KEY_UNKNOWN);
                }
            }
            for (int i = 0; i < d.lines; ++i) {
                oi_tbl_line[i] = uiobj_add_mousearea(5, 15 + i * 33, 288, 44 + i * 33, MOO_KEY_UNKNOWN);
            }
            if (d.num >= FLEET_LINES) {
                oi_wheel = uiobj_add_mousewheel(0, 0, 319, 199, &scroll);
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(3);
        }
    }

    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    free_fl_data(&d);
    return ret;
}
