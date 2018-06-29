#include "config.h"

#include <stdio.h>

#include "uiplanets.h"
#include "comp.h"
#include "game.h"
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

#define PLANETS_ON_SCREEN   12

struct planets_data_s {
    player_id_t api;
    int selected;
    int pos;
    int num;
    int16_t amount_trans;
    int planet_i;
    struct game_s *g;
    uint8_t planets[PLANETS_MAX];
    uint8_t *gfx_report;
    uint8_t *gfx_but_trans;
    uint8_t *gfx_but_ok;
    uint8_t *gfx_transfer;
};

static void load_pl_data(struct planets_data_s *d)
{
    d->gfx_report = lbxfile_item_get(LBXFILE_BACKGRND, 1, 0);
    d->gfx_but_trans = lbxfile_item_get(LBXFILE_BACKGRND, 0x1c, 0);
    d->gfx_but_ok = lbxfile_item_get(LBXFILE_BACKGRND, 0x1d, 0);
    d->gfx_transfer = lbxfile_item_get(LBXFILE_BACKGRND, 0x1e, 0);
}

static void free_pl_data(struct planets_data_s *d)
{
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_report);
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_but_trans);
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_but_ok);
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_transfer);
}

static void planets_draw_cb(void *vptr)
{
    struct planets_data_s *d = vptr;
    struct game_s *g = d->g;
    empiretechorbit_t *e = &(g->eto[d->api]);
    char buf[64];
    int v;

    ui_draw_filled_rect(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, 0x5d);
    for (int i = 0; i < PLANETS_ON_SCREEN; ++i) {
        int pi;
        pi = d->pos + i;
        if ((pi < d->num) && (d->planets[pi] == g->planet_focus_i[d->api])) {
            int y0, y1;
            y0 = 21 + i * 11;
            y1 = y0 + 6;
            ui_draw_filled_rect(8, y0, 309, y1, 0xeb);
        }
    }
    lbxgfx_draw_frame(0, 0, d->gfx_report, UI_SCREEN_W);
    lbxgfx_set_new_frame(d->gfx_but_trans, 1);
    lbxgfx_draw_frame(209, 181, d->gfx_but_trans, UI_SCREEN_W);
    ui_draw_filled_rect(213, 159, 245, 162, 0x2f);

    v = g->eto[d->api].tax;
    if (v > 0) {
        ui_draw_line_3h(213, 160, 212 + (v * 4) / 25, 0x73);
    }

    lbxfont_select(2, 8, 0, 0);
    lbxfont_print_str_normal(210, 170, game_str_pl_reserve, UI_SCREEN_W);
    lbxfont_select(2, 6, 0, 0);
    v = (e->tax * e->total_production_bc) / 2000;
    sprintf(buf, "+%i", v);
    lbxfont_print_str_right(272, 159, buf, UI_SCREEN_W);
    lbxfont_print_str_normal(276, 159, game_str_bc, UI_SCREEN_W);
    lbxfont_print_str_normal(276, 170, game_str_bc, UI_SCREEN_W);
    lbxfont_print_num_right(272, 170, e->reserve_bc, UI_SCREEN_W);

    for (int i = 0; i < PLANETS_ON_SCREEN; ++i) {
        int pi;
        pi = d->pos + i;
        if (pi < d->num) {
            int y0;
            planet_t *p;
            const char *str;
            uint8_t pli;
            pli = d->planets[pi];
            y0 = 21 + i * 11 + 1;   /* di + 1 */
            p = &(g->planet[pli]);
            lbxfont_select(2, 0xb, 0, 0);
            lbxfont_print_num_right(17, y0, pi + 1, UI_SCREEN_W);
            lbxfont_select(2, 0xd, 0, 0);
            lbxfont_print_str_normal(25, y0, p->name, UI_SCREEN_W);
            lbxfont_select(2, 6, 0, 0);
            lbxfont_print_num_right(83, y0, p->pop, UI_SCREEN_W);
            if (p->pop != p->pop_prev) {
                int v;
                char c;
                uint8_t *gfx;
                if (p->pop > p->pop_prev) {
                    v = p->pop - p->pop_prev;
                    c = '+';
                    gfx = ui_data.gfx.starmap.gr_arrow_u;
                } else {
                    v = p->pop_prev - p->pop;
                    c = '-';
                    gfx = ui_data.gfx.starmap.gr_arrow_d;
                }
                lbxgfx_draw_frame(92, y0 - 1, gfx, UI_SCREEN_W);
                sprintf(buf, "%c%i", c, v);
                lbxfont_print_str_right(111, y0, buf, UI_SCREEN_W);
            }
            lbxfont_print_str_right(149, y0, game_str_tbl_roman[p->shield], UI_SCREEN_W);
            /*lbxfont_select(2, 6, 0, 0);*/
            lbxfont_print_num_right(132, y0, p->factories, UI_SCREEN_W);
            lbxfont_print_num_right(170, y0, p->missile_bases, UI_SCREEN_W);
            if (p->waste) {
                lbxfont_print_num_right(189, y0, p->waste, UI_SCREEN_W);
            }
            lbxfont_select(0, 0xe, 0, 0);
            if (p->unrest == PLANET_UNREST_REBELLION) {
                v = 0;
            } else {
                v = p->total_prod;
            }
            lbxfont_print_num_right(214, y0, v, UI_SCREEN_W);
            str = NULL;
            lbxfont_select(2, 0xb, 0, 0);
            if (g->evn.have_plague && (g->evn.plague_planet_i == pli)) {
                str = game_str_pl_plague;
            } else if (g->evn.have_nova && (g->evn.nova_planet_i == pli)) {
                str = game_str_pl_nova;
            } else if (g->evn.have_comet && (g->evn.comet_planet_i == pli)) {
                str = game_str_pl_comet;
            } else if (g->evn.have_pirates && (g->evn.pirates_planet_i == pli)) {
                str = game_str_pl_pirates;
            } else if (p->unrest == PLANET_UNREST_REBELLION) {
                str = game_str_pl_rebellion;
            } else if (p->unrest == PLANET_UNREST_UNREST) {
                str = game_str_pl_unrest;
            } else if (g->evn.have_accident && (g->evn.accident_planet_i == pli)) {
                str = game_str_pl_accident;
            } else {
                lbxfont_select(2, 1, 0, 0);
                if (p->special != PLANET_SPECIAL_NORMAL) {
                    str = game_str_tbl_sm_pspecial[p->special];
                } else if (p->growth != PLANET_GROWTH_NORMAL) {
                    str = game_str_tbl_sm_pgrowth[p->growth];
                }
                if (p->have_stargate) {
                    if (str) {
                        strcpy(buf, str);
                        strcat(buf, " *");
                        str = buf;
                    } else {
                        str = game_str_sm_stargate;
                    }
                }
            }
            if (str) {
                lbxfont_print_str_normal(268, y0, str, UI_SCREEN_W);
            }
            if (p->slider[PLANET_SLIDER_SHIP] > 0) {
                if (p->buildship == BUILDSHIP_STARGATE) {
                    str = game_str_sm_stargate;
                } else {
                    str = g->srd[d->api].design[p->buildship].name;
                }
                lbxfont_print_str_normal(221, y0, str, UI_SCREEN_W);
            }
        }
    }

    lbxfont_select(2, 6, 0, 0);
    game_print_prod_of_total(g, e->ship_maint_bc, buf);
    lbxfont_print_str_right(59, 174, buf, UI_SCREEN_W);
    game_print_prod_of_total(g, e->bases_maint_bc, buf);
    lbxfont_print_str_right(59, 185, buf, UI_SCREEN_W);

    v = 0;
    for (player_id_t i = PLAYER_0; i < PLAYER_NUM; ++i) {
        if (i != d->api) {
            v += e->spying[i];
        }
    }
    sprintf(buf, "%i.%i%%", v / 10, v % 10);
    lbxfont_print_str_right(116, 174, buf, UI_SCREEN_W);
    v = e->security;
    sprintf(buf, "%i.%i%%", v / 10, v % 10);
    lbxfont_print_str_right(116, 185, buf, UI_SCREEN_W);
    sprintf(buf, "%i %s", e->total_trade_bc, game_str_bc);
    lbxfont_print_str_right(195, 174, buf, UI_SCREEN_W);
    sprintf(buf, "%i %s", e->total_production_bc, game_str_bc);
    lbxfont_print_str_right(195, 185, buf, UI_SCREEN_W);

    lbxfont_select_set_12_1(5, 8, 0, 0);
    lbxfont_print_str_center(66, 161, game_str_pl_spending, UI_SCREEN_W);
    lbxfont_print_str_center(163, 161, game_str_pl_tincome, UI_SCREEN_W);
}

