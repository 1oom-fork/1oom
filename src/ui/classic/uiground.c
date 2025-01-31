#include "config.h"

#include <ctype.h>
#include <stdio.h>

#include "ui.h"
#include "comp.h"
#include "game.h"
#include "game_ground.h"
#include "game_spy.h"
#include "game_str.h"
#include "game_tech.h"
#include "gfxaux.h"
#include "hw.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uilanding.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "util.h"
#include "vgabuf.h"

/* -------------------------------------------------------------------------- */

struct ground_data_s {
    struct ground_s *gr;
    struct landing_data_s l;
    uint8_t *gfx_s[2];
    uint8_t *gfx_death;
    bool flag_over;
};

static void ground_prepare(struct ground_data_s *d)
{
    const struct game_s *g = d->gr->g;
    vgabuf_copy_back_from_page3();
    lbxgfx_set_new_frame(d->l.gfx_transprt, 39);
    gfx_aux_draw_frame_to(d->l.gfx_transprt, &ui_data.aux.screen);
    gfx_aux_draw_frame_from_limit(0, 100, &ui_data.aux.screen, 0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, UI_SCREEN_W);
    vgabuf_copy_back_to_page3();
    ui_delay_1();
    ui_sound_stop_music();
    for (int i = 0; i < 2; ++i) {
        d->gfx_s[i] = lbxfile_item_get(LBXFILE_LANDING, 0x1b + g->eto[d->gr->s[i].player].race, 0);
    }
    d->gfx_death = lbxfile_item_get(LBXFILE_LANDING, 0x14, 0);
}

static void ground_free_data(struct ground_data_s *d)
{
    ui_landing_free_data(&d->l);
    lbxfile_item_release(LBXFILE_LANDING, d->gfx_s[0]);
    lbxfile_item_release(LBXFILE_LANDING, d->gfx_s[1]);
    lbxfile_item_release(LBXFILE_LANDING, d->gfx_death);
}

static void ground_draw_item(int popi, int popnum, uint8_t *gfx, bool is_right, int xoff)
{
    int x, y, totalrows, x0, y0, col, row;
    totalrows = (popnum / 15) + ((popnum % 15) != 0);
    x0 = 10 - xoff;
    y0 = 120 - (totalrows * 6);
    row = popi / 15;
    col = popi % 15;
    x = x0 + (col * 8) + (row & 1) * 4;
    y = y0 + (col & 1) * 2 + (row & 1) * 6 + (row / 2) * 12;
    gfx_aux_draw_frame_to(gfx, &ui_data.aux.btemp);
    if (is_right) {
        gfx_aux_flipx(&ui_data.aux.btemp);
        x = 295 - x - xoff;
    }
    gfx_aux_draw_frame_from(x, y, &ui_data.aux.btemp, UI_SCREEN_W);
}

