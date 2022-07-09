#include "config.h"

#include "ui.h"
#include "comp.h"
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

static int mm_ui_scale_helper;

static bool main_menu_game_active(void *vptr) {
    return ui_scale == mm_ui_scale_helper;
}

static bool main_menu_continue_active(void *vptr) {
    return main_menu_have_save_continue(vptr) && main_menu_game_active(vptr);
}

static bool main_menu_load_active(void *vptr) {
    return main_menu_have_save_any(vptr) && main_menu_game_active(vptr);
}

static void main_menu_toggle_fullscreen(void *vptr) {
    hw_video_toggle_fullscreen();
}

static const char* mm_hw_aspect_helper;

static void main_menu_next_aspect(void *vptr) {
    hw_uiopt_cb_aspect_next();
    mm_hw_aspect_helper = hw_uiopt_cb_aspect_get();
    hw_video_update_aspect();
}

static void main_menu_toggle_music(void *vptr) {
    if (opt_music_enabled) {
        ui_sound_stop_music();
        opt_music_enabled = false;
    } else {
        opt_music_enabled = true;
        ui_sound_play_music(1);
    }
}

static void main_menu_update_music_volume(void *vptr) {
    ui_sound_music_volume(opt_music_volume);
}

static void main_menu_update_sfx_volume(void *vptr) {
    ui_sound_sfx_volume(opt_sfx_volume);
}

/* -------------------------------------------------------------------------- */

#define MM_ITEMS_PER_PAGE 5
#define MM_PAGE_STACK_SIZE 4

typedef enum {
    MAIN_MENU_ITEM_GAME_CONTINUE,
    MAIN_MENU_ITEM_GAME_LOAD,
    MAIN_MENU_ITEM_GAME_NEW,
    MAIN_MENU_ITEM_GAME_CUSTOM,
    MAIN_MENU_ITEM_QUIT,
    MAIN_MENU_ITEM_PLAY,
    MAIN_MENU_ITEM_TUTOR,
    MAIN_MENU_ITEM_INTERFACE,
    MAIN_MENU_ITEM_UIEXTRA,
    MAIN_MENU_ITEM_MODERN_CONTROLS,
    MAIN_MENU_ITEM_COMBAT_AUTORESOLVE,
    MAIN_MENU_ITEM_SOUND,
    MAIN_MENU_ITEM_SOUND_MUSIC,
    MAIN_MENU_ITEM_SOUND_SFX,
    MAIN_MENU_ITEM_SOUND_MUSIC_VOLUME,
    MAIN_MENU_ITEM_SOUND_SFX_VOLUME,
    MAIN_MENU_ITEM_SCROLL_SPEED,
    MAIN_MENU_ITEM_OPTIONS,
    MAIN_MENU_ITEM_MOUSE,
    MAIN_MENU_ITEM_MOUSE_INVERT_SLIDER,
    MAIN_MENU_ITEM_MOUSE_INVERT_COUNTER,
    MAIN_MENU_ITEM_UI_SCALE,
    MAIN_MENU_ITEM_VIDEO,
    MAIN_MENU_ITEM_VIDEO_FULLSCREEN,
    MAIN_MENU_ITEM_VIDEO_ASPECT,
    MAIN_MENU_ITEM_BOMB_ANIMATION,
    MAIN_MENU_ITEM_SKIP_INTRO,
    MAIN_MENU_ITEM_BACK,
    MAIN_MENU_ITEM_NUM,
} main_menu_item_id_t;

typedef enum {
    MAIN_MENU_PAGE_MAIN,
    MAIN_MENU_PAGE_GAME,
    MAIN_MENU_PAGE_INTERFACE,
    MAIN_MENU_PAGE_SOUND,
    MAIN_MENU_PAGE_OPTIONS,
    MAIN_MENU_PAGE_MOUSE,
    MAIN_MENU_PAGE_VIDEO,
    MAIN_MENU_PAGE_NUM,
} main_menu_page_id_t;

