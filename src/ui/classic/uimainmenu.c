#include "config.h"

#include "ui.h"
#include "game.h"
#include "game_save.h"
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
#include "uiload.h"
#include "uinewgame.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "version.h"

/* -------------------------------------------------------------------------- */

struct main_menu_data_s {
    int frame;
    int selected;
    bool have_continue;
    bool have_loadgame;
    bool fix_version;
    uint8_t *gfx_vortex;
    uint8_t *gfx_title;
};

static void load_mainmenu_data(struct main_menu_data_s *d)
{
    d->gfx_vortex = lbxfile_item_get(LBXFILE_VORTEX, 0);
    d->gfx_title = lbxfile_item_get(LBXFILE_V11, 0);
}

static void free_mainmenu_data(struct main_menu_data_s *d)
{
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_vortex);
    lbxfile_item_release(LBXFILE_V11, d->gfx_title);
}

static void main_menu_draw_cb(void *vptr)
{
    struct main_menu_data_s *d = vptr;
    ui_draw_erase_buf();
    ui_draw_copy_buf();
    if (d->fix_version) {
        d->fix_version = false;
        lbxgfx_set_frame_0(d->gfx_vortex);
    }
    lbxgfx_draw_frame(0, 0, d->gfx_vortex, UI_SCREEN_W, ui_scale);
    if (!ui_extra_enabled) {
        lbxgfx_draw_frame(0, 0, d->gfx_title, UI_SCREEN_W, ui_scale);
    } else {
        lbxgfx_draw_frame_offs(0, 0, d->gfx_title, 0, 0, UI_VGA_W - 1, 191, UI_SCREEN_W, ui_scale);
        lbxfont_select(2, 7, 0, 0);
        lbxfont_print_str_center(160, 193, "PROGRAM VERSION " PACKAGE_NAME " " VERSION_STR, UI_SCREEN_W, ui_scale);
    }
    if (d->have_continue) {
        lbxfont_select(4, (d->selected == MAIN_MENU_ACT_CONTINUE_GAME) ? 3 : 2, 0, 0);
    } else {
        lbxfont_select(4, 7, 0, 0);
    }
    lbxfont_print_str_center(0xa0, 0x7f, game_str_mm_continue, UI_SCREEN_W, ui_scale);
    if (d->have_loadgame) {
        lbxfont_select(4, (d->selected == MAIN_MENU_ACT_LOAD_GAME) ? 3 : 2, 0, 0);
    } else {
        lbxfont_select(4, 7, 0, 0);
    }
    lbxfont_print_str_center(0xa0, 0x8f, game_str_mm_load, UI_SCREEN_W, ui_scale);
    lbxfont_select(4, (d->selected == MAIN_MENU_ACT_NEW_GAME) ? 3 : 2, 0, 0);
    lbxfont_print_str_center(0xa0, 0x9f, game_str_mm_new, UI_SCREEN_W, ui_scale);
    lbxfont_select(4, (d->selected == MAIN_MENU_ACT_QUIT_GAME) ? 3 : 2, 0, 0);
    lbxfont_print_str_center(0xa0, 0xaf, game_str_mm_quit, UI_SCREEN_W, ui_scale);
    d->frame = (d->frame + 1) % 0x30;
}

