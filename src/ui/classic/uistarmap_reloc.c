#include "config.h"

#include <stdio.h>

#include "uistarmap.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_save.h"
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

static void ui_starmap_reloc_draw_cb(void *vptr)
{
    struct starmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const planet_t *pf = &g->planet[d->from_i];
    const planet_t *pr = &g->planet[pf->reloc];
    const planet_t *pt = &g->planet[g->planet_focus_i[d->api]];
    char buf[0x40];
    int x0, y0;
    STARMAP_LIM_INIT();
    ui_starmap_draw_starmap(d);
    ui_starmap_draw_button_text(d, true);
    x0 = (pf->x - ui_data.starmap.x) * 2 + 8;
    y0 = (pf->y - ui_data.starmap.y) * 2 + 8;
    if (pf != pt || ui_modern_controls) {
        int x1, y1;
        const uint8_t *ctbl;
        x1 = (pt->x - ui_data.starmap.x) * 2 + 14;
        y1 = (pt->y - ui_data.starmap.y) * 2 + 14;
        ctbl = !d->valid_target_cb(d, g->planet_focus_i[d->api]) ? colortbl_line_red : colortbl_line_green;
        if (ui_modern_controls) {
            int x2, y2;
            x2 = (pr->x - ui_data.starmap.x) * 2 + 14;
            y2 = (pr->y - ui_data.starmap.y) * 2 + 14;
            if (pt != pf) {
                ui_draw_planet_frame_limit_ctbl(x1 - 6, y1 - 6, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
            }
            if (pr != pf) {
                ui_draw_line_limit_ctbl(x0 + 6, y0 + 6, x2, y2, colortbl_line_green, 5, ui_data.starmap.line_anim_phase, starmap_scale);
            }
        }
        if (pf != pt) {
            ui_draw_line_limit_ctbl(x0 + 6, y0 + 6, x1, y1, ctbl, 5, ui_data.starmap.line_anim_phase, starmap_scale);
        }
    }
    lbxgfx_draw_frame_offs(x0, y0, ui_data.gfx.starmap.planbord, STARMAP_LIMITS, UI_SCREEN_W, starmap_scale);
    lbxgfx_draw_frame(222, 80, ui_data.gfx.starmap.relocate, UI_SCREEN_W, ui_scale);
    lbxfont_select_set_12_1(5, 5, 0, 0);
    lbxfont_print_str_center(269, 90, game_str_sm_sreloc, UI_SCREEN_W, ui_scale);
    lbxfont_select(0, 6, 0, 0);
    lbxfont_print_str_split(229, 105, 80, game_str_sm_sreloc2, 2, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
    if (g->planet_focus_i[d->api] != d->from_i) {
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

static bool ui_starmap_reloc_valid_destination(const struct starmap_data_s *d, int planet_i)
{
    const struct game_s *g = d->g;
    const planet_t *p = &g->planet[planet_i];
    return (g->planet[d->from_i].buildship != BUILDSHIP_STARGATE)
        && (p->within_frange[g->active_player] == 1);
}

static void ui_starmap_reloc_do_accept(struct starmap_data_s *d)
{
    struct game_s *g = d->g;
    g->planet[d->from_i].reloc = g->planet_focus_i[d->api];
}

void ui_starmap_reloc(struct game_s *g, player_id_t active_player)
{
    bool flag_done = false;
    int16_t oi_cancel,
            oi_f4, oi_f5, oi_f6, oi_f7, oi_f10
            ;
    struct starmap_data_s d;

    ui_starmap_common_init(g, &d, active_player);
    d.valid_target_cb = ui_starmap_reloc_valid_destination;
    d.on_accept_cb = ui_starmap_reloc_do_accept;

    {
        uint8_t oldreloc;
        uint8_t pi = g->planet_focus_i[active_player];
        oldreloc = g->planet[pi].reloc;
        g->planet_focus_i[active_player] = oldreloc;
        if (g->planet[oldreloc].owner != active_player) {
            g->planet_focus_i[active_player] = pi;
        }
    }

    ui_starmap_common_late_init(&d, ui_starmap_reloc_draw_cb, true);

#define UIOBJ_CLEAR_LOCAL() \
    do { \
        STARMAP_UIOBJ_CLEAR_COMMON(); \
        STARMAP_UIOBJ_CLEAR_FX(); \
        oi_cancel = UIOBJI_INVALID; \
    } while (0)

    UIOBJ_CLEAR_LOCAL();

    uiobj_set_help_id(2);

    while (!flag_done) {
        int16_t oi1, oi2;
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if (ui_starmap_common_handle_oi(g, &d, &flag_done, oi1, oi2)) {
        } else if (oi1 == oi_f10) {
            game_save_do_save_i(GAME_SAVE_I_CONTINUE, "Continue", g);
        } else if (oi1 == oi_f4) {
            bool found;
            int i, pi;
            i = pi = g->planet_focus_i[active_player];
            found = false;
            do {
                i = (i + 1) % g->galaxy_stars;
                for (int j = 0; j < g->eto[active_player].shipdesigns_num; ++j) {
                    if (g->eto[active_player].orbit[i].ships[j]) {
                        found = true;
                        break;
                    }
                }
            } while ((!found) && (i != pi));
            if (found) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f5) {
            bool found;
            int i, pi;
            i = pi = g->planet_focus_i[active_player];
            found = false;
            do {
                if (--i < 0) { i = g->galaxy_stars - 1; }
                for (int j = 0; j < g->eto[active_player].shipdesigns_num; ++j) {
                    if (g->eto[active_player].orbit[i].ships[j]) {
                        found = true;
                        break;
                    }
                }
            } while ((!found) && (i != pi));
            if (found) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f6) {
            int i;
            i = ui_starmap_newship_next(g, active_player, g->planet_focus_i[active_player]);
            if (i != PLANET_NONE) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        } else if (oi1 == oi_f7) {
            int i;
            i = ui_starmap_newship_prev(g, active_player, g->planet_focus_i[active_player]);
            if (i != PLANET_NONE) {
                g->planet_focus_i[active_player] = i;
                ui_starmap_set_pos_focus(g, active_player);
                ui_sound_play_sfx_24();
            }
        }
        if ((oi1 == oi_cancel) || (oi1 == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
            ui_data.ui_main_loop_action = UI_MAIN_LOOP_STARMAP;
        }
        ui_starmap_set_ruler(&d, oi2);
        d.ruler_from_fleet = false;
        if (!flag_done) {
            ui_starmap_select_bottom_highlight(&d, oi2);
            ui_starmap_reloc_draw_cb(&d);
            uiobj_table_clear();
            UIOBJ_CLEAR_LOCAL();
            oi_f4 = uiobj_add_inputkey(MOO_KEY_F4);
            oi_f5 = uiobj_add_inputkey(MOO_KEY_F5);
            oi_f6 = uiobj_add_inputkey(MOO_KEY_F6);
            oi_f7 = uiobj_add_inputkey(MOO_KEY_F7);
            oi_f10 = uiobj_add_inputkey(MOO_KEY_F10);
            ui_starmap_common_fill_oi(&d);
            oi_cancel = uiobj_add_t0(227, 163, "", ui_data.gfx.starmap.reloc_bu_cancel, MOO_KEY_ESCAPE);
            if (d.valid_target_cb(&d, g->planet_focus_i[active_player])) {
                d.oi_accept = uiobj_add_t0(271, 163, "", ui_data.gfx.starmap.reloc_bu_accept, MOO_KEY_SPACE);
            }
            ui_draw_finish();
            ui_delay_ticks_or_click(STARMAP_DELAY);
        }
    }
    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    g->planet_focus_i[active_player] = d.from_i;
}
