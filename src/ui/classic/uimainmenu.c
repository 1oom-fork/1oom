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
#include "menu.h"
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
    const int num_saves = ui_extra_enabled ? NUM_ALL_SAVES : NUM_ALL_SAVES;
    for (int i = 0; i < num_saves; ++i) {
        if (game_save_tbl_have_save[i]) {
            return true;
        }
    }
    return false;
}

struct main_menu_data_s;
static void main_menu_set_item_dimensions(struct main_menu_data_s *d, int i);

/* -------------------------------------------------------------------------- */

#define MM_MAX_ITEMS_PER_PAGE 8
#define MM_PAGE_STACK_SIZE 4

typedef enum {
    MAIN_MENU_ITEM_GAME_CONTINUE,
    MAIN_MENU_ITEM_GAME_LOAD,
    MAIN_MENU_ITEM_GAME_NEW,
    MAIN_MENU_ITEM_QUIT,
    MAIN_MENU_ITEM_BACK,
    MAIN_MENU_ITEM_NUM,
} main_menu_item_id_t;

typedef enum {
    MAIN_MENU_PAGE_MAIN,
    MAIN_MENU_PAGE_NUM,
} main_menu_page_id_t;

struct main_menu_page_s {
    main_menu_item_id_t item_id[MM_MAX_ITEMS_PER_PAGE + 1];
    void (*set_item_dimensions)(struct main_menu_data_s *, int i);
    struct main_menu_item_data_s *item_data;
};

static struct main_menu_item_data_s mm_items[MAIN_MENU_ITEM_NUM] = {
    {
        MAIN_MENU_ITEM_TYPE_RETURN,
        NULL, main_menu_have_save_continue,
        "Continue", NULL, NULL, MAIN_MENU_ACT_CONTINUE_GAME,
        0, 0,
        MOO_KEY_c,
    },
    {
        MAIN_MENU_ITEM_TYPE_RETURN,
        NULL, main_menu_have_save_any,
        "Load Game", NULL, NULL, MAIN_MENU_ACT_LOAD_GAME,
        0, 0,
        MOO_KEY_l,
    },
    {
        MAIN_MENU_ITEM_TYPE_RETURN,
        NULL, NULL,
        "New Game", NULL, NULL, MAIN_MENU_ACT_NEW_GAME,
        0, 0,
        MOO_KEY_n,
    },
    {
        MAIN_MENU_ITEM_TYPE_RETURN,
        NULL, NULL,
        "Quit to OS", NULL, NULL, MAIN_MENU_ACT_QUIT_GAME,
        0, 0,
        MOO_KEY_q,
    },
    {
        MAIN_MENU_ITEM_TYPE_PAGE_BACK,
        NULL, NULL,
        "Back", NULL, NULL, 0,
        0, 0,
        MOO_KEY_b,
    },
};

static struct main_menu_page_s mm_pages[MAIN_MENU_PAGE_NUM] = {
    {
        {
            MAIN_MENU_ITEM_GAME_CONTINUE,
            MAIN_MENU_ITEM_GAME_LOAD,
            MAIN_MENU_ITEM_GAME_NEW,
            MAIN_MENU_ITEM_QUIT,
            MAIN_MENU_ITEM_NUM,
        },
        main_menu_set_item_dimensions,
        NULL,
    },
};

struct main_menu_item_s {
    struct main_menu_item_data_s data;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    int16_t oi;
    bool active;
};