static main_menu_action_t main_menu_do(struct main_menu_data_s *d)
{
    int16_t oi_n, oi_l, oi_c, oi_q;
    int16_t oi_newgame, oi_continue, oi_loadgame, oi_quit;
    int16_t oi_tutor, oi_extra, cursor_at;
    bool flag_done = false, flag_fadein = false;

    d->frame = 0;
    if (ui_draw_finish_mode != 0) {
        ui_palette_fadeout_19_19_1();
    }
    lbxpal_select(1, -1, 0);
    ui_cursor_setup_area(1, &ui_cursor_area_all_i1);
    lbxgfx_set_frame_0(d->gfx_vortex);
    uiobj_table_clear();

    /* HACK these fix the gfx mess after canceling a load */
    ui_draw_erase_buf();
    uiobj_finish_frame();

    oi_tutor = uiobj_add_alt_str("tutor");
    oi_extra = uiobj_add_alt_str("x");
    d->have_continue = game_save_tbl_have_save[GAME_SAVE_I_CONTINUE];
    if (d->have_continue) {
        oi_continue = uiobj_add_mousearea(0x3c, 0x7f, 0x104, 0x8e, MOO_KEY_UNKNOWN);
    } else {
        oi_continue = UIOBJI_INVALID;
    }
    oi_loadgame = UIOBJI_INVALID;
    d->have_loadgame = false;
    for (int i = 0; i < NUM_SAVES; ++i) {
        if (game_save_tbl_have_save[i]) {
            oi_loadgame = uiobj_add_mousearea(0x3c, 0x8f, 0x104, 0x9e, MOO_KEY_UNKNOWN);
            d->have_loadgame = true;
            break;
        }
    }
    oi_newgame = uiobj_add_mousearea(0x3c, 0x9f, 0x104, 0xae, MOO_KEY_UNKNOWN);
    oi_quit = uiobj_add_mousearea(0x3c, 0xaf, 0x104, 0xbe, MOO_KEY_UNKNOWN);
    if (d->have_continue) {
        oi_c = uiobj_add_inputkey(MOO_KEY_c);
    } else {
        oi_c = UIOBJI_INVALID;
    }
    if (d->have_loadgame) {
        oi_l = uiobj_add_inputkey(MOO_KEY_l);
    } else {
        oi_l = UIOBJI_INVALID;
    }
    oi_n = uiobj_add_inputkey(MOO_KEY_n);
    oi_q = uiobj_add_inputkey(MOO_KEY_q);
    uiobj_set_focus(oi_newgame);
    d->selected = -1;
    d->fix_version = false;
    cursor_at = abs(uiobj_at_cursor());
    if (cursor_at == oi_continue) {
        d->selected = MAIN_MENU_ACT_CONTINUE_GAME;
    } else if (cursor_at == oi_newgame) {
        d->selected = MAIN_MENU_ACT_NEW_GAME;
    } else if (cursor_at == oi_loadgame) {
        d->selected = MAIN_MENU_ACT_LOAD_GAME;
    } else if (cursor_at == oi_quit) {
        d->selected = MAIN_MENU_ACT_QUIT_GAME;
    }
    uiobj_set_callback_and_delay(main_menu_draw_cb, d, 2);
    while (!flag_done) {
        int16_t oi1, oi2;
        ui_delay_prepare();
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        if (oi1 != 0) {
            flag_done = true;
        }
        main_menu_draw_cb(d);
        d->selected = -1;
        if (oi2 == oi_continue) {
            d->selected = MAIN_MENU_ACT_CONTINUE_GAME;
        } else if (oi2 == oi_loadgame) {
            d->selected = MAIN_MENU_ACT_LOAD_GAME;
        } else if (oi2 == oi_newgame) {
            d->selected = MAIN_MENU_ACT_NEW_GAME;
        } else if (oi2 == oi_quit) {
            d->selected = MAIN_MENU_ACT_QUIT_GAME;
        }
        if (oi1 == oi_c) {
            d->selected = MAIN_MENU_ACT_CONTINUE_GAME;
        } else if (oi1 == oi_l) {
            d->selected = MAIN_MENU_ACT_LOAD_GAME;
        } else if (oi1 == oi_n) {
            d->selected = MAIN_MENU_ACT_NEW_GAME;
        } else if (oi1 == oi_q) {
            d->selected = MAIN_MENU_ACT_QUIT_GAME;
        } else if (oi1 == oi_tutor) {
            d->selected = MAIN_MENU_ACT_TUTOR;
        } else if (oi1 == oi_extra) {
            ui_extra_enabled = !ui_extra_enabled;
            d->fix_version = true;
            flag_done = false;
        }
        if (flag_done) {
            ui_sound_play_sfx_24();
        }
        if (d->selected == -1) {
            flag_done = false;
        }
        uiobj_finish_frame();
        if ((ui_draw_finish_mode != 0) && !flag_fadein) {
            ui_palette_fadein_4b_19_1();
            flag_fadein = true;
        }
        ui_delay_ticks_or_click(2);
    }
    uiobj_unset_callback();
    return d->selected;
}

/* -------------------------------------------------------------------------- */

main_menu_action_t ui_main_menu(struct game_new_options_s *newopts, int *load_game_i_ptr)
{
    struct main_menu_data_s d;
    bool flag_done = false;
    main_menu_action_t ret = MAIN_MENU_ACT_QUIT_GAME;
    uiobj_set_help_id(-1);
    load_mainmenu_data(&d);
    ui_sound_play_music(1);
    ui_draw_finish_mode = 1;
    ui_cursor_setup_area(1, &ui_cursor_area_all_i0);
    while (!flag_done) {
        ret = main_menu_do(&d);
        ui_draw_finish_mode = 0;
        switch (ret) {
            case MAIN_MENU_ACT_NEW_GAME:
                flag_done = ui_new_game(newopts);
                ui_draw_finish_mode = 1;
                break;
            case MAIN_MENU_ACT_LOAD_GAME:
                {
                    int i;
                    i = ui_load_game();
                    if (i >= 0) {
                        *load_game_i_ptr = i;
                        flag_done = true;
                    }
                    ui_draw_finish_mode = 1;
                }
                break;
            case MAIN_MENU_ACT_QUIT_GAME:
                hw_audio_music_fadeout();
                ui_palette_fadeout_a_f_1();
                flag_done = true;
                break;
            default:
                flag_done = true;
                break;
        }
    }
    ui_sound_stop_music();
    free_mainmenu_data(&d);
    return ret;
}
