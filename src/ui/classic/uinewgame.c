#include "config.h"

#include <string.h>

#include "ui.h"
#include "game.h"
#include "game_ai.h"
#include "game_new.h"
#include "game_str.h"
#include "gfxaux.h"
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
#include "uinewgame.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "util.h"
#include "pbx.h"
#include "comp.h"
#include "rnd.h"

/* -------------------------------------------------------------------------- */

static const uint8_t tbl_cursor_color[] = { 0x91, 0x91, 0x8d, 0x8d, 0x89, 0x89, 0x84, 0x84, 0x7f, 0x7f };

/* -------------------------------------------------------------------------- */

struct new_game_data_s {
    struct game_s *g;
    struct game_new_options_s *newopts;
    int frame;
    bool fadein;
    int section;
    const char *descr;
    int16_t selected;
    player_id_t pi;
    bool have_human;
    const char *str_title;
    int edit_mode;
    int w,h;
    uint8_t *gfx_newgame;
    uint8_t *gfx_optb_ng;
    uint8_t *gfx_optb_cancel;
    uint8_t *gfx_optb_ok;
    uint8_t *gfx_custom;
    uint8_t *gfx_flag[BANNER_NUM];
    uint8_t *gfx_portrait[RACE_NUM];
    char *str_tbl_2space_race[RACE_NUM + 2];
    char *str_tbl_2space_banner[BANNER_NUM + 2];
};

static void new_game_load_data(struct new_game_data_s *d)
{
    d->gfx_newgame = lbxfile_item_get(LBXFILE_VORTEX, 5);
    d->gfx_optb_ng = lbxfile_item_get(LBXFILE_VORTEX, 6);
    d->gfx_optb_cancel = lbxfile_item_get(LBXFILE_VORTEX, 0x1a);
    d->gfx_optb_ok = lbxfile_item_get(LBXFILE_VORTEX, 0x1b);
    d->gfx_custom = lbxfile_item_get(LBXFILE_VORTEX, 9);
    for (int i = 0; i < BANNER_NUM; ++i) {
        const banner_t t[BANNER_NUM] = { /* wrong order in lbx */
            BANNER_GREEN, BANNER_BLUE, BANNER_RED, BANNER_WHITE, BANNER_YELLOW, BANNER_PURPLE
        };
        d->gfx_flag[t[i]] = lbxfile_item_get(LBXFILE_VORTEX, 0xa + i);
    }
    for (int i = 0; i < RACE_NUM; ++i) {
        d->gfx_portrait[i] = lbxfile_item_get(LBXFILE_VORTEX, 0x10 + i);
    }
    for (int i = 0; i < RACE_NUM + 1; ++i) {
        d->str_tbl_2space_race[i] = util_concat("  ", game_str_tbl_race[i], NULL);
    }
    d->str_tbl_2space_race[RACE_NUM + 1] = 0;
    for (int i = 0; i < BANNER_NUM + 1; ++i) {
        d->str_tbl_2space_banner[i] = util_concat("  ", game_str_tbl_banner[i], NULL);
    }
    d->str_tbl_2space_banner[BANNER_NUM + 1] = 0;
}

static void new_game_free_data(struct new_game_data_s *d)
{
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_newgame);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_optb_ng);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_optb_cancel);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_optb_ok);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_custom);
    for (int i = 0; i < BANNER_NUM; ++i) {
        lbxfile_item_release(LBXFILE_VORTEX, d->gfx_flag[i]);
        lib_free(d->str_tbl_2space_banner[i]);
    }
    lib_free(d->str_tbl_2space_banner[BANNER_NUM + 1]);
    for (int i = 0; i < RACE_NUM; ++i) {
        lbxfile_item_release(LBXFILE_VORTEX, d->gfx_portrait[i]);
        lib_free(d->str_tbl_2space_race[i]);
    }
    lib_free(d->str_tbl_2space_race[RACE_NUM + 1]);
}

static void new_game_draw_cb1(void *vptr)
{
    struct new_game_data_s *d = vptr;
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, d->gfx_newgame, UI_SCREEN_W, ui_scale);
}

static void new_game_draw_race_cb(void *vptr)
{
    struct new_game_data_s *d = vptr;
    hw_video_copy_back_from_page2();
    if (d->selected < RACE_NUM) {
        lbxgfx_draw_frame(91, 11, d->gfx_portrait[d->selected], UI_SCREEN_W, ui_scale);
        lbxfont_select(0, 4, 0, 0);
        for (int i = 0; i < 3; ++i) {
            if (*game_str_tbl_traits[d->selected * 3 + i]) {
                lbxfont_print_str_center(0x6f, 0x35 + i * 8, game_str_tbl_traits[d->selected * 3 + i], UI_SCREEN_W, ui_scale);
            }
        }
    }
}

static int16_t ui_new_game_choose_race(struct game_new_options_s *newopts, struct new_game_data_s *d)
{
    int16_t race;
    d->selected = 0;
    new_game_draw_race_cb(d);
    uiobj_finish_frame();
    if (d->fadein) {
        d->fadein = false;
        ui_palette_fadein_4b_19_1();
    }
    uiobj_set_callback_and_delay(new_game_draw_race_cb, d, 2);
    uiobj_table_clear();
    lbxfont_select(5, 0xf, 0, 0);
    race = uiobj_select_from_list1(0xa, 0xa, 0x32, game_str_ng_choose_race, (char const * const *)d->str_tbl_2space_race, &d->selected, 0, 0xf, 2, true);
    if (race == -1) {
        ui_sound_play_sfx_06();
        return -1;
    }
    ui_sound_play_sfx_24();
    newopts->pdata[d->pi].race = race;
    return race;
}

