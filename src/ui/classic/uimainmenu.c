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

static bool main_menu_have_save_continue(void *vptr) {
    return game_save_tbl_have_save[GAME_SAVE_I_CONTINUE];
}

static bool main_menu_have_save_any(void *vptr) {
    for (int i = 0; i < NUM_SAVES; ++i) {
        if (game_save_tbl_have_save[i]) {
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------- */

#define MM_ITEMS_PER_PAGE 5

typedef enum {
    MAIN_MENU_ITEM_GAME_CONTINUE,
    MAIN_MENU_ITEM_GAME_LOAD,
    MAIN_MENU_ITEM_GAME_NEW,
    MAIN_MENU_ITEM_GAME_CUSTOM,
    MAIN_MENU_ITEM_QUIT,
    MAIN_MENU_ITEM_NUM,
} main_menu_item_id_t;

typedef enum {
    MAIN_MENU_PAGE_MAIN,
    MAIN_MENU_PAGE_NUM,
} main_menu_page_id_t;

typedef enum {
    MAIN_MENU_ITEM_TYPE_NONE,
    MAIN_MENU_ITEM_TYPE_RETURN,
} main_menu_item_type_t;

struct main_menu_item_s {
    const char *text;
    bool (*is_active) (void *);
    mookey_t key;
    main_menu_item_type_t type;
    main_menu_action_t ret_action;
};

struct main_menu_page_s {
    main_menu_item_id_t item_id[MM_ITEMS_PER_PAGE];
};

static struct main_menu_item_s mm_items[MAIN_MENU_ITEM_NUM] = {
    {
        "Continue",
        main_menu_have_save_continue,
        MOO_KEY_c,
        MAIN_MENU_ITEM_TYPE_RETURN,
        MAIN_MENU_ACT_CONTINUE_GAME,
    },
    {
        "Load Game",
        main_menu_have_save_any,
        MOO_KEY_l,
        MAIN_MENU_ITEM_TYPE_RETURN,
        MAIN_MENU_ACT_LOAD_GAME
    },
    {
        "New Game",
        NULL,
        MOO_KEY_n,
        MAIN_MENU_ITEM_TYPE_RETURN,
        MAIN_MENU_ACT_NEW_GAME,
    },
    {
        "Custom Game",
        NULL,
        MOO_KEY_g,
        MAIN_MENU_ITEM_TYPE_RETURN,
        MAIN_MENU_ACT_CUSTOM_GAME,
    },
    {
        "Quit to OS",
        NULL,
        MOO_KEY_q,
        MAIN_MENU_ITEM_TYPE_RETURN,
        MAIN_MENU_ACT_QUIT_GAME
    },
};

static struct main_menu_page_s mm_pages[MAIN_MENU_PAGE_NUM] = {
    {
        {
            MAIN_MENU_ITEM_GAME_CONTINUE,
            MAIN_MENU_ITEM_GAME_LOAD,
            MAIN_MENU_ITEM_GAME_NEW,
            MAIN_MENU_ITEM_GAME_CUSTOM,
            MAIN_MENU_ITEM_QUIT,
        },
    },
};

struct main_menu_data_s {
    const struct main_menu_item_s *items[MM_ITEMS_PER_PAGE];
    int16_t item_oi[MM_ITEMS_PER_PAGE];
    int16_t oi_quit;
    bool active[MM_ITEMS_PER_PAGE];
    int frame;
    main_menu_action_t ret;
    bool flag_done;
    int clicked_i;
    int highlight;
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

static void main_menu_load_page(struct main_menu_data_s *d, main_menu_page_id_t page_i)
{
    if (page_i < 0 || page_i >= MAIN_MENU_PAGE_NUM) {
        return;
    }
    lbxgfx_set_frame_0(d->gfx_vortex);
    uiobj_table_clear();
    d->oi_quit = uiobj_add_alt_str("q");
    for (int i = 0; i < MM_ITEMS_PER_PAGE; ++i) {
        d->item_oi[i] = UIOBJI_INVALID;
        if (mm_pages[page_i].item_id[i] < 0 || mm_pages[page_i].item_id[i] >= MAIN_MENU_ITEM_NUM) {
            d->items[i] = NULL;
            continue;
        }
        d->items[i] = &mm_items[mm_pages[page_i].item_id[i]];
        if (!d->items[i]->text) {
            d->items[i] = NULL;
            continue;
        }
        d->active[i] = d->items[i]->is_active ? d->items[i]->is_active(&d) : true;
        if (d->active[i]) {
            int y = 0x79 + 0xd * i;
            d->item_oi[i] = uiobj_add_mousearea(0x3c, y, 0x104, y + 0xd, d->items[i]->key);
        }
    }
}

static int main_menu_get_item(struct main_menu_data_s *d, int16_t oi)
{
    for (int i = 0; i < MM_ITEMS_PER_PAGE; ++i) {
        if (oi == d->item_oi[i] && d->items[i] != NULL) {
            return i;
        }
    }
    return -1;
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
    for (int i = 0; i < MM_ITEMS_PER_PAGE; ++i) {
        if (!d->items[i] || !d->items[i]->text) {
            continue;
        }
        int y = 0x79 + 0xe * i;
        if (d->active[i]) {
            lbxfont_select(4, (d->highlight == i) ? 3 : 2, 0, 0);
        } else {
            lbxfont_select(4, 7, 0, 0);
        }
        lbxfont_print_str_center(0xa0, y, d->items[i]->text, UI_SCREEN_W, ui_scale);
    }
    d->frame = (d->frame + 1) % 0x30;
}

static main_menu_action_t main_menu_do(struct main_menu_data_s *d)
{
    int16_t oi_tutor, oi_extra;
    bool flag_fadein = false;

    d->frame = 0;
    d->flag_done = false;
    d->ret = -1;
    if (ui_draw_finish_mode != 0) {
        ui_palette_fadeout_19_19_1();
    }
    lbxpal_select(1, -1, 0);
    ui_cursor_setup_area(1, &ui_cursor_area_all_i1);
    uiobj_table_clear();

    /* HACK these fix the gfx mess after canceling a load */
    ui_draw_erase_buf();
    uiobj_finish_frame();

    main_menu_load_page(d, MAIN_MENU_PAGE_MAIN);

    oi_tutor = uiobj_add_alt_str("tutor");
    oi_extra = uiobj_add_alt_str("x");
    d->fix_version = false;

    d->highlight = main_menu_get_item(d, uiobj_at_cursor());
    uiobj_set_callback_and_delay(main_menu_draw_cb, d, 2);
    while (!d->flag_done) {
        int16_t oi1, oi2;
        ui_delay_prepare();
        oi1 = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        d->highlight = main_menu_get_item(d, oi2);
        main_menu_draw_cb(d);
        d->clicked_i = main_menu_get_item(d, oi1);
        if (oi1 == d->oi_quit) {
            d->ret = MAIN_MENU_ACT_QUIT_GAME;
            d->flag_done = true;
        } else if (d->clicked_i != -1) {
            ui_sound_play_sfx_24();
            const struct main_menu_item_s *it = d->items[d->clicked_i];
            if (it->type == MAIN_MENU_ITEM_TYPE_RETURN) {
                d->ret = it->ret_action;
                d->flag_done = true;
            }
        }

        if (oi1 == oi_tutor) {
            ui_sound_play_sfx_24();
            d->ret = MAIN_MENU_ACT_TUTOR;
            d->flag_done = true;
        } else if (oi1 == oi_extra) {
            ui_extra_enabled = !ui_extra_enabled;
            d->fix_version = true;
        }
        uiobj_finish_frame();
        if ((ui_draw_finish_mode != 0) && !flag_fadein) {
            ui_palette_fadein_4b_19_1();
            flag_fadein = true;
        }
        ui_delay_ticks_or_click(2);
    }
    uiobj_unset_callback();
    return d->ret;
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
            case MAIN_MENU_ACT_CUSTOM_GAME:
                flag_done = ui_new_game(newopts, (ret == MAIN_MENU_ACT_CUSTOM_GAME));
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