static void ground_draw_cb1(void *vptr)
{
    struct ground_data_s *d = vptr;
    const struct ground_s *gr = d->gr;
    const struct game_s *g = d->gr->g;
    const char *strrace[2];
    char buf[0x80];
    for (int i = 0; i < 2; ++i) {
        strrace[i] = game_str_tbl_race[g->eto[gr->s[i].player].race];
    }
    vgabuf_copy_back_from_page3();
    if (d->l.frame < 50) {
        int y;
        y = d->l.frame * 4 - 100;
        gfx_aux_draw_frame_to(d->l.gfx_transprt, &ui_data.aux.screen);
        gfx_aux_draw_frame_from_limit(0, y, &ui_data.aux.screen, 0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, UI_SCREEN_W);
        if (!gr->flag_rebel) {
            sprintf(buf, "%i %s %i %s %s", gr->inbound, game_str_gr_outof, gr->total_inbound, strrace[gr->flag_swap ? 1 : 0], game_str_gr_transs);
        } else {
            /*7b9a8*/
            sprintf(buf, "%i %s", gr->inbound, game_str_gr_reclaim);
        }
        /*7b9bf*/
        lbxfont_select_set_12_4(4, 0x5, 0, 0);
        lbxfont_print_str_center(160, 5, buf, UI_SCREEN_W);
        if (!gr->flag_rebel) {
            sprintf(buf, "%s %s %s", game_str_gr_penetr, strrace[gr->flag_swap ? 0 : 1], game_str_gr_defenss);
            lbxfont_print_str_center(160, 17, buf, UI_SCREEN_W);
        }
    } else {
        /*7ba55*/
        lbxfont_select_set_12_4(4, 0x5, 0, 0);
        sprintf(buf, "%s %s", strrace[1], game_str_gr_troops);
        lbxfont_print_str_normal(30, 146, buf, UI_SCREEN_W);
        lbxfont_print_num_normal(10, 146, gr->s[1].pop1, UI_SCREEN_W);
        ui_draw_filled_rect(164, 155, 281, 157, 0);
        ui_draw_line1(165, 156, 280, 156, 4);
        lbxfont_select_set_12_4(0, 0x1, 0, 0);
        for (int i = 0; i < gr->s[1].strnum; ++i) {
            lbxfont_print_str_normal(10, 160 + 10 * i, gr->s[1].str[i], UI_SCREEN_W);
        }
        for (int i = 0; i < gr->s[1].pop1; ++i) {
            ground_draw_item(i, gr->s[1].pop2, d->gfx_s[1], false, 0);
        }
        lbxfont_select_set_12_4(4, 0x5, 0, 0);
        sprintf(buf, "%s %s", gr->flag_rebel ? game_str_gr_rebel : strrace[0], game_str_gr_troops);
        lbxfont_print_str_normal(190, 146, buf, UI_SCREEN_W);
        lbxfont_print_num_normal(170, 146, gr->s[0].pop1, UI_SCREEN_W);
        ui_draw_filled_rect(4, 155, 121, 157, 0);
        ui_draw_line1(5, 156, 120, 156, 4);
        lbxfont_select_set_12_4(0, 0x1, 0, 0);
        for (int i = 0; i < gr->s[0].strnum; ++i) {
            lbxfont_print_str_normal(170, 160 + 10 * i, gr->s[0].str[i], UI_SCREEN_W);
        }
        for (int i = 0; i < gr->s[0].pop1; ++i) {
            ground_draw_item(i, gr->s[0].pop2, d->gfx_s[0], true, 0);
        }
        lbxfont_select_set_12_4(4, 0x5, 0, 0);
        if ((gr->s[0].pop1 != 0) && (gr->s[1].pop1 != 0)) {
            sprintf(buf, "%s %s", game_str_gr_gcon, g->planet[gr->planet_i].name);
            lbxfont_print_str_center(160, 5, buf, UI_SCREEN_W);
        } else if (d->flag_over) {
            /*7bccd*/
            int pos, pop = gr->s[gr->flag_swap ? 1 : 0].pop1;
            if (pop != 0) {
                if (!gr->flag_swap) {
                    pos = sprintf(buf, "%s%s ", strrace[0], game_str_gr_scapt);
                } else {
                    if (gr->flag_rebel) {
                        pos = sprintf(buf, "%s ", game_str_gr_itroops);
                    } else {
                        pos = sprintf(buf, "%s%s ", strrace[1], game_str_gr_scapt);
                    }
                }
            } else {
                /*7bd5d*/
                const char *s;
                if (!gr->flag_swap) {
                    s = strrace[1];
                } else {
                    if (gr->flag_rebel) {
                        s = game_str_gr_rebel;
                    } else {
                        s = strrace[0];
                    }
                }
                /*7bdb1*/
                pos = sprintf(buf, "%s%s ", s, game_str_gr_succd);
            }
            /*7bdc0*/
            sprintf(&buf[pos], "%s", g->planet[gr->planet_i].name);
            lbxfont_print_str_center(160, 5, buf, UI_SCREEN_W);
            if ((pop > 0) && (!gr->flag_rebel)) {
                /*7be16*/
                if (gr->fact > 0) {
                    sprintf(buf, "%i %s", gr->fact, game_str_gr_fcapt);
                    lbxfont_print_str_center(160, 25, buf, UI_SCREEN_W);
                }
                /*7be54*/
                if (gr->techchance > 0) {
                    if (gr->flag_swap == gr->s[0].human) {
                        lbxfont_print_str_center(160, 40, game_str_gr_tsteal, UI_SCREEN_W);
                        for (int i = 0; i < gr->techchance; ++i) {
                            game_tech_get_name(g->gaux, gr->steal->tbl_field[i], gr->steal->tbl_tech2[i], buf);
                            lbxfont_print_str_center(160, 60 + 15 * i, buf, UI_SCREEN_W);
                        }
                    } else {
                        lbxfont_print_str_center(160, 38, game_str_gr_tnew, UI_SCREEN_W);
                    }
                }
            }
        }
        /*7bf47*/
        if ((gr->s[0].pop1 != 0) && (gr->s[1].pop1 != 0) && (gr->death != -1)) {
            int side = gr->death;
            ground_draw_item(gr->s[side].pop1, gr->s[side].pop2, d->gfx_death, side == 0, 5);
        }
    }
    if (++d->l.frame == (60 + 100)) {
        d->l.frame = 61;
    }
}