static void new_game_draw_banner_cb(void *vptr)
{
    struct new_game_data_s *d = vptr;
    race_t race = d->newopts->pdata[d->pi].race;
    hw_video_copy_back_from_page2();
    if (d->str_title) {
        lbxfont_select(5, 0, 0, 0);
        lbxfont_print_str_normal(0xa, 0xa, d->str_title, UI_SCREEN_W, ui_scale);
        lbxgfx_apply_colortable(0xa, 0x14, 0x52, 0x1f, 2, UI_SCREEN_W, ui_scale);
    }
    if (race < RACE_NUM) {
        lbxgfx_draw_frame(91, 11, d->gfx_portrait[race], UI_SCREEN_W, ui_scale);
    }
    ui_draw_filled_rect(0x5a, 0x35, 0x83, 0x5a, 0, ui_scale);
    ui_draw_box1(0x5a, 0x35, 0x83, 0x5a, 0x9b, 0x9b, ui_scale);
    if (d->selected < BANNER_NUM) {
        lbxgfx_set_new_frame(d->gfx_flag[d->selected], d->frame);
        gfx_aux_draw_frame_to(d->gfx_flag[d->selected], &ui_data.aux.screen);
        gfx_aux_draw_frame_from(0x5b, 0x38, &ui_data.aux.screen, UI_SCREEN_W, ui_scale);
    }
    if (++d->frame == 0xa) {
        d->frame = 0;
    }
}

static int16_t ui_new_game_choose_banner(struct game_new_options_s *newopts, struct new_game_data_s *d)
{
    int16_t banner;
    d->selected = 0;
    d->frame = 0;
    d->str_title = 0;
    uiobj_table_clear();
    uiobj_set_callback_and_delay(new_game_draw_banner_cb, d, 2);
    lbxfont_select(5, 0xf, 0, 0);
    banner = uiobj_select_from_list1(0xa, 0xa, 0x32, game_str_ng_choose_banner, (char const * const *)d->str_tbl_2space_banner, &d->selected, 0, 0xf, 2, true);
    if (banner == -1) {
        ui_sound_play_sfx_06();
        return -1;
    }
    ui_sound_play_sfx_24();
    newopts->pdata[d->pi].banner = banner;
    return banner;
}

static bool ui_new_game_pname(struct game_new_options_s *newopts, struct new_game_data_s *d, bool flag_generate)
{
    char buf[32];
    bool flag_ok;

    if (!flag_generate) {
        lib_strcpy(buf, newopts->pdata[d->pi].playername, sizeof(buf));
        flag_generate = (buf[0] == '\0');
    }
    uiobj_set_callback_and_delay(new_game_draw_banner_cb, d, 2);
    d->str_title = game_str_ng_your_name;
    uiobj_table_clear();
    flag_ok = false;
    while (!flag_ok) {
        lbxfont_select(5, 0xf, 0xf, 0xf);
        if (flag_generate) {
            game_new_generate_emperor_name(d->newopts->pdata[d->pi].race, buf, sizeof(buf));
        }
        if (!uiobj_read_str(0xf, 0x16, 0x41, buf, 0xb/*len*/, 0, 0, tbl_cursor_color)) {
            return false;
        }
        util_trim_whitespace(buf, sizeof(buf));
        flag_ok = buf[0] != '\0';
        flag_generate = true;
    }
    lib_strcpy(newopts->pdata[d->pi].playername, buf, EMPEROR_NAME_LEN);
    ui_sound_play_sfx_24();
    return true;
}

static bool ui_new_game_hname(struct game_new_options_s *newopts, struct new_game_data_s *d, bool flag_generate)
{
    char buf[32];
    bool flag_ok;

    if (!flag_generate) {
        lib_strcpy(buf, newopts->pdata[d->pi].homename, sizeof(buf));
        flag_generate = (buf[0] == '\0');
    }
    uiobj_set_callback_and_delay(new_game_draw_banner_cb, d, 2);
    d->str_title = game_str_ng_home_name;
    uiobj_table_clear();
    flag_ok = false;
    while (!flag_ok) {
        lbxfont_select(5, 0xf, 0xf, 0xf);
        if (flag_generate) {
            game_new_generate_home_name(d->newopts->pdata[d->pi].race, buf, sizeof(buf));
        }
        if (!uiobj_read_str(0xf, 0x16, 0x32, buf, PLANET_NAME_LEN, 0, 0, tbl_cursor_color)) {
            return false;
        }
        util_trim_whitespace(buf, sizeof(buf));
        flag_ok = buf[0] != '\0';
        flag_generate = true;
    }
    lib_strcpy(newopts->pdata[d->pi].homename, buf, PLANET_NAME_LEN);
    ui_sound_play_sfx_24();

    lbxfont_select(5, 1, 0xf, 0xf);
    return true;
}

