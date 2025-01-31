#include "config.h"

#include <ctype.h>
#include <stdio.h>

#include "ui.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_diplo.h"
#include "game_planet.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_str.h"
#include "game_tech.h"
#include "game_techtypes.h"
#include "hw.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lib.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "vgabuf.h"

/* -------------------------------------------------------------------------- */

struct newtech_data_s {
    struct game_s *g;
    player_id_t api;
    uint8_t *gfx_lab;
    uint8_t *gfx_tech;
    uint8_t *gfx_spies;
    uint8_t *gfx_framing;
    uint8_t *gfx_pulldown_u;
    uint8_t *gfx_pulldown_d;
    uint8_t *gfx_eco_chng2;
    uint8_t *gfx_eco_chng4;
    uint8_t *gfx_robo_but;
    newtech_t nt;
    uint8_t music_i;
    int cur_source;
    uint8_t anim;
    uint8_t dialog_type;
    bool flag_fadeout;
    bool flag_music;
    bool flag_is_current;
    bool flag_choose_next;
    player_id_t other1;
    player_id_t other2;
    uint8_t tech_next[TECH_NEXT_MAX];
    int num_next;
    int16_t selected;
    int16_t tbl_tech[TECH_NEXT_MAX + 1];
};

static const uint8_t newtech_music_tbl[6] = { 5, 6, 7, 5, 5, 5 };

static void newtech_load_data(struct newtech_data_s *d)
{
    d->gfx_framing = lbxfile_item_get(LBXFILE_BACKGRND, 9, 0);
    d->gfx_pulldown_u = lbxfile_item_get(LBXFILE_BACKGRND, 0x1a, 0);
    d->gfx_pulldown_d = lbxfile_item_get(LBXFILE_BACKGRND, 0x1b, 0);
    d->gfx_eco_chng2 = lbxfile_item_get(LBXFILE_BACKGRND, 0x1f, 0);
    d->gfx_eco_chng4 = lbxfile_item_get(LBXFILE_BACKGRND, 0x30, 0);
    d->gfx_robo_but = lbxfile_item_get(LBXFILE_BACKGRND, 0x31, 0);
    d->gfx_spies = 0;
}

static void newtech_free_data(struct newtech_data_s *d)
{
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_framing);
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_pulldown_u);
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_pulldown_d);
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_eco_chng2);
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_eco_chng4);
    lbxfile_item_release(LBXFILE_BACKGRND, d->gfx_robo_but);
    if (d->gfx_spies) {
        lbxfile_item_release(LBXFILE_SPIES, d->gfx_spies);
    }
}