static void planets_transfer_draw_cb(void *vptr)
{
    struct planets_data_s *d = vptr;
    struct game_s *g = d->g;
    planet_t *p = &(g->planet[d->planet_i]);
    empiretechorbit_t *e = &(g->eto[d->api]);
    const int x = 100, y = 50;
    char buf[64];

    hw_video_copy_back_from_page2();
    lbxgfx_draw_frame(x, y, d->gfx_transfer, UI_SCREEN_W);
    ui_draw_filled_rect(x + 14, y + 35, x + 64, y + 38, 0x2f);
    if (d->amount_trans > 0) {
        ui_draw_line_3h(x + 14, y + 35, x + 13 + d->amount_trans / 2, 0x74);
    }
    sprintf(buf, "%s %s", game_str_pl_resto, p->name);
    lbxfont_select(0, 0xd, 0, 0);
    lbxfont_print_str_center(x + 57, y + 11, game_str_pl_transof, UI_SCREEN_W);
    lbxfont_print_str_center(x + 57, y + 20, buf, UI_SCREEN_W);
    lbxfont_select(2, 6, 0, 0);
    lbxfont_print_str_right(x + 104, y + 35, game_str_bc, UI_SCREEN_W);
    {
        int v;
        v = (d->amount_trans * e->reserve_bc) / 100;
        lbxfont_print_num_right(x + 95, y + 35, v, UI_SCREEN_W);
    }
}