static bool ui_new_game_racebannernames(struct game_new_options_s *newopts, struct new_game_data_s *d)
{
    d->pi = PLAYER_0;
    /* orion.exe inits game->offs02 and game_new:researchflag here for whatever reason */
    uiobj_table_clear();
    ui_palette_fadeout_19_19_1();
    d->fadein = true;
    lbxpal_select(4, -1, 0);
    lbxpal_build_colortables();
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, d->gfx_custom, UI_SCREEN_W, ui_scale);
    ui_draw_box1(0x5a, 0xa, 0x83, 0x2d, 0x9b, 0x9b, ui_scale);
    hw_video_copy_back_to_page2();
    if (0
      || (ui_new_game_choose_race(newopts, d) < 0)
      || (ui_new_game_choose_banner(newopts, d) < 0)
      || (!ui_new_game_pname(newopts, d, true))
      || (!ui_new_game_hname(newopts, d, true))
    ) {
        return false;
    }
    return true;
}

/* -------------------------------------------------------------------------- */

#define PORTRAITBOX_TOP_MARGIN 20
#define PORTRAITBOX_H 46

static void new_game_print_shadow_right(int x, int y, uint8_t c1, uint8_t c2, const char *str)
{
    lbxfont_set_color_c_n(c2, 8);
    lbxfont_print_str_right(x + 1, y + 1, str, UI_SCREEN_W, ui_scale);
    lbxfont_set_color_c_n(c1, 8);
    lbxfont_print_str_right(x, y, str, UI_SCREEN_W, ui_scale);
}

static void new_game_print_shadow_left(int x, int y, uint8_t c1, uint8_t c2, const char *str)
{
    lbxfont_set_color_c_n(c2, 8);
    lbxfont_print_str_normal(x + 1, y + 1, str, UI_SCREEN_W, ui_scale);
    lbxfont_set_color_c_n(c1, 8);
    lbxfont_print_str_normal(x, y, str, UI_SCREEN_W, ui_scale);
}

#define EDIT_MODES 8
#define GALOPT_Y0    20
#define PLANOPT_Y0  123

