#include "config.h"

#include <stdio.h>

#include "uigovern.h"
#include "comp.h"
#include "game.h"
#include "game_planet.h"
#include "game_str.h"
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
#include "uisound.h"

/* -------------------------------------------------------------------------- */

struct govern_data_s {
    int spend_rest;
    governor_eco_mode_t eco_mode;
    uint16_t target;
    enum {
        UI_GV_HIGHLIGHT_NONE,
        UI_GV_HIGHLIGHT_ECO_MODE,
        UI_GV_HIGHLIGHT_STARGATE,
        UI_GV_HIGHLIGHT_ADJUST,
        UI_GV_HIGHLIGHT_SPENDTHIS,
        UI_GV_HIGHLIGHT_SPENDALL
    } highlight;
    bool allow_stargates;
    bool my_planet;
};

static void govern_draw_cb(void *vptr)
{
    struct govern_data_s *d = vptr;
    if (d->my_planet) {
        const int x = 56, y = 10;
        ui_draw_filled_rect(x, y, x + 160, y + 30, 0x06, ui_scale);
        lbxfont_select(0, 0xd, 0, 0);
        lbxfont_print_str_split(x + 5, y + 5, 145, game_str_gv_target, 0, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        lbxfont_select(2, 6, 0, 0);
        lbxfont_print_num_right(x + 73, y + 18, d->target, UI_SCREEN_W, ui_scale);
    }
    if (d->my_planet) {
        const int x = 56;
        int y = 45;
        ui_draw_filled_rect(x, y, x + 160, y + 25, 0x06, ui_scale);
        lbxfont_select(0, 0xd, 0, 0);
        y += 5;
        lbxfont_print_str_normal(x + 5, y, game_str_gv_thispl, UI_SCREEN_W, ui_scale);
        lbxfont_select(0, (d->highlight == UI_GV_HIGHLIGHT_SPENDTHIS) ? 0x1 : 0x0, 0, 0);
        sprintf(ui_data.strbuf, "%s %s", game_str_gv_rest, game_str_tbl_gv_rest[d->spend_rest]);
        y += 10;
        lbxfont_print_str_normal(x + 5, y, ui_data.strbuf, UI_SCREEN_W, ui_scale);
    }
    {
        const int x = 56;
        int y = 110;
        ui_draw_filled_rect(x, y, x + 160, y + 33, 0x06, ui_scale);
        lbxfont_select(0, 0xd, 0, 0);
        y += 5;
        lbxfont_print_str_normal(x + 5, y, game_str_gv_allpl, UI_SCREEN_W, ui_scale);
        y += 10;
        ui_draw_filled_rect(x + 5, y + 1, x + 8, y + 4, 0x01, ui_scale);
        if (d->allow_stargates) {
            ui_draw_filled_rect(x + 6, y + 2, x + 7, y + 3, 0x44, ui_scale);
        }
        lbxfont_select(0, (d->highlight == UI_GV_HIGHLIGHT_STARGATE) ? 0x1 : 0x0, 0, 0);
        lbxfont_print_str_normal(x + 11, y, game_str_gv_starg, UI_SCREEN_W, ui_scale);
        y += 8;
        sprintf(ui_data.strbuf, "%s: %s", game_str_gv_ecom, game_str_tbl_gv_ecom[d->eco_mode]);
        lbxfont_select(0, (d->highlight == UI_GV_HIGHLIGHT_ECO_MODE) ? 0x1 : 0x0, 0, 0);
        lbxfont_print_str_normal(x + 5, y, ui_data.strbuf, UI_SCREEN_W, ui_scale);
    }
    {
        const int x = 56, y = 150;
        ui_draw_filled_rect(x, y, x + 160, y + 22, 0x06, ui_scale);
        lbxfont_select(0, (d->highlight == UI_GV_HIGHLIGHT_ADJUST) ? 0x1 : 0x0, 0, 0);
        lbxfont_print_str_normal(x + 5, y + 5, game_str_gv_adjust, UI_SCREEN_W, ui_scale);
        sprintf(ui_data.strbuf, "%s %s", game_str_gv_resta, game_str_tbl_gv_rest[d->spend_rest]);
        lbxfont_select(0, (d->highlight == UI_GV_HIGHLIGHT_SPENDALL) ? 0x1 : 0x0, 0, 0);
        lbxfont_print_str_normal(x + 5, y + 13, ui_data.strbuf, UI_SCREEN_W, ui_scale);
    }
}

/* -------------------------------------------------------------------------- */

void ui_govern(struct game_s *g, player_id_t pi)
{
    struct govern_data_s d;
    bool flag_done = false;
    int16_t oi_cancel, oi_accept, oi_p, oi_m, oi_p10, oi_m10, oi_adjust, oi_wheel, oi_spendthis, oi_sg, oi_ecom, oi_spendall;
    const int x = 56;
    int y = 10;
    int16_t scroll = 0;
    planet_t *p = &(g->planet[g->planet_focus_i[pi]]);

    ui_draw_copy_buf();
    uiobj_finish_frame();
    d.my_planet = (p->owner == pi);
    if (d.my_planet) {
        d.target = p->target_bases;
        d.spend_rest = (p->extras[0] >> 1) & 3;
    } else {
        d.target = 0;
        d.spend_rest = 0;
    }
    d.eco_mode = g->evn.gov_eco_mode[pi];
    d.allow_stargates = BOOLVEC_IS0(g->evn.gov_no_stargates, pi);
    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);

    uiobj_table_clear();
    if (d.my_planet) {
        oi_p = uiobj_add_t0(x + 5 + 23, y + 15, "", ui_data.gfx.starmap.move_but_p, MOO_KEY_UNKNOWN);
        oi_m = uiobj_add_t0(x + 5 + 12, y + 15, "", ui_data.gfx.starmap.move_but_m, MOO_KEY_UNKNOWN);
        oi_p10 = uiobj_add_t0(x + 5 + 34, y + 15, "", ui_data.gfx.starmap.move_but_a, MOO_KEY_UNKNOWN);
        oi_m10 = uiobj_add_t0(x + 5, y + 15, "", ui_data.gfx.starmap.move_but_n, MOO_KEY_UNKNOWN);
        oi_cancel = uiobj_add_t0(x + 78, y + 14, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
        oi_accept = uiobj_add_t0(x + 116, y + 14, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
    } else {
        oi_p = UIOBJI_INVALID;
        oi_m = UIOBJI_INVALID;
        oi_p10 = UIOBJI_INVALID;
        oi_m10 = UIOBJI_INVALID;
        oi_cancel = UIOBJI_INVALID;
        oi_accept = UIOBJI_INVALID;
    }
    oi_wheel = uiobj_add_mousewheel(x, y, x + 80, y + 30, &scroll);
    y = 45 + 15;
    if (d.my_planet) {
        oi_spendthis = uiobj_add_mousearea(x, y, x + 160, y + 7, MOO_KEY_r);
    } else {
        oi_spendthis = UIOBJI_INVALID;
    }
    y = 110 + 15;
    oi_sg = uiobj_add_mousearea(x, y, x + 160, y + 7, MOO_KEY_s);
    y += 8;
    oi_ecom = uiobj_add_mousearea(x, y, x + 160, y + 7, MOO_KEY_e);
    y = 150 + 5;
    oi_adjust = uiobj_add_mousearea(x, y, x + 160, y + 7, MOO_KEY_o);
    y += 8;
    oi_spendall = uiobj_add_mousearea(x, y, x + 160, y + 7, MOO_KEY_v);

    uiobj_set_callback_and_delay(govern_draw_cb, &d, 1);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        {
            int16_t oi2;
            oi2 = uiobj_at_cursor();
            d.highlight = UI_GV_HIGHLIGHT_NONE;
            if (oi2 != UIOBJI_INVALID) {
                if (oi2 == oi_ecom) {
                    d.highlight = UI_GV_HIGHLIGHT_ECO_MODE;
                } else if (oi2 == oi_sg) {
                    d.highlight = UI_GV_HIGHLIGHT_STARGATE;
                } else if (oi2 == oi_adjust) {
                    d.highlight = UI_GV_HIGHLIGHT_ADJUST;
                } else if (oi2 == oi_spendthis)  {
                    d.highlight = UI_GV_HIGHLIGHT_SPENDTHIS;
                } else if (oi2 == oi_spendall)  {
                    d.highlight = UI_GV_HIGHLIGHT_SPENDALL;
                }
            }
        }
        ui_delay_prepare();
        if ((oi == oi_cancel) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
        } else if (oi == oi_accept) {
            ui_sound_play_sfx_24();
            p->target_bases = d.target;
            flag_done = true;
        } else if (oi == oi_adjust) {
            ui_sound_play_sfx_24();
            game_planet_govern_all_owned_by(g, pi);
            flag_done = true;
        } else if (oi == oi_wheel) {
            if (ui_mwi_counter) {
                scroll = -scroll;
            }
            if (scroll < 0) {
                SUBSAT0(d.target, -scroll);
            } else {
                ADDSATT(d.target, scroll, 0xffff);
            }
            scroll = 0;
        } else if (oi == oi_m) {
            SUBSAT0(d.target, 1);
        } else if (oi == oi_m10) {
            SUBSAT0(d.target, 10);
        } else if (oi == oi_p) {
            ADDSATT(d.target, 1, 0xffff);
        } else if (oi == oi_p10) {
            ADDSATT(d.target, 10, 0xffff);
        } else if (oi == oi_spendthis) {
            d.spend_rest = (d.spend_rest + 1) % 3;
            BOOLVEC_SET(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP, ((d.spend_rest & 1) != 0));
            BOOLVEC_SET(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND, ((d.spend_rest & 2) != 0));
        } else if (oi == oi_spendall) {
            ui_sound_play_sfx_24();
            for (uint8_t i = 0; i < g->galaxy_stars; ++i) {
                planet_t *p2 = &(g->planet[i]);
                if (p2->owner == pi) {
                    BOOLVEC_SET(p2->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP, ((d.spend_rest & 1) != 0));
                    BOOLVEC_SET(p2->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND, ((d.spend_rest & 2) != 0));
                }
            }
            flag_done = true;
        } else if (oi == oi_sg) {
            BOOLVEC_SET(g->evn.gov_no_stargates, pi, d.allow_stargates);
            d.allow_stargates = !d.allow_stargates;
        } else if (oi == oi_ecom) {
            d.eco_mode = (d.eco_mode + 1) % GOVERNOR_ECO_MODE_NUM;
            g->evn.gov_eco_mode[pi] = d.eco_mode;
        }
        if (!flag_done) {
            govern_draw_cb(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(1);
        }
    }

    uiobj_unset_callback();
    uiobj_table_clear();
}