static void newtech_draw_cb1(void *vptr)
{
    struct newtech_data_s *d = vptr;
    struct game_s *g = d->g;
    empiretechorbit_t *e = &(g->eto[d->api]);
    char buf[RESEARCH_DESCR_LEN];
    vgabuf_copy_back_from_page2();
    {
        int frame = lbxgfx_get_frame(d->gfx_spies);
        lbxgfx_set_frame_0(d->gfx_spies);
        for (int f = 0; f <= frame; ++f) {
            lbxgfx_draw_frame(0, 0, d->gfx_spies, UI_SCREEN_W);
        }
        if ((d->anim = (d->anim + 1) % 3) != 0) {
            lbxgfx_set_new_frame(d->gfx_spies, frame);
        }
    }
    switch (d->nt.source) {
        case TECHSOURCE_RESEARCH:
            sprintf(buf, "%s %s %s %s", game_str_tbl_race[e->race], game_str_nt_achieve, game_str_tbl_te_field[d->nt.field], game_str_nt_break);
            break;
        case TECHSOURCE_SPY:
            sprintf(buf, "%s %s %s", game_str_tbl_race[e->race], game_str_nt_infil, g->planet[d->nt.v06].name);
            break;
        case TECHSOURCE_FOUND:
            if (d->nt.v06 == NEWTECH_V06_ORION) {
                strcpy(buf, game_str_nt_orion);
            } else if (d->nt.v06 >= 0) {    /* WASBUG > 0 vs. scout case with planet 0 */
                sprintf(buf, "%s %s %s", game_str_nt_ruins, g->planet[d->nt.v06].name, game_str_nt_discover);
            } else {
                sprintf(buf, "%s %s %s", game_str_nt_scouts, g->planet[-(d->nt.v06 + 1)].name, game_str_nt_discover);
            }
            break;
        case TECHSOURCE_CHOOSE:
            strcpy(buf, game_str_nt_choose);
            break;
        case TECHSOURCE_TRADE:
            sprintf(buf, "%s %s %s %s", game_str_tbl_race[g->eto[d->nt.v06].race], game_str_nt_reveal, game_str_tbl_te_field[d->nt.field], game_str_nt_secrets);
            break;
        default:
            break;
    }
    lbxfont_select_set_12_4(5, 5, 0, 0);
    lbxfont_print_str_center(161, 7, buf, UI_SCREEN_W);
    if (d->nt.source != TECHSOURCE_CHOOSE) {
        int strh, y;
        char *p, c;
        game_tech_get_name(d->g->gaux, d->nt.field, d->nt.tech, buf);
        p = buf;
        while ((c = *p) != 0) {
            if (islower(c)) {
                *p = toupper(c);
            }
            ++p;
        }
        lbxfont_select_set_12_4(5, 8, 0, 0);
        lbxfont_print_str_center(160, 32, buf, UI_SCREEN_W);
        game_tech_get_descr(d->g->gaux, d->nt.field, d->nt.tech, buf);
        lbxfont_select_set_12_5(4, 0xf, 0, 0);
        strh = lbxfont_calc_split_str_h(305, buf);
        /* BUG?
           Some lowercase letters extend past the screen, for example 'p' in the Hyper-X msg.
           On DOS/v1.3 this only overwrites unused VRAM. y <= 148 would be OK.
        */
        y = (strh >= 36) ? 150 : 160;
        lbxfont_print_str_split(9, y, 305, buf, 3, UI_SCREEN_W, UI_SCREEN_H);
    }
    if (d->nt.frame) {
        /*ui_newtech_draw_frame:*/
        ui_draw_filled_rect(31, 62, 202, 125, 0xfb);
        ui_draw_filled_rect(37, 68, 196, 91, 0x04);
        lbxgfx_draw_frame(31, 62, d->gfx_framing, UI_SCREEN_W);
        ui_draw_filled_rect(50, 106, 110, 120, 0x00);
        ui_draw_filled_rect(51, 107, 109, 119, tbl_banner_color2[g->eto[d->other1].banner]);
        ui_draw_filled_rect(122, 106, 183, 120, 0x00);
        ui_draw_filled_rect(123, 107, 182, 119, tbl_banner_color2[g->eto[d->other2].banner]);
        lbxfont_select(5, 6, 0, 0);
        lbxfont_print_str_center(80, 110, game_str_tbl_races[g->eto[d->other1].race], UI_SCREEN_W);
        lbxfont_print_str_center(152, 110, game_str_tbl_races[g->eto[d->other2].race], UI_SCREEN_W);
        lbxfont_select(5, 0, 0, 0);
        lbxfont_set_gap_h(2);
        lbxfont_print_str_split(40, 70, 154, game_str_nt_frame, 3, UI_SCREEN_W, UI_SCREEN_H);
        lbxfont_select(0, 0, 0, 0);
        lbxfont_print_str_center(115, 96, game_str_nt_victim, UI_SCREEN_W);
    }
}