/* -------------------------------------------------------------------------- */

void ui_ground(struct ground_s *gr)
{
    struct ground_data_s d;
    bool flag_done = false;
    int downcount = 4;
    memset(&d, 0, sizeof(d));
    d.gr = gr;
    d.l.g = gr->g;
    d.l.api = PLAYER_0; /* for gfx_walk banner which is unused here */
    d.l.planet = gr->planet_i;
    d.l.colonize = false;
    d.flag_over = false;
    if (ui_draw_finish_mode != 2) {
        ui_palette_fadeout_a_f_1();
    }
    lbxpal_select(6, -1, 0);
    ui_landing_prepare(&d.l);
    ui_sound_play_music(d.l.music_i);
    lbxpal_build_colortables();
    ui_draw_finish_mode = 2;
    uiobj_set_callback_and_delay(ground_draw_cb1, &d, 2);
    uiobj_table_clear();
    uiobj_add_mousearea(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, MOO_KEY_UNKNOWN, -1);
    uiobj_set_downcount(3);
    while ((!flag_done) && (d.l.frame < 50)) {
        int16_t oi;
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if (oi != 0) {
            d.l.frame = 50;
            flag_done = true;
        }
        if (!flag_done) {
            ground_draw_cb1(&d);
            ui_draw_finish();
        }
        ui_delay_ticks_or_click(2);
    }
    ground_prepare(&d);
    /*7b442*/
    uiobj_table_clear();
    uiobj_add_mousearea(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, MOO_KEY_UNKNOWN, -1);
    ui_sound_play_music(0x26);
    flag_done = false;
    while ((!flag_done) && (gr->s[0].pop1 != 0) && (gr->s[1].pop1 != 0)) {
        int16_t oi;
        if (d.l.frame < 60) {
            gr->death = -1;
        } else {
            game_ground_kill(gr);
        }
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if (oi != 0) {
            d.l.frame = 50;
            flag_done = true;
        }
        if (!flag_done) {
            if (--downcount == 0) {
                ui_sound_play_sfx(0x02);
                downcount = rnd_0_nm1(3, &gr->g->seed) + 2;
            }
            ground_draw_cb1(&d);
            ui_draw_finish();
        }
        ui_delay_ticks_or_click(2);
    }
    while ((gr->s[0].pop1 != 0) && (gr->s[1].pop1 != 0)) {
        game_ground_kill(gr);
    }
    if (gr->flag_swap) {
        int t;
        t = gr->s[0].pop2; gr->s[0].pop2 = gr->s[1].pop2; gr->s[1].pop2 = t;
        t = gr->s[0].pop1; gr->s[0].pop1 = gr->s[1].pop1; gr->s[1].pop1 = t;
        t = gr->s[0].player; gr->s[0].player = gr->s[1].player; gr->s[1].player = t;
        ui_sound_play_music((gr->s[0].pop1 != 0) ? 0xb : 0xc);
    } else {
        ui_sound_play_music((gr->s[0].pop1 != 0) ? 0xc : 0xb);
    }
    game_ground_finish(gr);
    d.flag_over = true;
    if (gr->flag_swap) {
        int t;
        t = gr->s[0].pop2; gr->s[0].pop2 = gr->s[1].pop2; gr->s[1].pop2 = t;
        t = gr->s[0].pop1; gr->s[0].pop1 = gr->s[1].pop1; gr->s[1].pop1 = t;
        t = gr->s[0].player; gr->s[0].player = gr->s[1].player; gr->s[1].player = t;
    }
    flag_done = false;
    while (!flag_done) {
        int16_t oi;
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if (oi != 0) {
            flag_done = true;
        }
        if (!flag_done) {
            ground_draw_cb1(&d);
            ui_draw_finish();
        }
        ui_delay_ticks_or_click(2);
    }
    hw_audio_music_fadeout();
    ui_palette_fadeout_a_f_1();
    lbxpal_select(0, -1, 0);
    lbxpal_build_colortables();
    ui_draw_finish_mode = 2;
    uiobj_unset_callback();
    uiobj_table_clear();
    ground_free_data(&d);
}
