#include "config.h"

#include <stdio.h>

#include "uitech.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_misc.h"
#include "game_str.h"
#include "game_tech.h"
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

#define TECH_ON_SCREEN  16
#define TECH_SCROLL_NUM 12

struct tech_data_s {
    struct game_s *g;
    player_id_t api;
    uint8_t *gfx;
    int8_t completed[TECH_PER_FIELD + 3];
    int16_t oi_tbl_slider[TECH_FIELD_NUM];
    tech_field_t field;
    int num;
    int pos;
    int selected;
    bool hmm;
};

static void tech_load_data(struct tech_data_s *d)
{
    d->gfx = lbxfile_item_get(LBXFILE_BACKGRND, 4, 0);
}

static void tech_free_data(struct tech_data_s *d)
{
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx);
}

static void ui_tech_build_completed(struct tech_data_s *d)
{
    const techdata_t *t = &(d->g->eto[d->api].tech);
    tech_field_t field = d->field;
    const uint8_t *q = d->g->srd[d->api].researchcompleted[field];
    int num = t->completed[field];
    int8_t *p = d->completed;
    if (field == TECH_FIELD_WEAPON) {
        *p++ = -2;
        *p++ = -1;
    }
    for (int i = 0; i < num; ++i) {
        *p++ = *q++;
    }
    if (field == TECH_FIELD_WEAPON) {
        num += 2;
    }
    if (t->project[field]) {
        *p++ = t->project[field];
        num++;
    }
    d->num = num;
}