static void newtech_choose_next_draw_cb(void *vptr)
{
    struct newtech_data_s *d = vptr;
    char buf[RESEARCH_DESCR_LEN + 20];
    int x = 145, y = 30, yo, pos;
    uint8_t tech = d->tbl_tech[d->selected];
    d->nt.source = TECHSOURCE_CHOOSE;
    newtech_draw_cb1(d);
    yo = ((d->num_next > 10) ? 8 : 9) * d->num_next + 8;
    SETMAX(yo, 30);
    ui_draw_filled_rect(x, y, x + 165, y + yo + 12, 0xf9);
    /*limits(0, y, 319, y + yo - 1)*/
    lbxgfx_draw_frame_offs(x, y, d->gfx_pulldown_u, 0, y, UI_SCREEN_W - 1, y + yo - 1, UI_SCREEN_W);
    lbxgfx_draw_frame(x, y + yo, d->gfx_pulldown_d, UI_SCREEN_W);
    /* WARNING
       MOO1 does not limit the bottom part which will go below screen with enough techs to choose from.
       On DOS/v1.3 this only overwrite unused VRAM. We must use the _offs version or solid VRAM.
    */
    sprintf(buf, "%s %s", game_str_tbl_te_field[d->nt.field], game_str_te_techno);
    lbxfont_select(5, 0xe, 0, 0);
    lbxfont_print_str_center(x + 85, y + 5, buf, UI_SCREEN_W);
    lbxfont_select_set_12_1(0, 0, 0xe, 0);
    game_tech_get_descr(d->g->gaux, d->nt.field, tech, buf);
    pos = strlen(buf);
    sprintf(&buf[pos], " \x02(%u %s)\x1", game_tech_get_next_rp(d->g, d->api, d->nt.field, tech), game_str_te_rp);
    lbxfont_print_str_split(151, y + yo + 18, 156, buf, 0, UI_SCREEN_W, UI_SCREEN_H);
}

static void ui_newtech_choose_next(struct newtech_data_s *d)
{
    char tname[TECH_NEXT_MAX][35];
    const char *nptr[TECH_NEXT_MAX + 1];
    bool cond[TECH_NEXT_MAX + 1];
    int di = (d->num_next > 10) ? 8 : 9;
    for (int i = 0; i < (TECH_NEXT_MAX + 1); ++i) {
        cond[i] = true;
    }
    for (int i = 0; i < d->num_next; ++i) {
        uint8_t tech;
        tech = d->tech_next[i];
        d->tbl_tech[i] = tech; /* WASBUG overwrites flag_copybuf, flag_dialog and d->tech_next */
        game_tech_get_name(d->g->gaux, d->nt.field, tech, tname[i]);
        nptr[i] = tname[i];
    }
    nptr[d->num_next] = 0;
    uiobj_set_callback_and_delay(newtech_choose_next_draw_cb, d, 1);
    uiobj_table_clear();
    d->selected = 0;
    newtech_choose_next_draw_cb(d);
    lbxfont_select(0, 0, 0, 0);
    lbxfont_set_gap_h(di - 6);
    ui_draw_filled_rect(155, 49, 304, 56, 0x60);
    for (int i = 0; i < d->num_next; ++i) {
        lbxfont_print_str_normal(156, i * di + di + 41, tname[i], UI_SCREEN_W);
    }
    lbxfont_select_set_12_1(0, 0, 0, 0);
    ui_draw_finish();
    lbxfont_select(0, 0, 0, 0);
    lbxfont_set_gap_h(di - 6);
    /*sel = */uiobj_select_from_list1(156, 41, 148, "", nptr, &d->selected, cond, 1, 0, 0x60, 0, 0, 0, -1);
    ui_sound_play_sfx_24();
    ui_delay_prepare();
    game_tech_start_next(d->g, d->api, d->nt.field, d->tbl_tech[d->selected]);
    ui_delay_ticks_or_click(2);
    uiobj_unset_callback();
}

static void newtech_adjust_draw_typestr(char *buf, const char *str1, const char *str2, int x, int y, int we)
{
    strcat(buf, game_str_nt_inc);
    strcat(buf, str1);
    if (str2) {
        strcat(buf, str2);
    }
    lbxfont_print_str_split(x + 15, y + 16, 110 + we, buf, 3, UI_SCREEN_W, UI_SCREEN_H);
}

