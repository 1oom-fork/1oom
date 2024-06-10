#include "config.h"

#include "ui.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
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

static bool main_menu_have_save_continue(void) {
    return game_save_tbl_have_save[GAME_SAVE_I_CONTINUE];
}

static bool main_menu_have_save_any(void) {
    const int num_saves = ui_extra_enabled ? NUM_ALL_SAVES : NUM_ALL_SAVES;
    for (int i = 0; i < num_saves; ++i) {
        if (game_save_tbl_have_save[i]) {
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------- */

#define MM_PAGE_STACK_SIZE 4

typedef enum {
    MAIN_MENU_PAGE_MAIN,
    MAIN_MENU_PAGE_NUM,
} main_menu_page_id_t;

struct main_menu_item_s {
    struct menu_item_data_s data;
    uint16_t font_i;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    int16_t oi;
    bool active;
};

struct main_menu_data_s {
    void (*set_item_dimensions)(struct main_menu_data_s *, int i);
    main_menu_page_id_t page_stack[MM_PAGE_STACK_SIZE];
    int page_stack_i;
    int current_page_i;
    struct main_menu_item_s items[MENU_MAX_ITEMS_PER_PAGE];
    uint8_t item_count;
    int frame;
    bool refresh;
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

/* -------------------------------------------------------------------------- */

static void main_menu_get_item_string(const struct main_menu_item_s *it, char *buf, int len)
{
    if (it->data.type == MENU_ITEM_TYPE_BOOL) {
        bool *v = it->data.value_ptr;
        lib_sprintf(buf, len, "%s %s", it->data.text, *v ? "On" : "Off");
    } else if (it->data.type == MENU_ITEM_TYPE_INT) {
        int *v = it->data.value_ptr;
        lib_sprintf(buf, len, "%s %d", it->data.text, *v);
    } else if (it->data.type == MENU_ITEM_TYPE_ENUM) {
        int *v = it->data.value_ptr;
        lib_sprintf(buf, len, "%s %s", it->data.text, it->data.get_text_value(*v));
    } else if (it->data.type == MENU_ITEM_TYPE_STR) {
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
    if (d->refresh) {
        lbxgfx_set_frame_0(d->gfx_vortex);
        d->refresh = false;
    } else {
        hw_video_copy_back_from_page2();
    }
    lbxgfx_draw_frame(0, 0, d->gfx_vortex, UI_SCREEN_W, ui_scale);
    if (!ui_extra_enabled) {
        lbxgfx_draw_frame(0, 0, d->gfx_title, UI_SCREEN_W, ui_scale);
        hw_video_copy_back_to_page2();
    } else {
        lbxgfx_draw_frame_offs(0, 0, d->gfx_title, 0, 0, UI_VGA_W - 1, 191, UI_SCREEN_W, ui_scale);
        hw_video_copy_back_to_page2();
        lbxfont_select(2, 7, 0, 0);
        lbxfont_print_str_center(160, 193, "PROGRAM VERSION " PACKAGE_NAME " " VERSION_STR, UI_SCREEN_W, ui_scale);
    }
    for (int i = 0; i < d->item_count; ++i) {
        struct main_menu_item_s *it = &d->items[i];
        if (it->active) {
            lbxfont_select(it->font_i, (d->highlight == i) ? 3 : 2, 0, 0);
        } else {
            lbxfont_select(it->font_i, 7, 0, 0);
        }
        main_menu_get_item_string(it, buf, sizeof(buf));
        lbxfont_print_str_center(it->x, it->y, buf, UI_SCREEN_W, ui_scale);
    }
    d->frame = (d->frame + 1) % 0x30;
}

/* -------------------------------------------------------------------------- */

static void main_menu_set_item_wh(struct main_menu_data_s *d, struct main_menu_item_s *it)
{
    it->w = 0xc8;
    it->h = 0x10;
}

static void main_menu_set_item_dimensions(struct main_menu_data_s *d, int i)
{
    struct main_menu_item_s *it = &d->items[i];
    uint16_t step_y;
    it->font_i = 4;
    main_menu_set_item_wh(d, it);
    step_y = 0x40 / d->item_count;
    it->x = 0xa0;
    it->y = 0x7f + step_y * i;
}

static void main_menu_make_main_page(struct main_menu_data_s *d)
{
    d->set_item_dimensions = main_menu_set_item_dimensions;
    menu_make_action_conditional(menu_allocate_item(), "Continue", MAIN_MENU_ACT_CONTINUE_GAME, main_menu_have_save_continue, MOO_KEY_c);
    menu_make_action_conditional(menu_allocate_item(), "Load Game", MAIN_MENU_ACT_LOAD_GAME, main_menu_have_save_any, MOO_KEY_l);
    menu_make_action(menu_allocate_item(), "New Game", MAIN_MENU_ACT_NEW_GAME, MOO_KEY_n);
    menu_make_action(menu_allocate_item(), "Quit to OS", MAIN_MENU_ACT_QUIT_GAME, MOO_KEY_q);
}

/* -------------------------------------------------------------------------- */

struct main_menu_page_s {
    void (*make_page)(struct main_menu_data_s *);
};

static struct main_menu_page_s mm_pages[MAIN_MENU_PAGE_NUM] = {
    {
        main_menu_make_main_page,
    },
};

static bool main_menu_load_page(struct main_menu_data_s *d, main_menu_page_id_t page_i)
{
    if (page_i < 0 || page_i >= MAIN_MENU_PAGE_NUM) {
        return false;
    }
    d->current_page_i = page_i;
    uiobj_table_clear();
    menu_clear();
    mm_pages[page_i].make_page(d);
    d->item_count = menu_get_item_count();
    for (int i = 0; i < d->item_count; ++i) {
        struct main_menu_item_s *it = &d->items[i];
        it->data = *menu_get_item(i);
        d->set_item_dimensions(d, i);
        it->active = it->data.is_active ? it->data.is_active() : true;
        if (it->active) {
            it->oi = uiobj_add_mousearea(it->x - it->w / 2, it->y, it->x + it->w / 2, it->y + it->h - 1, it->data.key);
        } else {
            it->oi = UIOBJI_INVALID;
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

static void main_menu_item_do_plus(struct main_menu_data_s *d, int item_i)
{
    const struct main_menu_item_s *it = &d->items[item_i];
    if (it->data.type == MENU_ITEM_TYPE_RETURN) {
        d->ret = it->data.action_i;
        d->flag_done = true;
    } else if (it->data.type == MENU_ITEM_TYPE_PAGE) {
        main_menu_push_page(d, it->data.action_i);
    } else if (it->data.type == MENU_ITEM_TYPE_PAGE_BACK) {
        main_menu_pop_page(d);
    } else if (it->data.type == MENU_ITEM_TYPE_FUNCTION) {
        if (it->data.func) {
            it->data.func();
        }
    } else if (it->data.type == MENU_ITEM_TYPE_BOOL) {
        if (it->data.func) {
            it->data.func();
        } else {
            bool *v = it->data.value_ptr;
            *v = !*v;
        }
    } else if (it->data.type == MENU_ITEM_TYPE_INT
            || it->data.type == MENU_ITEM_TYPE_ENUM) {
        int *v = it->data.value_ptr;
        int old_value = *v;
        int step = 1;
        if (!kbd_is_modifier(MOO_MOD_CTRL)) {
            step = (it->data.value_max - it->data.value_min + 9) / 10;
        }
        *v += step;
        SETMIN(*v, it->data.value_max);
        if (it->data.func && old_value != *v) {
            it->data.func();
        }
    } else if (it->data.type == MENU_ITEM_TYPE_STR) {
        if (it->data.func) {
            it->data.func();
        }
    }
}

static void main_menu_item_do_minus(struct main_menu_data_s *d, int item_i)
{
    const struct main_menu_item_s *it = &d->items[item_i];
    if (it->data.type == MENU_ITEM_TYPE_INT
     || it->data.type == MENU_ITEM_TYPE_ENUM) {
        int *v = it->data.value_ptr;
        int old_value = *v;
        int step = 1;
        if (!kbd_is_modifier(MOO_MOD_CTRL)) {
            step = (it->data.value_max - it->data.value_min + 9) / 10;
        }
        *v -= step;
        SETMAX(*v, it->data.value_min);
        if (it->data.func && old_value != *v) {
            it->data.func();
        }
    }
    else {
        main_menu_item_do_plus(d, item_i);
    }
}

static main_menu_action_t main_menu_do(struct main_menu_data_s *d)
{
    int16_t oi_tutor;
    bool flag_fadein = false;

    d->page_stack_i = -1;
    d->frame = 0;
    d->refresh = true;
    d->flag_done = false;
    d->ret = -1;
    if (ui_draw_finish_mode != 0) {
        ui_palette_fadeout_19_19_1();
    }
    lbxpal_select(1, -1, 0);
    ui_cursor_setup_area(1, &ui_cursor_area_all_i1);
    uiobj_table_clear();

    ui_draw_erase_buf();
    hw_video_copy_back_to_page2();

    main_menu_push_page(d, MAIN_MENU_PAGE_MAIN);

    oi_tutor = uiobj_add_alt_str("tutor");
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
        if (d->clicked_i != -1) {
            ui_sound_play_sfx_24();
            main_menu_item_do_plus(d, d->clicked_i);
        } else if (oi1 == UIOBJI_ESC) {
            if (d->highlight != -1) {
                ui_sound_play_sfx_24();
                main_menu_item_do_minus(d, d->highlight);
            } else {
                main_menu_pop_page(d);
            }
        }

        if (oi1 == oi_tutor) {
            ui_sound_play_sfx_24();
            d->ret = MAIN_MENU_ACT_TUTOR;
            d->flag_done = true;
        }
        uiobj_finish_frame();
        if ((ui_draw_finish_mode != 0) && !flag_fadein) {
            ui_palette_fadein_4b_19_1();
            flag_fadein = true;
        }
        ui_delay_ticks_or_click(2);
    }
    uiobj_unset_callback();
    if ((d->ret == MAIN_MENU_ACT_NEW_GAME) && ui_extra_enabled) {
        d->ret = MAIN_MENU_ACT_CUSTOM_GAME;
    }
    return d->ret;
}

/* -------------------------------------------------------------------------- */

main_menu_action_t ui_main_menu(struct game_new_options_s *newopts, struct game_new_options_s *customopts, int *load_game_i_ptr)
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
                flag_done = ui_new_game(newopts, false);
                ui_draw_finish_mode = 1;
                break;
            case MAIN_MENU_ACT_CUSTOM_GAME:
                flag_done = ui_new_game(customopts, true);
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
                flag_done = true;
                break;
            default:
                flag_done = true;
                break;
        }
    }
    ui_sound_stop_music();
    /* call run_starmap_exe */
    if (ret != MAIN_MENU_ACT_QUIT_GAME) {
        ui_palette_fadeout_19_19_1();
    }
    free_mainmenu_data(&d);
    return ret;
}