static void ui_planets_transfer(struct planets_data_s *d)
{
    struct game_s *g = d->g;
    planet_t *p = &(g->planet[d->planet_i]);
    int16_t oi_cancel, oi_accept, oi_minus, oi_plus, oi_equals /*, oi_slider*/;
    int prod, allreserve, v;
    const int x = 100, y = 50;
    bool flag_done = false;

    d->amount_trans = 0;
    ui_draw_copy_buf();
    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);
    lbxgfx_set_new_frame(d->gfx_but_ok, 1);
    lbxgfx_set_new_frame(d->gfx_but_trans, 1);
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reprtbut_up, 1);
    lbxgfx_set_new_frame(ui_data.gfx.starmap.reprtbut_down, 1);
    lbxgfx_draw_frame(292, 159, ui_data.gfx.starmap.reprtbut_up, UI_SCREEN_W);
    lbxgfx_draw_frame(292, 176, ui_data.gfx.starmap.reprtbut_down, UI_SCREEN_W);
    lbxgfx_draw_frame(256, 181, d->gfx_but_ok, UI_SCREEN_W);
    lbxgfx_draw_frame(209, 181, d->gfx_but_trans, UI_SCREEN_W);
    hw_video_copy_back_to_page2();

    prod = p->prod_after_maint - p->reserve;
    SETMAX(prod, 0);
    allreserve = g->eto[d->api].reserve_bc;
    v = allreserve;
    if (v != 0) {
        v = (prod * 100) / v;
    }

    uiobj_table_clear();
    oi_cancel = uiobj_add_t0(x + 10, y + 47, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_LEFT, -1); /* FIXME key == "\x01xb" ?? */
    oi_accept = uiobj_add_t0(x + 66, y + 47, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE, -1);
    /*oi_slider =*/ uiobj_add_slider(x + 14, y + 35, 0, 100, 0, 100, 50, 9, &d->amount_trans, MOO_KEY_UNKNOWN, -1);
    oi_minus = uiobj_add_mousearea(x + 10, y + 33, x + 12, y + 41, MOO_KEY_UNKNOWN, -1);
    oi_plus = uiobj_add_mousearea(x + 66, y + 33, x + 70, y + 41, MOO_KEY_UNKNOWN, -1);
    oi_equals = uiobj_add_inputkey(MOO_KEY_EQUALS);

    uiobj_set_callback_and_delay(planets_transfer_draw_cb, d, 1);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == oi_cancel) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
        } else if (oi == oi_accept) {
            ui_sound_play_sfx_24();
            v = (d->amount_trans * allreserve) / 100;
            p->reserve += v;
            allreserve -= v;
            SETMAX(allreserve, 0);
            g->eto[d->api].reserve_bc = allreserve;
            flag_done = true;
        } else if (oi == oi_minus) {
            ui_sound_play_sfx_24();
            d->amount_trans -= 2;
            SETMAX(d->amount_trans, 0);
        } else if (oi == oi_plus) {
            ui_sound_play_sfx_24();
            d->amount_trans += 2;
            SETMIN(d->amount_trans, 100);
        } else if (oi == oi_equals) {
            ui_sound_play_sfx_24();
            d->amount_trans = v;
            SETRANGE(d->amount_trans, 0, 100);
        }
        if (!flag_done) {
            planets_transfer_draw_cb(d);
            ui_draw_finish();
            ui_delay_ticks_or_click(1);
        }
    }
}

/* -------------------------------------------------------------------------- */