static void newtech_adjust_draw_cb(void *vptr)
{
    struct newtech_data_s *d = vptr;
    char buf[0x96];
    int x = 150, y = 30;
    newtech_draw_cb1(d);
    ui_draw_filled_rect(x, y, x + 135, y + 80, 0xf9);
    lbxgfx_draw_frame(x, y, (d->dialog_type == 0) ? d->gfx_eco_chng2 : d->gfx_eco_chng4, UI_SCREEN_W);
    lbxfont_select_set_12_1(0, 0, 0, 0);
    strcpy(buf, game_str_nt_doyou);
    switch (d->dialog_type) {
        case 0:
            strcat(buf, game_str_nt_redueco);
            lbxfont_print_str_split(x + 15, y + 11, 110, buf, 3, UI_SCREEN_W, UI_SCREEN_H);
            lbxgfx_set_frame_0(ui_data.gfx.starmap.scrapbut_yes);
            lbxgfx_set_frame_0(ui_data.gfx.starmap.scrapbut_no);
            lbxgfx_draw_frame(x + 83, y + 60, ui_data.gfx.starmap.scrapbut_yes, UI_SCREEN_W);
            lbxgfx_draw_frame(x + 18, y + 60, ui_data.gfx.starmap.scrapbut_no, UI_SCREEN_W);
            break;
        case 1:
            newtech_adjust_draw_typestr(buf, game_str_nt_ind, 0, x, y, 0);
            break;
        case 2:
            newtech_adjust_draw_typestr(buf, game_str_nt_ecoall, game_str_nt_terra, x, y, 0);
            break;
        case 3:
            newtech_adjust_draw_typestr(buf, game_str_nt_def, 0, x, y - 5, 0);
            break;
        case 4:
            newtech_adjust_draw_typestr(buf, game_str_nt_ecostd, game_str_nt_terra, x - 1, y - 5, 2);
            break;
        case 5:
            newtech_adjust_draw_typestr(buf, game_str_nt_ecohost, game_str_nt_terra, x - 1, y - 5, 2);
            break;
        default:
            break;
    }
#if 0   /* FIXME does not seem to be needed, the buttons draw themselves */
    if (d->dialog_type != 0) {
        x += 10;
        for (int i = 0; i < 4; ++i, x += 32) {
            lbxgfx_set_frame_0(d->gfx_robo_but);
            lbxgfx_draw_frame(x, y + 60, d->gfx_robo_but, UI_SCREEN_W);
            lbxfont_print_str_center(x + 12, y + 64, game_str_tbl_nt_adj[i], UI_SCREEN_W);
        }
    }
#endif
}

static void ui_newtech_adjust(struct newtech_data_s *d)
{
    int16_t oi_tbl[3], oi_y, oi_n;
    int x = 150, y = 30;
    bool flag_done = false;
    uiobj_set_callback_and_delay(newtech_adjust_draw_cb, d, 1);
    uiobj_table_clear();
    oi_y = UIOBJI_INVALID;
    oi_n = UIOBJI_INVALID;
    UIOBJI_SET_TBL_INVALID(oi_tbl);
    if (d->dialog_type == 0) {
        oi_y = uiobj_add_t0(x + 83, y + 60, "", ui_data.gfx.starmap.scrapbut_yes, MOO_KEY_y, -1);
        oi_n = uiobj_add_t0(x + 18, y + 60, "", ui_data.gfx.starmap.scrapbut_no, MOO_KEY_n, -1);
    } else {
        lbxfont_select(2, 6, 0, 0);
        oi_n = uiobj_add_t0(x + 10, y + 60, game_str_tbl_nt_adj[0], d->gfx_robo_but, MOO_KEY_n, -1);
        oi_tbl[0] = uiobj_add_t0(x + 42, y + 60, game_str_tbl_nt_adj[1], d->gfx_robo_but, MOO_KEY_2, -1);
        oi_tbl[1] = uiobj_add_t0(x + 74, y + 60, game_str_tbl_nt_adj[2], d->gfx_robo_but, MOO_KEY_5, -1);
        oi_tbl[2] = uiobj_add_t0(x + 106, y + 60, game_str_tbl_nt_adj[3], d->gfx_robo_but, MOO_KEY_7, -1);

    }
    while (!flag_done) {
        int16_t oi;
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if ((oi == UIOBJI_ESC) || (oi == oi_n)) {
            ui_sound_play_sfx_06();
            flag_done = true;
        }
        if (oi == oi_y) {
            ui_sound_play_sfx_24();
            game_update_tech_util(d->g);
            game_update_eco_on_waste(d->g, d->api, true);
            flag_done = true;
        }
        for (int i = 0; i < 3; ++i) {
            if (oi == oi_tbl[i]) {
                const int tbl_si[5] = { 0, 3, 2, 3, 3 };
                const int tbl_gr[5] = { 0, 0, 0, 1, 2 };
                ui_sound_play_sfx_24();
                game_update_tech_util(d->g);
                game_update_eco_on_waste(d->g, d->api, true);
                game_planet_adjust_percent(d->g, d->api, tbl_si[d->dialog_type - 1], game_num_tbl_tech_autoadj[i + 1], tbl_gr[d->dialog_type - 1]);
                flag_done = true;
            }
        }
        if (!flag_done) {
            newtech_adjust_draw_cb(d);
            ui_draw_finish();
        }
        ui_delay_ticks_or_click(1);
    }
    uiobj_table_clear();
}