struct main_menu_data_s {
    main_menu_page_id_t page_stack[MM_PAGE_STACK_SIZE];
    int page_stack_i;
    struct main_menu_item_s items[MM_MAX_ITEMS_PER_PAGE];
    uint8_t item_count;
    int16_t oi_quit;
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

static void main_menu_get_item_string(const struct main_menu_item_s *it, char *buf, int len)
{
    if (it->data.type == MAIN_MENU_ITEM_TYPE_BOOL) {
        bool *v = it->data.value_ptr;
        lib_sprintf(buf, len, "%s %s", it->data.text, *v ? "On" : "Off");
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_INT) {
        int *v = it->data.value_ptr;
        lib_sprintf(buf, len, "%s %d", it->data.text, *v);
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_ENUM) {
        int *v = it->data.value_ptr;
        lib_sprintf(buf, len, "%s %s", it->data.text, it->data.get_text_value(*v));
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_STR) {
        const char *(*f)(void) = it->data.value_ptr;
        lib_sprintf(buf, len, "%s %s", it->data.text, f());
    } else {
        lib_sprintf(buf, len, "%s", it->data.text);
    }
}

static void main_menu_draw_cb(void *vptr)
{
    struct main_menu_data_s *d = vptr;
    char buf[64];
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
    for (int i = 0; i < d->item_count; ++i) {
        struct main_menu_item_s *it = &d->items[i];
        if (it->active) {
            lbxfont_select(4, (d->highlight == i) ? 3 : 2, 0, 0);
        } else {
            lbxfont_select(4, 7, 0, 0);
        }
        main_menu_get_item_string(it, buf, sizeof(buf));
        lbxfont_print_str_center(it->x, it->y, buf, UI_SCREEN_W, ui_scale);
    }
    d->frame = (d->frame + 1) % 0x30;
}

static void main_menu_set_item_wh(struct main_menu_item_s *it)
{
    it->w = 0xc8;
    it->h = 0x10;
}

static void main_menu_set_item_dimensions(struct main_menu_data_s *d, int i)
{
    struct main_menu_item_s *it = &d->items[i];
    uint16_t step_y;
    main_menu_set_item_wh(it);
    step_y = 0x40 / d->item_count;
    it->x = 0xa0;
    it->y = 0x7f + step_y * i;
}

static void main_menu_refresh_screen(struct main_menu_data_s *d)
{
    lbxgfx_set_frame_0(d->gfx_vortex);
}

static struct main_menu_item_data_s *main_menu_get_local_itemdata(main_menu_page_id_t page_i, int i)
{
    if (mm_pages[page_i].item_id[i] < 0 || mm_pages[page_i].item_id[i] >= MAIN_MENU_ITEM_NUM) {
        return NULL;
    }
    struct main_menu_item_data_s *ret = &mm_items[mm_pages[page_i].item_id[i]];
    if (!ret->text) {
        return NULL;
    }
    return ret;
}

static struct main_menu_item_data_s *main_menu_get_itemdata(main_menu_page_id_t page_i, int i)
{
    struct main_menu_page_s *page = &mm_pages[page_i];
    if (page->item_data != NULL) {
        if (page->item_data[i].type == MAIN_MENU_ITEM_TYPE_NONE) {
            return NULL;
        }
        return &page->item_data[i];
    }
    return main_menu_get_local_itemdata(page_i, i);
}

static bool main_menu_load_page(struct main_menu_data_s *d, main_menu_page_id_t page_i)
{
    if (page_i < 0 || page_i >= MAIN_MENU_PAGE_NUM) {
        return false;
    }
    main_menu_refresh_screen(d);
    uiobj_table_clear();
    d->oi_quit = uiobj_add_inputkey(MOO_KEY_q | MOO_MOD_ALT);
    d->item_count = 0;
    for (int i = 0; i < MM_MAX_ITEMS_PER_PAGE; ++i) {
        struct main_menu_item_s *it = &d->items[d->item_count];
        struct main_menu_item_data_s *it_data = main_menu_get_itemdata(page_i, i);
        it->oi = UIOBJI_INVALID;
        if (it_data) {
            it->data = *it_data;
        } else {
            break;
        }
        ++d->item_count;
    }
    for (int i = 0; i < d->item_count; ++i) {
        struct main_menu_item_s *it = &d->items[i];
        if (d->item_count > 4) {
            mm_pages[page_i].set_item_dimensions(d, i);
        } else {
            main_menu_set_item_dimensions(d, i);
        }
        it->active = it->data.is_active ? it->data.is_active(&d) : true;
        if (it->active) {
            it->oi = uiobj_add_mousearea(it->x - it->w / 2, it->y, it->x + it->w / 2, it->y + it->h - 1, it->data.key);
        }
    }
    return true;
}

static void main_menu_push_page(struct main_menu_data_s *d, main_menu_page_id_t page_i)
{
    if (d->page_stack_i + 1 >= MM_PAGE_STACK_SIZE) {
        return;
    }
    if (main_menu_load_page(d, page_i)) {
        ++d->page_stack_i;
        d->page_stack[d->page_stack_i] = page_i;
    }
}

static void main_menu_pop_page(struct main_menu_data_s *d) {
    if (d->page_stack_i > 0) {
        ui_sound_play_sfx_06();
        --d->page_stack_i;
        main_menu_load_page(d, d->page_stack[d->page_stack_i]);
    }
}

static int main_menu_get_item(struct main_menu_data_s *d, int16_t oi)
{
    for (int i = 0; i < d->item_count; ++i) {
        if (oi == d->items[i].oi) {
            return i;
        }
    }
    return -1;
}

static void main_menu_item_do_plus(struct main_menu_data_s *d)
{
    const struct main_menu_item_s *it = &d->items[d->clicked_i];
    if (it->data.type == MAIN_MENU_ITEM_TYPE_RETURN) {
        d->ret = it->data.action_i;
        d->flag_done = true;
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_PAGE) {
        main_menu_push_page(d, it->data.action_i);
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_PAGE_BACK) {
        main_menu_pop_page(d);
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_FUNCTION) {
        if (it->data.func) {
            it->data.func(d);
        }
        main_menu_refresh_screen(d);
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_BOOL) {
        if (it->data.func) {
            it->data.func(d);
        } else {
            bool *v = it->data.value_ptr;
            *v = !*v;
        }
        main_menu_refresh_screen(d);
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_INT
            || it->data.type == MAIN_MENU_ITEM_TYPE_ENUM) {
        int *v = it->data.value_ptr;
        int step = 1;
        if (!kbd_is_modifier(MOO_MOD_CTRL)) {
            step = (it->data.value_max - it->data.value_min + 9) / 10;
        }
        *v += step;
        SETMIN(*v, it->data.value_max);
        if (it->data.func) {
            it->data.func(d);
        }
        main_menu_refresh_screen(d);
    } else if (it->data.type == MAIN_MENU_ITEM_TYPE_STR) {
        if (it->data.func) {
            it->data.func(d);
        }
        main_menu_refresh_screen(d);
    }
}

static void main_menu_item_do_minus(struct main_menu_data_s *d)
{
    const struct main_menu_item_s *it = &d->items[d->highlight];
    if (it->data.type == MAIN_MENU_ITEM_TYPE_INT
     || it->data.type == MAIN_MENU_ITEM_TYPE_ENUM) {
        int *v = it->data.value_ptr;
        int step = 1;
        if (!kbd_is_modifier(MOO_MOD_CTRL)) {
            step = (it->data.value_max - it->data.value_min + 9) / 10;
        }
        *v -= step;
        SETMAX(*v, it->data.value_min);
        if (it->data.func) {
            it->data.func(d);
        }
        main_menu_refresh_screen(d);
    }
    else {
        d->clicked_i = d->highlight;
        main_menu_item_do_plus(d);
    }
}

static main_menu_action_t main_menu_do(struct main_menu_data_s *d)
{
    int16_t oi_tutor, oi_extra;
    bool flag_fadein = false;

    d->page_stack_i = -1;
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

    main_menu_push_page(d, MAIN_MENU_PAGE_MAIN);

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
            main_menu_item_do_plus(d);
        } else if (oi1 == UIOBJI_ESC) {
            if (d->highlight != -1) {
                ui_sound_play_sfx_24();
                main_menu_item_do_minus(d);
            } else {
                main_menu_pop_page(d);
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
