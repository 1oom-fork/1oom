#include "config.h"

#include <string.h>

#include "ui.h"
#include "game.h"
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

/* -------------------------------------------------------------------------- */

static const bool tbllistallow[] = { true, true, true, true, true, true, true, true, true, true, true, true };

/* -------------------------------------------------------------------------- */

struct new_game_data_s {
    int frame;
    int16_t selected;
    int16_t race;
    int16_t banner;
    const char *str_title;
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
    lbxgfx_draw_frame(0, 0, d->gfx_newgame, UI_SCREEN_W);
}

static void new_game_draw_race_cb(void *vptr)
{
    struct new_game_data_s *d = vptr;
    hw_video_copy_back_from_page2();
    if (d->selected < RACE_NUM) {
        lbxgfx_draw_frame(91, 11, d->gfx_portrait[d->selected], UI_SCREEN_W);
        lbxfont_select(0, 4, 0, 0);
        for (int i = 0; i < 3; ++i) {
            if (*game_str_tbl_traits[d->selected * 3 + i]) {
                lbxfont_print_str_center(0x6f, 0x35 + i * 8, game_str_tbl_traits[d->selected * 3 + i], UI_SCREEN_W);
            }
        }
    }
}

static void new_game_draw_banner_cb(void *vptr)
{
    struct new_game_data_s *d = vptr;
    hw_video_copy_back_from_page2();
    if (d->str_title) {
        lbxfont_select(5, 0, 0, 0);
        lbxfont_print_str_normal(0xa, 0xa, d->str_title, UI_SCREEN_W);
        lbxgfx_apply_colortable(0xa, 0x14, 0x52, 0x1f, 2, UI_SCREEN_W);
    }
    if (d->race < RACE_NUM) {
        lbxgfx_draw_frame(91, 11, d->gfx_portrait[d->race], UI_SCREEN_W);
    }
    ui_draw_filled_rect(0x5a, 0x35, 0x83, 0x5a, 0);
    ui_draw_box1(0x5a, 0x35, 0x83, 0x5a, 0x9b, 0x9b);
    if (d->selected < BANNER_NUM) {
        lbxgfx_set_new_frame(d->gfx_flag[d->selected], d->frame);
        gfx_aux_draw_frame_to(d->gfx_flag[d->selected], &ui_data.aux.screen);
        gfx_aux_draw_frame_from(0x5b, 0x38, &ui_data.aux.screen, UI_SCREEN_W);
    }
    if (++d->frame == 0xa) {
        d->frame = 0;
    }
}

static bool ui_new_game_names(struct game_new_options_s *newopts, struct new_game_data_s *d)
{
    const uint8_t tbl_cursor_color[] = { 0x91, 0x91, 0x8d, 0x8d, 0x89, 0x89, 0x84, 0x84, 0x7f, 0x7f };
    char buf[32];
    bool flag_ok;

    d->str_title = game_str_ng_your_name;
    uiobj_table_clear();
    flag_ok = false;
    while (!flag_ok) {
        lbxfont_select(5, 0xf, 0xf, 0xf);
        game_new_generate_emperor_name(d->race, buf);
        if (!uiobj_read_str(0xf, 0x16, 0x41, buf, 0xb/*len*/, 0, 0, 0, tbl_cursor_color)) {
            return false;
        }
        util_trim_whitespace(buf);
        flag_ok = buf[0] != '\0';
    }
    strcpy(newopts->pdata[PLAYER_0].playername, buf);
    ui_sound_play_sfx_24();

    d->str_title = game_str_ng_home_name;
    uiobj_table_clear();
    flag_ok = false;
    while (!flag_ok) {
        lbxfont_select(5, 0xf, 0xf, 0xf);
        game_new_generate_home_name(d->race, buf);
        if (!uiobj_read_str(0xf, 0x16, 0x32, buf, PLANET_NAME_LEN, 0, 0, 0, tbl_cursor_color)) {
            return false;
        }
        util_trim_whitespace(buf);
        flag_ok = buf[0] != '\0';
    }
    strcpy(newopts->pdata[PLAYER_0].homename, buf);
    ui_sound_play_sfx_24();

    lbxfont_select(5, 1, 0xf, 0xf);
    /* orion.exe generates other emperor names here */
    return true;
}

static bool ui_new_game_racebannernames(struct game_new_options_s *newopts, struct new_game_data_s *d)
{
    int16_t race = 0, banner = 0;
    d->str_title = 0;
    /* orion.exe inits game->hmm02 and game_new:researchflag here for whatever reason */
    uiobj_table_clear();
    ui_palette_fadeout_19_19_1();
    lbxpal_select(4, -1, 0);
    lbxpal_build_colortables();
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, d->gfx_custom, UI_SCREEN_W);
    ui_draw_box1(0x5a, 0xa, 0x83, 0x2d, 0x9b, 0x9b);
    hw_video_copy_back_to_page2();
    d->selected = 0;

    /* race */
    new_game_draw_race_cb(d);
    uiobj_finish_frame();
    ui_palette_fadein_4b_19_1();
    uiobj_set_callback_and_delay(new_game_draw_race_cb, d, 2);
    uiobj_table_clear();
    lbxfont_select(5, 0xf, 0, 0);
    race = uiobj_select_from_list1(0xa, 0xa, 0x32, game_str_ng_choose_race, (char const * const *)d->str_tbl_2space_race, &d->selected, tbllistallow, 0xf, 0, 2, 0, 0, 0);
    if (race == -1) {
        ui_sound_play_sfx_06();
        return false;
    }
    ui_sound_play_sfx_24();
    newopts->pdata[PLAYER_0].race = race;
    d->race = race;

    /* banner */
    d->selected = 0;
    d->frame = 0;
    uiobj_table_clear();
    uiobj_set_callback_and_delay(new_game_draw_banner_cb, d, 2);
    lbxfont_select(5, 0xf, 0, 0);
    banner = uiobj_select_from_list1(0xa, 0xa, 0x32, game_str_ng_choose_banner, (char const * const *)d->str_tbl_2space_banner, &d->selected, tbllistallow, 0xf, 0, 2, 0, 0, 0);
    if (banner == -1) {
        ui_sound_play_sfx_06();
        return false;
    }
    ui_sound_play_sfx_24();
    newopts->pdata[PLAYER_0].banner = banner;
    d->banner = banner;

    /* name & home */
    return ui_new_game_names(newopts, d);
}

/* -------------------------------------------------------------------------- */

bool ui_new_game(struct game_new_options_s *newopts)
{
    struct new_game_data_s d;
    bool flag_done = false, flag_fadein = false, flag_ok = false;
    uint16_t oppon = 0, gsize = 0, diffic = 0;
    int16_t oi_gsize, oi_diffic, oi_oppon, oi_cancel, oi_ok;
    int16_t oi_esc, oi_d, oi_g, oi_o, oi_space;

    gsize = newopts->galaxy_size;
    diffic = newopts->difficulty;
    oppon = newopts->players - 1/*0-based*/ - 1/*player*/;

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
            flag_done = 1;
        }
        if ((oi == oi_ok) || (oi == oi_space)) {
            flag_ok = true;
            flag_done = 1;
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
        flag_ok = ui_new_game_racebannernames(newopts, &d);
    }

    uiobj_unset_callback();
    new_game_free_data(&d);

    return flag_ok;
}