typedef enum {
    MAIN_MENU_ITEM_TYPE_NONE,
    MAIN_MENU_ITEM_TYPE_RETURN,
    MAIN_MENU_ITEM_TYPE_PAGE,
    MAIN_MENU_ITEM_TYPE_PAGE_BACK,
    MAIN_MENU_ITEM_TYPE_FUNCTION,
    MAIN_MENU_ITEM_TYPE_BOOL,
    MAIN_MENU_ITEM_TYPE_INT,
    MAIN_MENU_ITEM_TYPE_STR,
} main_menu_item_type_t;

struct main_menu_item_s {
    const char *text;
    void *value;
    bool (*is_active) (void *);
    void (*func) (void *);
    int value_min;
    int value_max;
    mookey_t key;
    main_menu_item_type_t type;
    main_menu_page_id_t page;
    main_menu_action_t ret_action;
};

struct main_menu_page_s {
    main_menu_item_id_t item_id[MM_ITEMS_PER_PAGE];
};

static struct main_menu_item_s mm_items[MAIN_MENU_ITEM_NUM] = {
    {
        "Continue",
        NULL,
        main_menu_continue_active,
        NULL,
        0,
        0,
        MOO_KEY_c,
        MAIN_MENU_ITEM_TYPE_RETURN,
        -1,
        MAIN_MENU_ACT_CONTINUE_GAME,
    },
    {
        "Load Game",
        NULL,
        main_menu_load_active,
        NULL,
        0,
        0,
        MOO_KEY_l,
        MAIN_MENU_ITEM_TYPE_RETURN,
        -1,
        MAIN_MENU_ACT_LOAD_GAME
    },
    {
        "New Game",
        NULL,
        main_menu_game_active,
        NULL,
        0,
        0,
        MOO_KEY_n,
        MAIN_MENU_ITEM_TYPE_RETURN,
        -1,
        MAIN_MENU_ACT_NEW_GAME,
    },
    {
        "Custom Game",
        NULL,
        main_menu_game_active,
        NULL,
        0,
        0,
        MOO_KEY_g,
        MAIN_MENU_ITEM_TYPE_RETURN,
        -1,
        MAIN_MENU_ACT_CUSTOM_GAME,
    },
    {
        "Quit to OS",
        NULL,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_q,
        MAIN_MENU_ITEM_TYPE_RETURN,
        -1,
        MAIN_MENU_ACT_QUIT_GAME
    },
    {
        "Game",
        NULL,
        main_menu_game_active,
        NULL,
        0,
        0,
        MOO_KEY_g,
        MAIN_MENU_ITEM_TYPE_PAGE,
        MAIN_MENU_PAGE_GAME,
        -1,
    },
    {
        "Tutorial",
        NULL,
        main_menu_game_active,
        NULL,
        0,
        0,
        MOO_KEY_t,
        MAIN_MENU_ITEM_TYPE_RETURN,
        -1,
        MAIN_MENU_ACT_TUTOR,
    },
    {
        "Interface",
        NULL,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_i,
        MAIN_MENU_ITEM_TYPE_PAGE,
        MAIN_MENU_PAGE_INTERFACE,
        -1,
    },
    {
        "UI Extra",
        &ui_extra_enabled,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_x,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "Modern Controls",
        &ui_modern_starmap_controls,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_m,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "Combat Autoresolve",
        &ui_space_combat_autoresolve,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_c,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "Sound",
        NULL,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_s,
        MAIN_MENU_ITEM_TYPE_PAGE,
        MAIN_MENU_PAGE_SOUND,
        -1,
    },
    {
        "Music",
        &opt_music_enabled,
        NULL,
        main_menu_toggle_music,
        0,
        0,
        MOO_KEY_m,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "SFX",
        &opt_sfx_enabled,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_s,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "Music Volume",
        &opt_music_volume,
        NULL,
        main_menu_update_music_volume,
        0,
        128,
        MOO_KEY_UNKNOWN,
        MAIN_MENU_ITEM_TYPE_INT,
        -1,
        -1,
    },
    {
        "SFX Volume",
        &opt_sfx_volume,
        NULL,
        main_menu_update_sfx_volume,
        0,
        128,
        MOO_KEY_UNKNOWN,
        MAIN_MENU_ITEM_TYPE_INT,
        -1,
        -1,
    },
    {
        "Scroll speed",
        &ui_sm_scroll_speed,
        NULL,
        NULL,
        0,
        UI_SCROLL_SPEED_MAX,
        MOO_KEY_UNKNOWN,
        MAIN_MENU_ITEM_TYPE_INT,
        -1,
        -1,
    },
    {
        "Options",
        NULL,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_o,
        MAIN_MENU_ITEM_TYPE_PAGE,
        MAIN_MENU_PAGE_OPTIONS,
        -1,
    },
    {
        "Mouse",
        NULL,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_m,
        MAIN_MENU_ITEM_TYPE_PAGE,
        MAIN_MENU_PAGE_MOUSE,
        -1,
    },
    {
        "Invert Slider",
        &ui_mwi_slider,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_s,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "Invert Counter",
        &ui_mwi_counter,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_c,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "UI Scale",
        &mm_ui_scale_helper,
        NULL,
        NULL,
        1,
        UI_SCALE_MAX - 1,
        MOO_KEY_UNKNOWN,
        MAIN_MENU_ITEM_TYPE_INT,
        -1,
        -1,
    },
    {
        "Video",
        NULL,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_v,
        MAIN_MENU_ITEM_TYPE_PAGE,
        MAIN_MENU_PAGE_VIDEO,
        -1,
    },
    {
        "Fullscreen",
        &hw_opt_fullscreen,
        NULL,
        main_menu_toggle_fullscreen,
        0,
        0,
        MOO_KEY_f,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "Aspect Ratio",
        &mm_hw_aspect_helper,
        NULL,
        main_menu_next_aspect,
        0,
        0,
        MOO_KEY_a,
        MAIN_MENU_ITEM_TYPE_STR,
        -1,
        -1,
    },
    {
        "Bomb Animation",
        &game_battle_enable_bomb_animation,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_o,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "Skip Intro",
        &game_opt_skip_intro_always,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_s,
        MAIN_MENU_ITEM_TYPE_BOOL,
        -1,
        -1,
    },
    {
        "Back",
        NULL,
        NULL,
        NULL,
        0,
        0,
        MOO_KEY_b,
        MAIN_MENU_ITEM_TYPE_PAGE_BACK,
        -1,
        -1,
    },
};