static void new_game_draw_extra_cb(void *vptr)
{
    struct new_game_data_s *d = vptr;
    struct game_new_options_s *newopts = d->newopts;
    const uint8_t *ropt = d->g->popt;
    const uint8_t *nopt = d->newopts->popt;
    gameopts_t *ro = &d->g->opt;
    newopts_t  *no = &d->newopts->opt;
    char buf[64];
    hw_video_copy_back_from_page3();
    lbxfont_select(5, 0, 3, 0);
    lbxfont_print_str_center(30, 180, game_str_ng_cancel, UI_SCREEN_W, ui_scale);
    lbxfont_print_str_center(90, 180, "\x2Players\x1" + (d->section != 1), UI_SCREEN_W, ui_scale);
    lbxfont_print_str_center(150, 180, "\x2Rules\x1" + (d->section != 2), UI_SCREEN_W, ui_scale);
    lbxfont_print_str_center(210, 180, "\x2Galaxy\x1" + (d->section != 3), UI_SCREEN_W, ui_scale);
    lbxfont_print_str_center(275, 180, game_str_ng_ok, UI_SCREEN_W, ui_scale);
    if (d->section==1) { /* Players */
        for (int i = 0; i < newopts->players; ++i) {
            int x0 = 4 + (i / 3) * 160;
            int y0 = 20 + (i % 3) * PORTRAITBOX_H;
            ui_draw_box1(x0, y0, x0 + 41, y0 + 35, 0x9b, 0x9b, ui_scale);
            ui_draw_filled_rect(x0 + 43, y0, x0 + 43 + 41, y0 + 35, 0, ui_scale);
            ui_draw_box1(x0 + 43, y0, x0 + 43 + 41, y0 + 35, 0x9b, 0x9b, ui_scale);
        }
        lbxfont_select(5, 0, 0, 0);
        for (player_id_t i = 0; i < d->newopts->players; ++i) {
            int x0 = 4 + (i / 3) * 160;
            int y0 = PORTRAITBOX_TOP_MARGIN + (i % 3) * PORTRAITBOX_H;
            if (newopts->pdata[i].race < RACE_NUM) {
                lbxgfx_draw_frame(x0 + 1, y0 + 1, d->gfx_portrait[newopts->pdata[i].race], UI_SCREEN_W, ui_scale);
            }
            if (newopts->pdata[i].banner < BANNER_NUM) {
                lbxgfx_set_new_frame(d->gfx_flag[newopts->pdata[i].banner], d->frame);
                gfx_aux_draw_frame_to(d->gfx_flag[newopts->pdata[i].banner], &ui_data.aux.screen);
                gfx_aux_draw_frame_from(x0 + 43 + 1, y0 + 1, &ui_data.aux.screen, UI_SCREEN_W, ui_scale);
            }
            lbxfont_print_str_normal(x0 + 43 + 41 + 4, y0 + 2 , d->newopts->pdata[i].playername, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(x0 + 43 + 41 + 4, y0 + 2 + 11, d->newopts->pdata[i].homename, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(x0 + 43 + 41 + 4, y0 + 2 + 22, d->newopts->pdata[i].is_ai ? game_str_ng_computer : game_str_ng_player, UI_SCREEN_W, ui_scale);
        }
        if (!d->have_human) {
            lbxfont_print_str_center(160, 4, game_str_ng_allai, UI_SCREEN_W, ui_scale);
        }
        lbxfont_select(0, 0, 0, 0);
        lbxfont_print_str_normal(50, 163, game_ais[d->newopts->ai_id]->name, UI_SCREEN_W, ui_scale);
        lbxfont_select(0, 3, 0, 0);
        lbxfont_print_str_normal(90, 163, game_ais[d->newopts->ai_id]->description, UI_SCREEN_W, ui_scale);
        if (++d->frame >= 10) {
            d->frame = 0;
        }
    } else if (d->section==2) { /* Rules */
        lbxfont_select(5, 0, 0, 0);
        lbxfont_print_str_center(80, 5, "Rules", UI_SCREEN_W, ui_scale);
        lbxfont_print_str_center(250, 5, "Events", UI_SCREEN_W, ui_scale);
        new_game_print_shadow_right(150, 20, 71, 19, "default");
        new_game_print_shadow_right(150, 35, 71, 19, "RBO");
        new_game_print_shadow_right(300, 20, 208, 206, "default");
        new_game_print_shadow_right(300, 35, 208, 206, "good");
        new_game_print_shadow_right(300, 50, 208, 206, "off");
        lbxfont_print_num_right(300, 85, 2300 + d->g->evn.year, UI_SCREEN_W, ui_scale);
        lbxfont_select(0, 0, 0, 0);
        new_game_print_shadow_right(300, 75, 208, 206, "not before");
        for (int i = 0; i < GAMEOPTS; ++i) {
            lbxfont_set_color_c_n(ropt[i] == gameopt_descr[i].dflt ? 19 : 71, 9);
            lib_sprintf(buf, sizeof(buf), "%s: %s",gameopt_descr[i].name, gameopt_descr[i].opt[ropt[i]]);
            lbxfont_print_str_normal(20, 20 + 8 * i, buf, UI_SCREEN_W, ui_scale);
        }
        for (int i = 1; i < GAME_EVENT_NUM; ++i) {
            bool is_on = BOOLVEC_IS0(d->g->evn.done, i);
            lbxfont_set_color_c_n(is_on ? 208 : 207, 9);
            lib_sprintf(buf, sizeof(buf), "%s: %s",game_str_tbl_event[i], is_on ? "on" : "off");
            lbxfont_print_str_normal(200, 12 + 8 * i, buf, UI_SCREEN_W, ui_scale);
        }
        if (d->descr) {
            lbxfont_set_gap_h(1);
            lbxfont_set_color_c_n(187, 8);
            lbxfont_print_str_split(21, 157, 285, d->descr, 0, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
            lbxfont_set_color_c_n(166, 8);
            lbxfont_print_str_split(20, 156, 285, d->descr, 0, UI_SCREEN_W, UI_SCREEN_H, ui_scale);
        }
    } else { /* Galaxy */
        const uint8_t *nopt = d->newopts->popt;
        lbxfont_select(5, 0, 0, 0);
        lbxfont_print_str_center(212, 87, "Galaxy Options currently not implemented", UI_SCREEN_W, ui_scale);
        lbxfont_print_str_center(55, GALOPT_Y0 - 15, "Galaxy", UI_SCREEN_W, ui_scale);
        lbxfont_print_str_center(55, PLANOPT_Y0 - 15, "Planets", UI_SCREEN_W, ui_scale);
/*
        new_game_print_shadow_left(10, 85, 71, 19, "Reset");
        new_game_print_shadow_left(70, 85, 71, 19, "Edit");
*/
        ui_draw_box1(110, 10, 310+3, 170+3, 0xfa, 0xfc, ui_scale);
        lbxfont_select(0, 0, 0, 0);
        {
            lib_sprintf(buf, sizeof(buf), "Type: %s",game_str_tbl_gtype[d->newopts->type]);
            lbxfont_print_str_normal(10, GALOPT_Y0, buf, UI_SCREEN_W, ui_scale);
            lib_sprintf(buf, sizeof(buf), "Stars: %d",d->newopts->stars);
            lbxfont_print_str_normal(10, GALOPT_Y0+8, buf, UI_SCREEN_W, ui_scale);
            lib_sprintf(buf, sizeof(buf), "Seed: %08X",d->newopts->galaxy_seed);
            lbxfont_print_str_normal(10, GALOPT_Y0+16, buf, UI_SCREEN_W, ui_scale);
        }
        for (int i = 0; i < NEWOPTS; ++i) {
            int y = i < GALAXYOPTS ? 8 * i + GALOPT_Y0+24 : 8 * (i-GALAXYOPTS) + PLANOPT_Y0;
            lbxfont_set_color_c_n(nopt[i] == newopt_descr[i].dflt ? 19 : 71, 9);
            lib_sprintf(buf, sizeof(buf), "%s: %s",newopt_descr[i].name, newopt_descr[i].opt[nopt[i]]);
            lbxfont_print_str_normal(10, y, buf, UI_SCREEN_W, ui_scale);
        }
    }
}

#define SW1_MAX 23   /* 160 stars */

static bool ui_new_game_extra(struct new_game_data_s *d)
{
    bool flag_done = false, flag_ok = false;
    int16_t oi_cancel, oi_ok, oi_players, oi_rules, oi_galaxy, oi_ai_id,
            oi_option[GAMEOPTS], oi_event[GAME_EVENT_TBL_NUM], oi_optdef, oi_rbo, oi_evdef, oi_good, oi_evoff, oi_year,
            oi_noption[NEWOPTS], oi_type, oi_stars, oi_starwheel, oi_seed, oi_map,
            oi_types[GALAXY_TYPE_NUM], oi_empty, oi_load, oi_reset, oi_edit[EDIT_MODES],
            oi_race[PLAYER_NUM], oi_banner[PLAYER_NUM], oi_pname[PLAYER_NUM], oi_hname[PLAYER_NUM], oi_ai[PLAYER_NUM];
    int16_t starwheel = d->newopts->galaxy_size, sw_min = 0, sw_max = GALAXY_SIZE_NUM - 1;
    const uint8_t sw_tbl0[GALAXY_SIZE_NUM] = { 24, 48, 70, 108 };            /* std. sizes */
    const uint8_t sw_tbl1[28] = { 8, 8, 8, 10, 12, 14, 16, 18, 20, 24,       /* small */
                                  28, 32, 36, 40, 48,  56, 64, 72, 80, 96,   /* medium + large */
                                  112, 128, 144, 160, 180, 200, 220, 240 };  /* huge */
    const uint8_t sw1_idx[GALAXY_SIZE_NUM] = { 9, 13, 17, 20 };
    const uint8_t *sw_tbl = sw_tbl0;
    struct game_new_options_s *newopts = d->newopts;
    uint8_t *ropt = d->g->popt;
    uint8_t *nopt = d->newopts->popt;
    gameopts_t *ro = &d->g->opt;
    newopts_t  *no = &d->newopts->opt;
    d->pi = PLAYER_0;
    d->str_title = 0;
    d->frame = 0;
    d->section = 1;
    d->edit_mode = 0;
    d->newopts->stars = sw_tbl[starwheel];

    newopts->pdata[PLAYER_0].race = RACE_HUMAN;
    newopts->pdata[PLAYER_0].banner = BANNER_BLUE;
    d->have_human = true;
    game_new_generate_emperor_name(newopts->pdata[PLAYER_0].race, newopts->pdata[PLAYER_0].playername, EMPEROR_NAME_LEN);
    game_new_generate_home_name(newopts->pdata[PLAYER_0].race, newopts->pdata[PLAYER_0].homename, PLANET_NAME_LEN);
    uiobj_table_clear();
    ui_palette_fadeout_19_19_1();
    d->fadein = true;
    lbxpal_select(4, -1, 0);
    lbxpal_build_colortables();
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, d->gfx_custom, UI_SCREEN_W, ui_scale);
    hw_video_copy_back_to_page3();
    hw_video_copy_back_to_page2();
    hw_video_copy_back_from_page3();

restart:
    d->descr = NULL;
    uiobj_table_clear();
    oi_cancel   = uiobj_add_mousearea(  0, 170,  55, 199, MOO_KEY_ESCAPE);
    oi_players  = uiobj_add_mousearea( 65, 170, 115, 199, MOO_KEY_ESCAPE);
    oi_rules    = uiobj_add_mousearea(125, 170, 175, 199, MOO_KEY_ESCAPE);
    oi_galaxy   = uiobj_add_mousearea(185, 170, 235, 199, MOO_KEY_ESCAPE);
    oi_ok       = uiobj_add_mousearea(245, 170, 300, 199, MOO_KEY_SPACE);
    oi_ai_id = UIOBJI_INVALID;
    for (int i = 0; i < PLAYER_NUM; ++i) {
         oi_race[i] = UIOBJI_INVALID;
         oi_banner[i] = UIOBJI_INVALID;
         oi_pname[i] = UIOBJI_INVALID;
         oi_hname[i] = UIOBJI_INVALID;
         oi_ai[i] = UIOBJI_INVALID;
    }
    for (int i = 0; i < GAMEOPTS; ++i) oi_option[i] = UIOBJI_INVALID;
    for (int i = 0; i < GAME_EVENT_TBL_NUM; ++i) oi_event[i] = UIOBJI_INVALID;
    oi_optdef = oi_rbo = oi_evdef = oi_good = oi_evoff = oi_year = UIOBJI_INVALID;
    for (int i = 0; i < NEWOPTS; ++i) oi_noption[i] = UIOBJI_INVALID;
    oi_type = oi_stars = oi_starwheel = oi_seed = oi_map = UIOBJI_INVALID;
    for (int i = 0; i < GALAXY_TYPE_NUM; ++i) oi_types[i] = UIOBJI_INVALID;
    oi_empty = oi_load = oi_reset = UIOBJI_INVALID;
    for (int i = 0; i < EDIT_MODES; ++i) oi_edit[i] = UIOBJI_INVALID;

    if (d->section==1) {
        oi_ai_id    = uiobj_add_mousearea( 45, 160,  90, 170, MOO_KEY_a);
        for (int i = 0; i < newopts->players; ++i) {
            int x0 = 4 + (i / 3) * 160;
            int y0 = PORTRAITBOX_TOP_MARGIN + (i % 3) * PORTRAITBOX_H;
            oi_race[i] = uiobj_add_mousearea(x0, y0, x0 + 41, y0 + 35, MOO_KEY_UNKNOWN);
            oi_banner[i] = uiobj_add_mousearea(x0 + 43, y0, x0 + 43 + 41, y0 + 35, MOO_KEY_UNKNOWN);
            oi_pname[i] = uiobj_add_mousearea(x0 + 43 + 41 + 2, y0 + 2, x0 + 43 + 41 + 2 + 60, y0 + 2 + 10, MOO_KEY_UNKNOWN);
            oi_hname[i] = uiobj_add_mousearea(x0 + 43 + 41 + 2, y0 + 2 + 11, x0 + 43 + 41 + 2 + 60, y0 + 2 + 11 + 10, MOO_KEY_UNKNOWN);
            oi_ai[i] = uiobj_add_mousearea(x0 + 43 + 41 + 2, y0 + 2 + 22, x0 + 43 + 41 + 2 + 60, y0 + 2 + 22 + 10, MOO_KEY_UNKNOWN);
        }
        for (int i = newopts->players; i < PLAYER_NUM; ++i) {
            oi_race[i] = UIOBJI_INVALID;
            oi_banner[i] = UIOBJI_INVALID;
            oi_pname[i] = UIOBJI_INVALID;
            oi_hname[i] = UIOBJI_INVALID;
            oi_ai[i] = UIOBJI_INVALID;
        }
    } else if (d->section==2) {
        for (int i = 0; i < GAMEOPTS; ++i) {
            oi_option[i] = uiobj_add_mousearea(30, 19 + 8 * i, 110, 25 + 8 * i, MOO_KEY_UNKNOWN);
        }
        for (int i = 1; i < GAME_EVENT_NUM; ++i) {
            oi_event[i] = uiobj_add_mousearea(200, 11 + 8 * i, 260, 17 + 8 * i, MOO_KEY_UNKNOWN);
        }
        oi_optdef = uiobj_add_mousearea(120, 20, 150, 30, MOO_KEY_UNKNOWN);
        oi_rbo    = uiobj_add_mousearea(120, 35, 150, 45, MOO_KEY_UNKNOWN);
        oi_evdef  = uiobj_add_mousearea(270, 20, 300, 30, MOO_KEY_UNKNOWN);
        oi_good   = uiobj_add_mousearea(270, 35, 300, 45, MOO_KEY_UNKNOWN);
        oi_evoff  = uiobj_add_mousearea(270, 50, 300, 60, MOO_KEY_UNKNOWN);
        oi_year   = uiobj_add_mousearea(270, 75, 300, 95, MOO_KEY_UNKNOWN);
    } else {
        oi_type =      uiobj_add_mousearea(10, 19, 100, 19 + 6, MOO_KEY_UNKNOWN);
        if (d->edit_mode < 2) {
            oi_stars =     uiobj_add_mousearea(10, 27, 100, 27 + 6, MOO_KEY_UNKNOWN);
            oi_starwheel = uiobj_add_mousewheel(10, 27, 100, 27 + 6, &starwheel);
        }
        oi_seed =      uiobj_add_mousearea(10, 35, 100, 35 + 6, MOO_KEY_UNKNOWN);
        for (int i = 0; i < NEWOPTS; ++i) {
            int y = i < GALAXYOPTS ? 8 * i + GALOPT_Y0+24 : 8 * (i-GALAXYOPTS) + PLANOPT_Y0;
            oi_noption[i] = uiobj_add_mousearea(10, y - 1, 100, y + 5, MOO_KEY_UNKNOWN);
        }
        if (d->edit_mode >= 2) {
            for (int i = 0; i < EDIT_MODES; ++i) {
                oi_edit[i] = UIOBJI_INVALID; /* TODO */
            }
        }
    }

    uiobj_set_callback_and_delay(new_game_draw_extra_cb, d, 2);
    uiobj_set_xyoff(1, 1);
    new_game_draw_extra_cb(d);

    while (!flag_done) {
        int16_t oi,oi2;
        oi = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        bool calc_map = false;
        d->descr = NULL;

        ui_delay_prepare();
        if ((oi != UIOBJI_ESC) && (oi != UIOBJI_NONE) && (oi != oi_cancel)) {
            ui_sound_play_sfx_24();
        }
        if ((oi == UIOBJI_ESC) || (oi == oi_cancel)) {
            ui_sound_play_sfx_06();
            flag_ok = false;
            flag_done = true;
        } else if (oi == oi_ok) {
            if (d->have_human) {
                flag_ok = true;
                flag_done = true;
            }
        } else if (oi == oi_players) {
            d->section = 1;
            goto restart;
        } else if (oi == oi_rules) {
            d->section = 2;
            goto restart;
        } else if (oi == oi_galaxy) {
            d->section = 3;
            goto restart;
        }

        if (d->section == 1) {
            if (oi == oi_ai_id) {
                d->newopts->ai_id = (d->newopts->ai_id + 1) % GAME_AI_NUM_VISIBLE;
            }
            for (int i = 0; d->section == 1 && i < newopts->players; ++i) {
                if (oi == oi_race[i]) {
                    race_t old_race, new_race;
                    old_race = newopts->pdata[i].race;
                    d->pi = i;
                    ui_new_game_choose_race(newopts, d);
                    new_race = newopts->pdata[i].race;
                    if (new_race != old_race) {
                        if (new_race < RACE_NUM) {
                            game_new_generate_emperor_name(new_race, newopts->pdata[i].playername, EMPEROR_NAME_LEN);
                            game_new_generate_home_name(new_race, newopts->pdata[i].homename, PLANET_NAME_LEN);
                        } else {
                            newopts->pdata[i].playername[0] = '\0';
                            newopts->pdata[i].homename[0] = '\0';
                        }
                    }
                    goto restart;
                } else if (oi == oi_banner[i]) {
                    d->pi = i;
                    d->selected = newopts->pdata[i].banner;
                    ui_new_game_choose_banner(newopts, d);
                    goto restart;
                } else if (oi == oi_pname[i]) {
                    d->pi = i;
                    d->selected = newopts->pdata[i].banner;
                    ui_new_game_pname(newopts, d, false);
                    goto restart;
                } else if (oi == oi_hname[i]) {
                    d->pi = i;
                    d->selected = newopts->pdata[i].banner;
                    ui_new_game_hname(newopts, d, false);
                    goto restart;
                } else if (oi == oi_ai[i]) {
                    newopts->pdata[i].is_ai = !newopts->pdata[i].is_ai;
                    d->have_human = false;
                    for (int j = 0; j < newopts->players; ++j) {
                        if (!newopts->pdata[j].is_ai) {
                            d->have_human = true;
                            break;
                        }
                    }
                }
            }

        } else if (d->section == 2) {
            if (oi == oi_optdef) {
                for (int i = 0; i < GAMEOPTS; ++i) ropt[i] = gameopt_descr[i].dflt;
            } else if (oi == oi_rbo) {
                for (int i = 0; i < GAMEOPTS; ++i) ropt[i] = gameopt_descr[i].dflt;
                d->g->opt.retreat = 2;
                d->g->opt.spec_war = 1;
                d->g->opt.fix_bait_yoyo = 3;
                d->g->opt.threats = 2;
            } else if (oi == oi_evdef) {
                BOOLVEC_CLEAR(d->g->evn.done, GAME_EVENT_TBL_NUM);
                d->g->evn.year = 40;
            } else if (oi == oi_good) {
                for (int i = 1; i < GAME_EVENT_NUM; ++i) BOOLVEC_SET1(d->g->evn.done, i);
                BOOLVEC_SET0(d->g->evn.done, GAME_EVENT_DERELICT);
                BOOLVEC_SET0(d->g->evn.done, GAME_EVENT_ENVIRO);
                BOOLVEC_SET0(d->g->evn.done, GAME_EVENT_RICH);
                BOOLVEC_SET0(d->g->evn.done, GAME_EVENT_SUPPORT);
            } else if (oi == oi_evoff) {
                for (int i = 1; i < GAME_EVENT_NUM; ++i) BOOLVEC_SET1(d->g->evn.done, i);
            } else if (oi == oi_year) {
                d->g->evn.year += 60;
                d->g->evn.year -= d->g->evn.year % 50;
                if (d->g->evn.year > 200) d->g->evn.year = 40;
            }
            for (int i = 0; i < GAMEOPTS; ++i) {
                if (oi == oi_option[i]) {
                    ropt[i] += 1;
                    if (ropt[i] >= gameopt_descr[i].opts) ropt[i] = 0;
                }
                if (oi2 == oi_option[i]) {
                    d->descr = gameopt_descr[i].descr;
                    break;
                }
            }
            for (int i = 1; d->section == 2 && i < GAME_EVENT_NUM; ++i) {
                if (oi == oi_event[i]) {
                    BOOLVEC_TOGGLE(d->g->evn.done, i);
                }
            }

        } else if (d->section == 3) {
            if (oi == oi_type) {
                if (d->newopts->type == 1 || d->newopts->type == GALAXY_TYPE_NUM - 1) {
                    d->newopts->type = 0;
                    sw_tbl = sw_tbl0; sw_min=0; sw_max = GALAXY_SIZE_NUM - 1;
                    starwheel = d->newopts->galaxy_size;
                    d->newopts->stars = sw_tbl[starwheel];
                    d->edit_mode = 0;
                    goto restart;
                } if (d->newopts->type == 0) {
                    d->newopts->type = 2;
                    sw_tbl = sw_tbl1; sw_min=d->newopts->players; sw_max = SW1_MAX;
                    starwheel = sw1_idx[d->newopts->galaxy_size];
                    d->newopts->stars = sw_tbl[starwheel];
                    goto restart;
                } else {
                    ++d->newopts->type;
                    calc_map = true;
                }
                /* TODO: enable mask */
            } else if(oi == oi_stars) {
                ++starwheel;
                if (starwheel > sw_max) starwheel = sw_min;
                d->newopts->stars = sw_tbl[starwheel];
                goto restart;
            } else if(oi == oi_starwheel) {
                SETRANGE(starwheel, sw_min, sw_max);
                d->newopts->stars = sw_tbl[starwheel];
                goto restart;
            } else if(oi == oi_seed) {
                d->newopts->galaxy_seed = rnd_get_new_seed();
                calc_map = true;
            } else if(oi == oi_map) {
                if (d->edit_mode == 0) {
                    if(d->newopts->type > 1) d->edit_mode = 1;
                } else if (d->edit_mode == 1) {
                    d->edit_mode = 4;
                } else if (d->edit_mode == 2) { /* set size */
                } else if (d->edit_mode == 3) { /* place nebulas */
                } else if (d->edit_mode == 4) { /* place stars */
                } else if (d->edit_mode == 5) { /* place homeworlds */
                } else if (d->edit_mode == 6) { /* place start */
                } else if (d->edit_mode == 7) { /* place Orion */
                }
            }
            for (int i = 2; i < EDIT_MODES; ++i) {
                if (d->edit_mode >=2 && oi == oi_edit[i]) {
                    d->edit_mode = i;
                }
            }
            for (int i = 0; i < NEWOPTS; ++i) {
                if (oi == oi_noption[i]) {
                    nopt[i] += 1;
                    if (nopt[i] >= newopt_descr[i].opts) nopt[i] = 0;
                }
                if (oi2 == oi_option[i]) {
                    d->descr = newopt_descr[i].descr;
                    break;
                }
            }
        }
        new_game_draw_extra_cb(d);
        uiobj_finish_frame();
        if (d->fadein) {
            d->fadein = false;
            ui_palette_fadein_4b_19_1();
        }
        ui_delay_ticks_or_click(2);
    }

#undef MAKE_UIOBJS
    return flag_ok;
}

#undef PORTRAITBOX_H
#undef PORTRAITBOX_TOP_MARGIN

/* -------------------------------------------------------------------------- */

bool ui_new_game(struct game_s *g, struct game_new_options_s *newopts)
{
    struct new_game_data_s d;
    bool flag_done = false, flag_fadein = false, flag_ok = false;
    uint16_t oppon = 0, gsize = 0, diffic = 0;
    int16_t oi_gsize, oi_diffic, oi_oppon, oi_cancel, oi_ok;
    int16_t oi_esc, oi_d, oi_g, oi_o, oi_space;

    d.g = g;
    d.newopts = newopts;

    gsize = newopts->galaxy_size;
    diffic = newopts->difficulty;
    oppon = newopts->players - 1/*0-based*/ - 1/*player*/;
    if (!newopts->galaxy_seed) newopts->galaxy_seed = rnd_get_new_seed();

    ui_palette_fadeout_19_19_1();
    lbxpal_select(3, -1, 0);
    lbxpal_build_colortables();

    new_game_load_data(&d);

#define MAKE_UIOBJS() \
    do { \
        lbxfont_select(0, 0, 0, 0); \
        uiobj_table_clear(); \
        oi_esc = uiobj_add_inputkey(MOO_KEY_ESCAPE); \
        oi_gsize = uiobj_add_t0(0xaf, 0x1d, game_str_tbl_gsize[gsize], d.gfx_optb_ng, MOO_KEY_UNKNOWN); \
        oi_diffic = uiobj_add_t0(0xaf, 0x44, game_str_tbl_diffic[diffic], d.gfx_optb_ng, MOO_KEY_UNKNOWN); \
        oi_oppon = uiobj_add_t0(0xaf, 0x6b, game_str_tbl_oppon[oppon], d.gfx_optb_ng, MOO_KEY_UNKNOWN); \
        oi_cancel = uiobj_add_t0(0x5a, 0x93, "", d.gfx_optb_cancel, MOO_KEY_UNKNOWN); \
        oi_ok = uiobj_add_t0(0xa1, 0x93, "", d.gfx_optb_ok, MOO_KEY_UNKNOWN); \
        oi_d = uiobj_add_inputkey(MOO_KEY_d); \
        oi_g = uiobj_add_inputkey(MOO_KEY_g); \
        oi_o = uiobj_add_inputkey(MOO_KEY_o); \
        oi_space = uiobj_add_inputkey(MOO_KEY_SPACE); \
    } while (0)

    MAKE_UIOBJS();

    uiobj_set_callback_and_delay(new_game_draw_cb1, &d, 2);
    uiobj_set_xyoff(1, 1);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi != UIOBJI_ESC) && (oi != UIOBJI_NONE)) {
            ui_sound_play_sfx_24();
        }
        if ((oi == UIOBJI_ESC) || (oi == oi_cancel)) {
            ui_sound_play_sfx_06();
        }
        if ((oi == oi_diffic) || (oi == oi_d)) {
            if (++diffic >= 5) { diffic = 0; }
        }
        if ((oi == oi_gsize) || (oi == oi_g)) {
            if (++gsize >= 4) { gsize = 0; }
        }
        if ((oi == oi_oppon) || (oi == oi_o)) {
            if (++oppon >= 5) { oppon = 0; }
        }
        if ((oi == UIOBJI_ESC) || (oi == oi_cancel) || (oi == oi_esc)) {
            flag_ok = false;
            flag_done = true;
        }
        if ((oi == oi_ok) || (oi == oi_space)) {
            flag_ok = true;
            flag_done = true;
        }
        new_game_draw_cb1(&d);

        MAKE_UIOBJS();

        uiobj_finish_frame();

        if (!flag_fadein) {
            ui_palette_fadein_4b_19_1();
            flag_fadein = true;
        }
        ui_delay_ticks_or_click(2);
    }

#undef MAKE_UIOBJS

    newopts->galaxy_size = gsize;
    newopts->difficulty = diffic;
    newopts->players = oppon + 1/*0-based*/ + 1/*player*/;

    uiobj_unset_callback();

    if (flag_ok) {
        if (ui_extra_enabled) {
            flag_ok = ui_new_game_extra(&d);
        } else {
            flag_ok = ui_new_game_racebannernames(newopts, &d);
        }
    }

    uiobj_unset_callback();
    new_game_free_data(&d);

    ui_palette_fadeout_19_19_1();
    ui_draw_erase_buf();
    hw_video_draw_buf();
    ui_draw_erase_buf();
    hw_video_draw_buf();

    return flag_ok;
}