static void ui_newtech_do(struct newtech_data_s *d)
{
    struct game_s *g = d->g;
    empiretechorbit_t *e = &(g->eto[d->api]);
    uint8_t tech = d->nt.tech;
    bool flag_dialog;
    if (d->cur_source != d->nt.source) {
        if ((d->nt.source <= TECHSOURCE_FOUND) || (d->cur_source != TECHSOURCE_RESEARCH)) {
            int m;
            m = ((d->nt.source == TECHSOURCE_RESEARCH) || (d->nt.source > TECHSOURCE_FOUND)) ? d->music_i : newtech_music_tbl[d->nt.source];
            ui_sound_play_music(m);
        }
        d->flag_music = true;
        d->cur_source = d->nt.source;
    }
    flag_dialog = false;
    if (tech < 51) {
        flag_dialog = false;
        if ((d->nt.field == TECH_FIELD_CONSTRUCTION) && (((tech - 5) % 10) == 0) && (g->evn.best_wastereduce[d->api] < tech)) {
            flag_dialog = true;
            d->dialog_type = 0;
            g->evn.best_wastereduce[d->api] = tech;
        }
        if ((d->nt.field == TECH_FIELD_PLANETOLOGY) && (g->evn.best_ecorestore[d->api] < tech)) {
            if (0
              || (tech == TECH_PLAN_IMPROVED_ECO_RESTORATION)
              || (tech == TECH_PLAN_ENHANCED_ECO_RESTORATION)
              || (tech == TECH_PLAN_ADVANCED_ECO_RESTORATION)
              || (tech == TECH_PLAN_COMPLETE_ECO_RESTORATION)
            ) {
                flag_dialog = true;
                d->dialog_type = 0;
                g->evn.best_ecorestore[d->api] = tech;
            }
        }
        if (e->race == RACE_SILICOID) {
            flag_dialog = false;
        }
        if ((d->nt.field == TECH_FIELD_COMPUTER) && (((tech - 8) % 10) == 0) && (g->evn.best_roboctrl[d->api] < tech)) {
            flag_dialog = true;
            d->dialog_type = 1;
            g->evn.best_roboctrl[d->api] = tech;
        }
        if (d->nt.field == TECH_FIELD_PLANETOLOGY) {
            if ((((tech - 2) % 6) == 0) && (g->evn.best_terraform[d->api] < tech)) {
                flag_dialog = true;
                d->dialog_type = 2;
                g->evn.best_terraform[d->api] = tech;
            }
            if ((tech == TECH_PLAN_SOIL_ENRICHMENT) || (tech == TECH_PLAN_ADVANCED_SOIL_ENRICHMENT)) {
                flag_dialog = true;
                d->dialog_type = 4;
            }
            if (tech == TECH_PLAN_ATMOSPHERIC_TERRAFORMING) {
                flag_dialog = true;
                d->dialog_type = 5;
            }
        }
        if ((d->nt.field == TECH_FIELD_FORCE_FIELD) && (((tech - 12) % 10) == 0)) {
            flag_dialog = true;
            d->dialog_type = 3;
        }
    }
    if ((ui_draw_finish_mode == 0) && d->flag_fadeout) {
        ui_palette_fadeout_a_f_1();
        /* FIXME temp_turn_hmm3 = 1; */
        ui_draw_finish_mode = 2;
    }
    d->flag_fadeout = false;
again:
    if (d->flag_choose_next) {
        d->num_next = game_tech_get_next_techs(g, d->api, d->nt.field, d->tech_next);
        if (d->num_next == 0) {
            return;
        }
        d->nt.frame = false;
        d->nt.source = TECHSOURCE_CHOOSE;
        ui_newtech_choose_next(d);
    } else {
        bool flag_done;
        int16_t oi_ok, oi_o1, oi_o2;
        oi_ok = UIOBJI_INVALID;
        oi_o1 = UIOBJI_INVALID;
        oi_o2 = UIOBJI_INVALID;
        flag_done = false;
        uiobj_set_callback_and_delay(newtech_draw_cb1, d, 1);
        uiobj_table_clear();
        if (!d->nt.frame) {
            oi_ok = uiobj_add_mousearea(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, MOO_KEY_UNKNOWN, -1);
        } else {
            oi_o1 = uiobj_add_mousearea(50, 106, 110, 120, MOO_KEY_UNKNOWN, -1);
            oi_o2 = uiobj_add_mousearea(122, 106, 183, 120, MOO_KEY_UNKNOWN, -1);
        }
        while (!flag_done) {
            int16_t oi;
            ui_delay_prepare();
            oi = uiobj_handle_input_cond();
            if ((oi == UIOBJI_ESC) || (oi == oi_ok)) {
                flag_done = true;
            }
            if ((oi == oi_o1) || (oi == oi_o2)) {
                int v;
                flag_done = true;
                ui_sound_play_sfx_24();
                v = -(rnd_1_n(12, &g->seed) + rnd_1_n(12, &g->seed));
                game_diplo_act(g, v, (oi == oi_o1) ? d->other1 : d->other2, d->nt.stolen_from, 5, 0, 0);
            }
            newtech_draw_cb1(d);
            ui_draw_finish();
            ui_delay_ticks_or_click(3);
        }
        uiobj_table_clear();
        if (flag_dialog) {
            ui_newtech_adjust(d);
        }
        if (d->flag_is_current) {
            d->flag_is_current = false;
            d->flag_choose_next = true;
            d->nt.frame = false;
            goto again;
        }
    }
    uiobj_unset_callback();
}