static struct main_menu_page_s mm_pages[MAIN_MENU_PAGE_NUM] = {
    {
        {
            MAIN_MENU_ITEM_PLAY,
            MAIN_MENU_ITEM_TUTOR,
            MAIN_MENU_ITEM_OPTIONS,
            -1,
            MAIN_MENU_ITEM_QUIT,
        },
    },
    {
        {
            MAIN_MENU_ITEM_GAME_CONTINUE,
            MAIN_MENU_ITEM_GAME_LOAD,
            MAIN_MENU_ITEM_GAME_NEW,
            MAIN_MENU_ITEM_GAME_CUSTOM,
            MAIN_MENU_ITEM_BACK,
        },
    },
    {
        {
            MAIN_MENU_ITEM_COMBAT_AUTORESOLVE,
            MAIN_MENU_ITEM_MODERN_CONTROLS,
            MAIN_MENU_ITEM_BOMB_ANIMATION,
            MAIN_MENU_ITEM_UIEXTRA,
            MAIN_MENU_ITEM_BACK,
        },
    },
    {
        {
            MAIN_MENU_ITEM_SOUND_MUSIC,
            MAIN_MENU_ITEM_SOUND_MUSIC_VOLUME,
            MAIN_MENU_ITEM_SOUND_SFX,
            MAIN_MENU_ITEM_SOUND_SFX_VOLUME,
            MAIN_MENU_ITEM_BACK,
        },
    },
    {
        {
            MAIN_MENU_ITEM_INTERFACE,
            MAIN_MENU_ITEM_MOUSE,
            MAIN_MENU_ITEM_SOUND,
            MAIN_MENU_ITEM_VIDEO,
            MAIN_MENU_ITEM_BACK,
        },
    },
    {
        {
            MAIN_MENU_ITEM_SCROLL_SPEED,
            MAIN_MENU_ITEM_MOUSE_INVERT_SLIDER,
            MAIN_MENU_ITEM_MOUSE_INVERT_COUNTER,
            -1,
            MAIN_MENU_ITEM_BACK,
        },
    },
    {
        {
            MAIN_MENU_ITEM_VIDEO_FULLSCREEN,
            MAIN_MENU_ITEM_VIDEO_ASPECT,
            MAIN_MENU_ITEM_UI_SCALE,
            MAIN_MENU_ITEM_SKIP_INTRO,
            MAIN_MENU_ITEM_BACK,
        },
    },
};