static void tech_draw_cb(void *vptr)
{
    struct tech_data_s *d = vptr;
    struct game_s *g = d->g;
    empiretechorbit_t *e = &(g->eto[d->api]);
    techdata_t *t = &(e->tech);
    int16_t oi;
    char buf[0xe0];

    game_update_total_research(g);
    oi = uiobj_get_clicked_oi();
    for (int i = 0; i < TECH_FIELD_NUM; ++i) {
        if ((oi == d->oi_tbl_slider[i]) && (!t->slider_lock[i])) {
            game_adjust_slider_group(t->slider, i, t->slider[i], TECH_FIELD_NUM, t->slider_lock);
        }
    }

    ui_draw_filled_rect(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, 0x3a);
    ui_draw_filled_rect(3, 150, 275, 196, 0x5b);
    ui_draw_filled_rect(5, 4, 53, 15, (d->field == 0) ? 0x89 : 0xc0);
    ui_draw_filled_rect(55, 4, 108, 15, (d->field == 1) ? 0x89 : 0xc0);
    ui_draw_filled_rect(109, 4, 161, 16, (d->field == 2) ? 0x89 : 0xc0);
    ui_draw_filled_rect(5, 19, 54, 31, (d->field == 3) ? 0x89 : 0xc0);
    ui_draw_filled_rect(55, 19, 108, 31, (d->field == 4) ? 0x89 : 0xc0);
    ui_draw_filled_rect(109, 19, 161, 31, (d->field == 5) ? 0x89 : 0xc0);

    for (int i = 0; i < TECH_FIELD_NUM; ++i) {
        int y;
        y = 21 * i + 22;
        ui_draw_filled_rect(168, y, 218, y + 8, t->slider_lock[i] ? 0x24 : 0xbc);
    }

    ui_draw_filled_rect(7, 36, 154, 147, 0);
    lbxgfx_draw_frame(7, 36, ui_data.gfx.screens.techback, UI_SCREEN_W);
    lbxgfx_draw_frame(0, 0, d->gfx, UI_SCREEN_W);

    sprintf(buf, "%i %s", e->total_research_bc, game_str_bc);
    lbxfont_select(2, 6, 0, 0);
    lbxfont_print_str_right(309, 169, buf, UI_SCREEN_W);

    for (int i = 0; i < TECH_ON_SCREEN; ++i) {
        if ((i + d->pos) < d->num) {
            uint8_t a2;
            if (i == d->selected) {
                a2 = 0;
            } else if (((i + d->pos) == (d->num - 1)) && t->project[d->field]) {
                a2 = 5;
            } else {
                a2 = 1;
            }
            lbxfont_select(2, a2, 0, 0);
            game_tech_get_name(g->gaux, d->field, d->completed[i + d->pos], buf);
            lbxfont_print_str_normal(9, 37 + i * 7, buf, UI_SCREEN_W);
        }
    }
    game_tech_get_descr(g->gaux, d->field, d->completed[d->selected + d->pos], buf);
    lbxfont_select(5, 6, 0, 0);
    lbxfont_set_gap_h(1);
    lbxfont_print_str_split(10, 155, 260, buf, 0, UI_SCREEN_W, UI_SCREEN_H);

    lbxgfx_set_new_frame(ui_data.gfx.screens.tech_but_up, 1);
    lbxgfx_set_new_frame(ui_data.gfx.screens.tech_but_down, 1);
    lbxgfx_draw_frame(157, 35, ui_data.gfx.screens.tech_but_up, UI_SCREEN_W);
    lbxgfx_draw_frame(157, 141, ui_data.gfx.screens.tech_but_down, UI_SCREEN_W);

    for (int i = 0; i < TECH_FIELD_NUM; ++i) {
        int y;
        y = 21 * i + 24;
        ui_draw_filled_rect(227, y, 277, y + 3, 0x2f);
        if (t->slider[i]) {
            ui_draw_slider(227, y + 1, 226 + t->slider[i] / 2, t->slider_lock[i] ? 0x22 : 0x73);
        }
        lbxfont_select(0, 6, 0, 0);
        lbxfont_set_color_c_n(0x26, 5);
        lbxfont_print_num_right(307, y, t->percent[i], UI_SCREEN_W);
    }

    lbxfont_select(2, 6, 0, 0);
    sprintf(buf, "%s %i, %s %i/%s", game_str_te_scrange, e->scanner_range, game_str_te_rctrl, e->colonist_oper_factories, game_str_te_col);
    lbxfont_print_str_normal(168, 33, buf, UI_SCREEN_W);
    {
        uint8_t groundcmbonus = 0, groundcmbonus2 = 0;
        for (int i = 0; i < t->completed[TECH_FIELD_CONSTRUCTION]; ++i) {
            uint8_t c, tier;
            const uint8_t *p;
            c = g->srd[d->api].researchcompleted[TECH_FIELD_CONSTRUCTION][i];
            p = RESEARCH_D0_PTR(g->gaux, TECH_FIELD_CONSTRUCTION, c);
            tier = game_tech_get_tier(g->gaux, TECH_FIELD_CONSTRUCTION, c);
            if (p[0] == 7) {
                groundcmbonus = tier * 5;
            } else if (p[0] == 15) {
                groundcmbonus2 = tier * 10;
            }
        }
        groundcmbonus += groundcmbonus2;
        sprintf(buf, "%s %i, %s +%i%%", game_str_te_fwaste, e->ind_waste_scale, game_str_te_gcombat, groundcmbonus);
        lbxfont_print_str_normal(168, 54, buf, UI_SCREEN_W);
    }
    {
        uint8_t groundcmbonus = 0;
        for (int i = 0; i < t->completed[TECH_FIELD_FORCE_FIELD]; ++i) {
            uint8_t c, tier;
            const uint8_t *p;
            c = g->srd[d->api].researchcompleted[TECH_FIELD_FORCE_FIELD][i];
            p = RESEARCH_D0_PTR(g->gaux, TECH_FIELD_FORCE_FIELD, c);
            tier = game_tech_get_tier(g->gaux, TECH_FIELD_FORCE_FIELD, c);
            if (p[0] == 16) {
                groundcmbonus = tier * 10;
            }
        }
        sprintf(buf, "%s +%i%%", game_str_te_gcombat, groundcmbonus);
        lbxfont_print_str_normal(168, 75, buf, UI_SCREEN_W);
    }
    sprintf(buf, "%s +%i, %s %i/%s", game_str_te_tform, e->have_terraform_n, game_str_te_wasteel, e->have_eco_restoration_n, game_str_bc);
    lbxfont_print_str_normal(168, 96, buf, UI_SCREEN_W);
    sprintf(buf, "%s %i %s", game_str_te_shrange, e->fuel_range, game_str_sm_parsecs2);
    lbxfont_print_str_normal(168, 117, buf, UI_SCREEN_W);
    {
        uint8_t groundcmbonus = 0;
        for (int i = 0; i < t->completed[TECH_FIELD_WEAPON]; ++i) {
            uint8_t c, tier;
            const uint8_t *p;
            c = g->srd[d->api].researchcompleted[TECH_FIELD_WEAPON][i];
            p = RESEARCH_D0_PTR(g->gaux, TECH_FIELD_WEAPON, c);
            tier = game_tech_get_tier(g->gaux, TECH_FIELD_WEAPON, c);
            if (p[0] == 21) {
                groundcmbonus = tier * 5;
            }
        }
        sprintf(buf, "%s +%i%%", game_str_te_gcombat, groundcmbonus);
        lbxfont_print_str_normal(168, 138, buf, UI_SCREEN_W);
    }

    for (int i = 0; i < TECH_FIELD_NUM; ++i) {
        int y, complpercent;
        y = 21 * i + 21;
        complpercent = game_tech_current_research_percent2(e, i);
        if (complpercent < 99) {
            if (complpercent > 0) {
                sprintf(buf, "%i%%", complpercent);
                lbxfont_select_set_12_1(2, 0xd, 0, 0);
                lbxfont_print_str_right(295, y + 3, buf, UI_SCREEN_W);
            } else {
                int y0, y1;
                complpercent = game_tech_current_research_percent1(e, i);
                lbxgfx_draw_frame(287, y, ui_data.gfx.screens.litebulb_off, UI_SCREEN_W);
                y0 = y + (8 - (complpercent * 4) / 50);
                y1 = y + 7;
                lbxgfx_draw_frame_offs(287, y, ui_data.gfx.screens.litebulb_on, 0, y0, UI_SCREEN_W - 1, y1, UI_SCREEN_W);
            }
        } else {
            lbxfont_select_set_12_1(2, 0xd, 0, 0);
            lbxfont_print_str_right(295, y + 3, game_str_te_max, UI_SCREEN_W);
        }
    }
}