/* -------------------------------------------------------------------------- */

void ui_newtech(struct game_s *g, int pi)
{
    struct newtech_data_s d;
    empiretechorbit_t *e = &(g->eto[pi]);
    bool flag_copybuf = false;

    ui_sound_stop_music();
    uiobj_set_xyoff(0, 0);

    memset(&d, 0, sizeof(d));
    d.g = g;
    d.api = pi;
    d.flag_fadeout = true;
    d.cur_source = -1;
    d.flag_music = false;
    d.anim = 0;

    newtech_load_data(&d);

    for (int i = 0; i < g->evn.newtech[pi].num; ++i) {
        d.nt = g->evn.newtech[pi].d[i];
        if (!flag_copybuf) {
            flag_copybuf = true;
            vgabuf_copy_back_from_page2();
            vgabuf_copy_back_to_page3();
        }
        d.flag_is_current = false;
        d.flag_choose_next = false;
        if (g->eto[pi].tech.project[d.nt.field] == d.nt.tech) {
            d.flag_is_current = true;
        }
        ui_draw_erase_buf();
        d.gfx_lab = lbxfile_item_get(LBXFILE_TECHNO, (d.nt.source != TECHSOURCE_TRADE) ? d.nt.source : 0, 0);
        lbxgfx_draw_frame(0, 0, d.gfx_lab, UI_SCREEN_W);
        {
            int v;
            if (d.nt.tech <= 50) {
                v = game_tech_get_gfx_i(g->gaux, d.nt.field, d.nt.tech);
            } else {
                const int tbl[TECH_FIELD_NUM] = { 9, 22, 20, 25, 15, 29 };
                v = tbl[d.nt.field];
            }
            d.gfx_tech = lbxfile_item_get(LBXFILE_TECHNO, v, 0);
        }
        lbxgfx_draw_frame(145, 54, d.gfx_tech, UI_SCREEN_W);
        vgabuf_copy_back_to_page2();
        {
            int v;
            if (d.nt.source == TECHSOURCE_TRADE) {
                v = g->eto[d.nt.v06].race;
            } else {
                v = d.nt.source * 10 + e->race;
            }
            if (d.gfx_spies) {
                lbxfile_item_release(LBXFILE_SPIES, d.gfx_spies);
            }
            d.gfx_spies = lbxfile_item_get(LBXFILE_SPIES, v, 0);
        }
        d.music_i = newtech_music_tbl[d.nt.source];
        /* FIXME move below to game/ */
        if (d.nt.frame) {
            const empiretechorbit_t *et;
            et = &(g->eto[d.nt.stolen_from]);
            d.other1 = PLAYER_NONE;
            d.other2 = PLAYER_NONE;
            for (int i = 0; (i < g->players) && (d.other2 == PLAYER_NONE); ++i) {
                if ((i != pi) && BOOLVEC_IS1(et->within_frange, i)) {
                    if (d.other1 == PLAYER_NONE) {
                        d.other1 = i;
                    } else {
                        d.other2 = i;
                    }
                }
            }
            if ((d.other2 == PLAYER_NONE) || (d.nt.source != TECHSOURCE_SPY)) {
                d.nt.frame = false;
            }
        }
        ui_newtech_do(&d);
        lbxfile_item_release(LBXFILE_TECHNO, d.gfx_lab);
        lbxfile_item_release(LBXFILE_TECHNO, d.gfx_tech);
    }
    for (tech_field_t field = 0; field < TECH_FIELD_NUM; ++field) {
        if (1
          && (e->tech.investment[field] != 0)
          && (e->tech.project[field] == 0)
          && (e->tech.percent[field] < 99)
        ) {
            if (!flag_copybuf) {
                flag_copybuf = true;
                vgabuf_copy_back_from_page2();
                vgabuf_copy_back_to_page3();
            }
            if (d.cur_source == -1) {
                /*soundsys_hmm3?*/
                ui_draw_erase_buf();
                d.gfx_lab = lbxfile_item_get(LBXFILE_TECHNO, 0, 0);
                lbxgfx_draw_frame(0, 0, d.gfx_lab, UI_SCREEN_W);
                vgabuf_copy_back_to_page2();
                lbxfile_item_release(LBXFILE_TECHNO, d.gfx_lab);
                if (d.gfx_spies) {
                    lbxfile_item_release(LBXFILE_SPIES, d.gfx_spies);
                }
                d.gfx_spies = lbxfile_item_get(LBXFILE_SPIES, e->race, 0);
                d.music_i = newtech_music_tbl[0];
            }
            d.nt.field = field;
            d.nt.tech = 0;
            d.nt.source = TECHSOURCE_CHOOSE;
            d.flag_choose_next = true;
            d.nt.frame = false;
            ui_newtech_do(&d);
        }
    }
    if (d.flag_music) {
        hw_audio_music_fadeout();
    }
    if (flag_copybuf) {
        vgabuf_copy_back_from_page3();
        vgabuf_copy_back_to_page2();
        ui_palette_fadeout_a_f_1();
        ui_draw_finish_mode = 2;
    }
    g->evn.newtech[pi].num = 0;
    newtech_free_data(&d);
}