struct main_menu_data_s {
    const struct main_menu_item_s *items[MM_ITEMS_PER_PAGE];
    main_menu_page_id_t pages_stack[MM_PAGE_STACK_SIZE];
    int pages_stack_i;
    int16_t item_oi[MM_ITEMS_PER_PAGE];
    int16_t oi_quit, oi_plus, oi_minus, oi_equals;
    bool active[MM_ITEMS_PER_PAGE];
    int frame;
    main_menu_action_t ret;
    bool flag_done;
    int clicked_i;
    int highlight;
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

static void main_menu_refresh_screen(struct main_menu_data_s *d) {
    lbxgfx_set_frame_0(d->gfx_vortex);
}

static bool main_menu_load_page(struct main_menu_data_s *d, main_menu_page_id_t page_i)
{
    if (page_i < 0 || page_i >= MAIN_MENU_PAGE_NUM) {
        return false;
    }
    if (d->pages_stack_i + 1 >= MM_PAGE_STACK_SIZE) {
        return false;
    }
    main_menu_refresh_screen(d);
    uiobj_table_clear();
    d->oi_quit = uiobj_add_alt_str("q");
    d->oi_plus = uiobj_add_inputkey(MOO_KEY_PLUS);
    d->oi_minus = uiobj_add_inputkey(MOO_KEY_MINUS);
    d->oi_equals = uiobj_add_inputkey(MOO_KEY_EQUALS);
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
    return true;
}

static void main_menu_push_page(struct main_menu_data_s *d, main_menu_page_id_t page_i) {
    if (main_menu_load_page(d, page_i)) {
        ++d->pages_stack_i;
        d->pages_stack[d->pages_stack_i] = page_i;
    }
}

static void main_menu_pop_page(struct main_menu_data_s *d) {
    if (d->pages_stack_i > 0) {
        --d->pages_stack_i;
        main_menu_load_page(d, d->pages_stack[d->pages_stack_i]);
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
    char buf[64];
    ui_draw_erase_buf();
    ui_draw_copy_buf();
    lbxgfx_draw_frame(0, 0, d->gfx_vortex, UI_SCREEN_W, ui_scale);
    lbxgfx_draw_frame_offs(0, 0, d->gfx_title, 0, 0, UI_VGA_W - 1, 191, UI_SCREEN_W, ui_scale);
    lbxfont_select(2, 7, 0, 0);
    lbxfont_print_str_right(315, 193, PACKAGE_NAME " " VERSION_STR, UI_SCREEN_W, ui_scale);
    if (!main_menu_game_active(vptr)) {
        lbxfont_select(2, 2, 0, 0);
        lbxfont_print_str_center(160, 80, "Press Alt+Q to apply the new settings", UI_SCREEN_W, ui_scale);
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
        if (d->items[i]->type == MAIN_MENU_ITEM_TYPE_BOOL) {
            bool *v = d->items[i]->value;
            lib_sprintf(buf, sizeof(buf), "%s %s", d->items[i]->text, *v ? "On" : "Off");
        } else if (d->items[i]->type == MAIN_MENU_ITEM_TYPE_INT) {
            int *v = d->items[i]->value;
            lib_sprintf(buf, sizeof(buf), "%s %d", d->items[i]->text, *v);
        } else if (d->items[i]->type == MAIN_MENU_ITEM_TYPE_STR) {
            const char **v = d->items[i]->value;
            lib_sprintf(buf, sizeof(buf), "%s %s", d->items[i]->text, *v);
        } else {
            lib_sprintf(buf, sizeof(buf), "%s", d->items[i]->text);
        }
        lbxfont_print_str_center(0xa0, y, buf, UI_SCREEN_W, ui_scale);
    }
    d->frame = (d->frame + 1) % 0x30;
}

static void main_menu_item_do_plus(struct main_menu_data_s *d)
{
    const struct main_menu_item_s *it = d->items[d->clicked_i];
    if (it->type == MAIN_MENU_ITEM_TYPE_RETURN) {
        d->ret = it->ret_action;
        d->flag_done = true;
    } else if (it->type == MAIN_MENU_ITEM_TYPE_PAGE) {
        main_menu_push_page(d, it->page);
    } else if (it->type == MAIN_MENU_ITEM_TYPE_PAGE_BACK) {
        main_menu_pop_page(d);
    } else if (it->type == MAIN_MENU_ITEM_TYPE_FUNCTION) {
        if (it->func) {
            it->func(d);
        }
    } else if (it->type == MAIN_MENU_ITEM_TYPE_BOOL) {
        if (it->func) {
            it->func(d);
        } else {
            bool *v = it->value;
            *v = !*v;
        }
        main_menu_refresh_screen(d);
    } else if (it->type == MAIN_MENU_ITEM_TYPE_INT) {
        int *v = it->value;
        int step = 1;
        if (!kbd_is_modifier(MOO_MOD_CTRL)) {
            step = (it->value_max - it->value_min + 9) / 10;
        }
        *v += step;
        SETMIN(*v, it->value_max);
        if (it->func) {
            it->func(d);
        }
        main_menu_refresh_screen(d);
    } else if (it->type == MAIN_MENU_ITEM_TYPE_STR) {
        if (it->func) {
            it->func(d);
        }
        main_menu_refresh_screen(d);
    }
}

static void main_menu_item_do_minus(struct main_menu_data_s *d)
{
    const struct main_menu_item_s *it = d->items[d->highlight];
    if (it->type == MAIN_MENU_ITEM_TYPE_INT) {
        int *v = it->value;
        int step = 1;
        if (!kbd_is_modifier(MOO_MOD_CTRL)) {
            step = (it->value_max - it->value_min + 9) / 10;
        }
        *v -= step;
        SETMAX(*v, it->value_min);
        if (it->func) {
            it->func(d);
        }
        main_menu_refresh_screen(d);
    } else {
        d->clicked_i = d->highlight;
        main_menu_item_do_plus(d);
    }
}

static main_menu_action_t main_menu_do(struct main_menu_data_s *d)
{
    bool flag_fadein = false;
    d->pages_stack_i = -1;
    d->frame = 0;
    d->flag_done = false;
    d->ret = -1;
    mm_ui_scale_helper = ui_scale;
    mm_hw_aspect_helper = hw_uiopt_cb_aspect_get();
    if (ui_draw_finish_mode != 0) {
        ui_palette_fadeout_19_19_1();
    }
    lbxpal_select(1, -1, 0);
    ui_cursor_setup_area(1, &ui_cursor_area_all_i1);
    uiobj_table_clear();

    /* HACK these fix the gfx mess after canceling a load */
    ui_draw_erase_buf();
    uiobj_finish_frame();

    main_menu_push_page(d, MAIN_MENU_PAGE_MAIN);

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
        } else if ((oi1 == d->oi_plus || oi1 == d->oi_equals) && d->highlight != -1) {
            d->clicked_i = d->highlight;
            ui_sound_play_sfx_24();
            main_menu_item_do_plus(d);
        } else if ((oi1 == d->oi_minus) && d->highlight != -1) {
            ui_sound_play_sfx_24();
            main_menu_item_do_minus(d);
        } else if (d->clicked_i != -1) {
            ui_sound_play_sfx_24();
            main_menu_item_do_plus(d);
        } else if (oi1 == UIOBJI_ESC) {
            if (d->highlight != -1) {
                ui_sound_play_sfx_24();
                main_menu_item_do_minus(d);
            } else {
                ui_sound_play_sfx_06();
                main_menu_pop_page(d);
            }
        }
        if(1
          && ui_scale != mm_ui_scale_helper
          && d->ret == MAIN_MENU_ACT_QUIT_GAME) {
            ui_scale = mm_ui_scale_helper;
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