/* -------------------------------------------------------------------------- */

void ui_tech(struct game_s *g, player_id_t active_player)
{
    struct tech_data_s d;
    bool flag_done = false;
    int16_t oi_ok, oi_up, oi_down, oi_equals,
            oi_tbl_lock[TECH_FIELD_NUM],
            oi_tbl_plus[TECH_FIELD_NUM],
            oi_tbl_minus[TECH_FIELD_NUM],
            oi_tbl_field[TECH_FIELD_NUM],
            oi_tbl_hmm15[TECH_FIELD_NUM],
            oi_tbl_techname[TECH_ON_SCREEN]
            ;
    techdata_t *t = &(g->eto[active_player].tech);

    d.g = g;
    d.api = active_player;
    tech_load_data(&d);

    game_update_production(g);

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        oi_ok = UIOBJI_INVALID; \
        oi_up = UIOBJI_INVALID; \
        oi_down = UIOBJI_INVALID; \
        oi_equals = UIOBJI_INVALID; \
        UIOBJI_SET_TBL6_INVALID(oi_tbl_lock, oi_tbl_plus, oi_tbl_minus, oi_tbl_field, oi_tbl_hmm15, d.oi_tbl_slider); \
        UIOBJI_SET_TBL_INVALID(oi_tbl_techname); \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    d.field = TECH_FIELD_COMPUTER;
    ui_tech_build_completed(&d);
    d.pos = d.num - TECH_ON_SCREEN;
    SETMAX(d.pos, 0);
    d.selected = d.num - 1 - d.pos;

    uiobj_set_help_id(13);
    uiobj_set_callback_and_delay(tech_draw_cb, &d, 2);

    uiobj_table_clear();

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == oi_ok) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_24();
            flag_done = true;
        } else if (oi == oi_up) {
            ui_sound_play_sfx_24();
            d.pos -= TECH_SCROLL_NUM;
        } else if (oi == oi_down) {
            ui_sound_play_sfx_24();
            d.pos += TECH_SCROLL_NUM;
        }
        for (int i = 0; i < TECH_FIELD_NUM; ++i) {
            if (oi == oi_tbl_field[i]) {
                ui_sound_play_sfx_24();
                d.field = i;
                ui_tech_build_completed(&d);
                d.pos = d.num - TECH_ON_SCREEN;
                SETMAX(d.pos, 0);
                d.selected = d.num - 1 - d.pos;
            } else if (oi == oi_tbl_lock[i]) {
                ui_sound_play_sfx_24();
                t->slider_lock[i] ^= 1;
            } else if (oi == oi_tbl_minus[i]) {
                int16_t v;
                ui_sound_play_sfx_24();
                v = t->slider[i];
                v -= 2;
                SETMAX(v, 0);
                t->slider[i] = v;
                game_adjust_slider_group(t->slider, i, v, TECH_FIELD_NUM, t->slider_lock);
            } else if (oi == oi_tbl_plus[i]) {
                int16_t v;
                ui_sound_play_sfx_24();
                v = t->slider[i];
                v += 2;
                SETMIN(v, 100);
                t->slider[i] = v;
                game_adjust_slider_group(t->slider, i, v, TECH_FIELD_NUM, t->slider_lock);
            } else if ((oi == d.oi_tbl_slider[i]) && (!t->slider_lock[i])) {
                game_adjust_slider_group(t->slider, i, t->slider[i], TECH_FIELD_NUM, t->slider_lock);
            } else if (oi == oi_tbl_hmm15[i]) {
                d.hmm = !d.hmm;
            }
        }
        for (int i = 0; i < TECH_ON_SCREEN; ++i) {
            if (oi == oi_tbl_techname[i]) {
                ui_sound_play_sfx_24();
                d.selected = i;
            }
        }
        if (oi == oi_equals) {
            ui_sound_play_sfx_24();
            t->slider[0] = 16;
            t->slider[1] = 17;
            t->slider[2] = 17;
            t->slider[3] = 16;
            t->slider[4] = 17;
            t->slider[5] = 17;
        }
        if ((d.pos + TECH_ON_SCREEN) >= d.num) {
            d.pos = d.num - TECH_ON_SCREEN;
        }
        SETMAX(d.pos, 0);
        if ((d.selected + d.pos) >= d.num) {
            d.selected = d.num - d.pos;
        }
        SETMAX(d.pos, 0);
        if (!flag_done) {
            tech_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            if (d.num >= TECH_ON_SCREEN) {
                if (d.pos > 0) {
                    oi_up = uiobj_add_t0(157, 35, "", ui_data.gfx.screens.tech_but_up, MOO_KEY_COMMA, -1);
                }
                if ((d.pos + TECH_ON_SCREEN) < d.num) {
                    oi_down = uiobj_add_t0(157, 141, "", ui_data.gfx.screens.tech_but_down, MOO_KEY_PERIOD, -1);
                }
            }
            oi_ok = uiobj_add_t0(277, 181, "", ui_data.gfx.screens.tech_but_ok, MOO_KEY_SPACE, -1);
            oi_tbl_field[0] = uiobj_add_mousearea(5, 4, 53, 15, MOO_KEY_UNKNOWN, -1);
            oi_tbl_field[1] = uiobj_add_mousearea(55, 4, 108, 15, MOO_KEY_UNKNOWN, -1);
            oi_tbl_field[2] = uiobj_add_mousearea(109, 4, 161, 16, MOO_KEY_UNKNOWN, -1);
            oi_tbl_field[3] = uiobj_add_mousearea(5, 19, 54, 31, MOO_KEY_UNKNOWN, -1);
            oi_tbl_field[4] = uiobj_add_mousearea(55, 19, 108, 31, MOO_KEY_UNKNOWN, -1);
            oi_tbl_field[5] = uiobj_add_mousearea(109, 19, 161, 31, MOO_KEY_UNKNOWN, -1);
            oi_equals = uiobj_add_inputkey(MOO_KEY_EQUALS);
            for (int i = 0; i < TECH_FIELD_NUM; ++i) {
                int y;
                y = i * 21 + 22;
                oi_tbl_hmm15[i] = uiobj_add_mousearea(287, y - 1, 295, y + 7, MOO_KEY_UNKNOWN, -1);
                oi_tbl_lock[i] = uiobj_add_mousearea(168, y, 218, y + 8, MOO_KEY_UNKNOWN, -1);
                if (!t->slider_lock[i]) {
                    oi_tbl_minus[i] = uiobj_add_mousearea(223, y, 226, y + 8, MOO_KEY_UNKNOWN, -1);
                    oi_tbl_plus[i] = uiobj_add_mousearea(279, y, 283, y + 8, MOO_KEY_UNKNOWN, -1);
                    d.oi_tbl_slider[i] = uiobj_add_slider(227, y, 0, 100, 0, 100, 50, 9, &t->slider[i], MOO_KEY_UNKNOWN, -1);
                }
            }
            for (int i = 0; i < TECH_ON_SCREEN; ++i) {
                if ((i + d.pos) < d.num) {
                    oi_tbl_techname[i] = uiobj_add_mousearea(9, i * 7 + 37, 160, i * 7 + 43, MOO_KEY_UNKNOWN, -1);
                }
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(2);
        }
    }

    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    tech_free_data(&d);
}