void ui_planets(struct game_s *g, player_id_t active_player)
{
    struct planets_data_s d;
    bool flag_done = false, flag_trans;
    int16_t oi_alt_moola, oi_up, oi_down, oi_ok, oi_trans, oi_minus, oi_plus, oi_tbl_planets[PLANETS_ON_SCREEN] /*, oi_slider*/;
    uint8_t tbl_onscreen_planets[PLANETS_ON_SCREEN];

    load_pl_data(&d);
    uiobj_set_help_id(37);
again:
    flag_trans = false;
    game_update_production(g); /* XXX this probably needed for alt-moola */

    d.g = g;
    d.api = active_player;
    d.selected = -1;
    d.pos = 0;
    d.num = 0;

    for (int i = 0; i < g->galaxy_stars; ++i) {
        if (g->planet[i].owner == active_player) {
            if (i == g->planet_focus_i[active_player]) {
                d.pos = i - 5;
                d.selected = d.num - d.pos;
            }
            d.planets[d.num++] = i;
        }
    }

    oi_up = UIOBJI_INVALID;
    oi_down = UIOBJI_INVALID;
    oi_ok = UIOBJI_INVALID;
    oi_trans = UIOBJI_INVALID;
    oi_minus = UIOBJI_INVALID;
    oi_plus = UIOBJI_INVALID;
    /*oi_slider = UIOBJI_INVALID;*/
    for (int i = 0; i < PLANETS_ON_SCREEN; ++i) {
        tbl_onscreen_planets[i] = 0;
        oi_tbl_planets[i] = UIOBJI_INVALID;
    }

    uiobj_set_callback_and_delay(planets_draw_cb, &d, 2);

    uiobj_table_clear();
    oi_alt_moola = uiobj_add_alt_str("moola");

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == oi_ok) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
        } else if (oi == oi_up) {
            ui_sound_play_sfx_24();
            d.pos -= PLANETS_ON_SCREEN;
        } else if (oi == oi_down) {
            ui_sound_play_sfx_24();
            d.pos += PLANETS_ON_SCREEN;
        }
        for (int i = 0; i < PLANETS_ON_SCREEN; ++i) {
            if (oi == oi_tbl_planets[i]) {
                ui_sound_play_sfx_24();
                if (!flag_trans) {
                    g->planet_focus_i[active_player] = tbl_onscreen_planets[i];
                    flag_done = true;
                } else {
                    d.planet_i = tbl_onscreen_planets[i];
                    ui_planets_transfer(&d);
                    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);
                    goto again;
                }
            }
        }
        if (oi == oi_alt_moola) {
            ui_sound_play_sfx_24();
            g->eto[active_player].reserve_bc += 100;
        }
        if ((d.pos + PLANETS_ON_SCREEN) >= d.num) {
            d.pos = d.num - PLANETS_ON_SCREEN;
        }
        SETMAX(d.pos, 0);
        if (oi == oi_minus) {
            ui_sound_play_sfx_24();
            g->eto[active_player].tax -= 5;
            SETMAX(g->eto[active_player].tax, 0);
        } else if (oi == oi_plus) {
            ui_sound_play_sfx_24();
            g->eto[active_player].tax += 5;
            SETMIN(g->eto[active_player].tax, 200);
        } else if (oi == oi_trans) {
            ui_sound_play_sfx_24();
            flag_trans = true;
        }
        ui_cursor_setup_area(1, &ui_cursor_area_tbl[flag_trans ? 9 : 0]);
        if (!flag_done) {
            uiobj_table_set_last(oi_alt_moola);
            oi_up = uiobj_add_t0(292, 159, "", ui_data.gfx.starmap.reprtbut_up, MOO_KEY_COMMA, -1);
            oi_down = uiobj_add_t0(292, 176, "", ui_data.gfx.starmap.reprtbut_down, MOO_KEY_PERIOD, -1);
            oi_ok = uiobj_add_t0(256, 181, "", d.gfx_but_ok, MOO_KEY_o, -1);
            for (int i = 0; i < PLANETS_ON_SCREEN; ++i) {
                int pi, y0, y1;
                pi = i + d.pos;
                if (pi < d.num) {
                    tbl_onscreen_planets[i] = d.planets[pi];
                    y0 = 21 + i * 11;
                    y1 = y0 + 8;
                    oi_tbl_planets[i] = uiobj_add_mousearea(7, y0, 248, y1, MOO_KEY_UNKNOWN, -1);
                }
            }
            oi_trans = UIOBJI_INVALID;
            if (!flag_trans) {
                if (g->eto[active_player].reserve_bc != 0) {
                    oi_trans = uiobj_add_t0(209, 181, "", d.gfx_but_trans, MOO_KEY_t, -1);
                }
                /*oi_slider =*/ uiobj_add_slider(213, 160, 0, 200, 0, 200, 32, 9, &g->eto[active_player].tax, MOO_KEY_UNKNOWN, -1);
                oi_minus = uiobj_add_mousearea(208, 157, 211, 165, MOO_KEY_UNKNOWN, -1);
                oi_plus = uiobj_add_mousearea(247, 157, 251, 165, MOO_KEY_UNKNOWN, -1);
            } else {
                oi_minus = UIOBJI_INVALID;
                oi_plus = UIOBJI_INVALID;
                /*oi_slider = UIOBJI_INVALID;*/
            }
            planets_draw_cb(&d); /* FIXME not needed in original */
            ui_draw_finish();
            ui_delay_ticks_or_click(2);
        }
    }

    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    free_pl_data(&d);
}
