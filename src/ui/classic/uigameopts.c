#include "config.h"

#include <stdio.h>

#include "uigameopts.h"
#include "comp.h"
#include "hw.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "options.h"
#include "types.h"
#include "uicursor.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiload.h"
#include "uiobj.h"
#include "uiopt.h"
#include "uipal.h"
#include "uisave.h"
#include "uisound.h"

/* -------------------------------------------------------------------------- */

static bool ui_opt_toggle_mwi_slider(void)
{
    ui_mwi_slider = !ui_mwi_slider;
    return true;
}

static bool ui_opt_toggle_mwi_counter(void)
{
    ui_mwi_counter = !ui_mwi_counter;
    return true;
}

static bool ui_opt_cb_scrollspd(void)
{
    ui_sm_scroll_speed = 3;
    return true;
}

static const struct uiopt_s ui_uiopts[] = {
    UIOPT_ITEM_BOOL("Invert wheel slider", ui_mwi_slider, ui_opt_toggle_mwi_slider),
    UIOPT_ITEM_BOOL("Invert wheel counter", ui_mwi_counter, ui_opt_toggle_mwi_counter),
    UIOPT_ITEM_FUNC("Scroll spd", ui_opt_cb_scrollspd),
    UIOPT_ITEM_SLIDER_INT(ui_sm_scroll_speed, 0, UI_SCROLL_SPEED_MAX),
    UIOPT_ITEM_END
};

/* -------------------------------------------------------------------------- */

#define NEWOPTS_MAX 30

struct gameopts_new_s {
    const struct uiopt_s *u;
    int16_t oi;
    int16_t value;
};

struct gameopts_data_s {
    int num_newopts, newopt_y;
    struct gameopts_new_s *newopts;
    uint8_t *gfx_game;
    uint8_t *gfx_save;
    uint8_t *gfx_load;
    uint8_t *gfx_quit;
    uint8_t *gfx_silent;
    uint8_t *gfx_fx;
    uint8_t *gfx_music;
};

static void load_go_data(struct gameopts_data_s *d)
{
    d->gfx_game = lbxfile_item_get(LBXFILE_VORTEX, 0x1c);
    d->gfx_save = lbxfile_item_get(LBXFILE_VORTEX, 0x1d);
    d->gfx_load = lbxfile_item_get(LBXFILE_VORTEX, 0x1e);
    d->gfx_quit = lbxfile_item_get(LBXFILE_VORTEX, 0x1f);
    d->gfx_silent = lbxfile_item_get(LBXFILE_VORTEX, 0x20);
    d->gfx_fx = lbxfile_item_get(LBXFILE_VORTEX, 0x21);
    d->gfx_music = lbxfile_item_get(LBXFILE_VORTEX, 0x22);
}

static void free_go_data(struct gameopts_data_s *d)
{
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_game);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_save);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_load);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_quit);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_silent);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_fx);
    lbxfile_item_release(LBXFILE_VORTEX, d->gfx_music);
}

static void gameopts_slider_cb(void *ctx, uint8_t slideri, int16_t value)
{
    struct gameopts_data_s *d = ctx;
    const struct gameopts_new_s *o = &(d->newopts[slideri]);
    const struct uiopt_s *u = o->u;
    if (u->type == UIOPT_TYPE_SLIDER_CALL) {
        u->ts.set(o->value);
    } else {
        *u->ts.value_ptr = o->value;
    }
}

static void gameopts_draw_cb(void *vptr)
{
    struct gameopts_data_s *d = vptr;
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, d->gfx_game, UI_SCREEN_W, ui_scale);
    if (d->num_newopts) {
        const struct gameopts_new_s *o = d->newopts;
        int x = 203, y = 51;
        ui_draw_filled_rect(203, 50, 292, 127, 0x00, ui_scale);
        ui_draw_filled_rect(213, 35, 278, 48, 0x1e, ui_scale); /* hide "Sound" */
        lbxfont_select(0, 1, 0, 0);
        for (int i = 0; i < d->num_newopts; ++i) {
            const struct uiopt_s *u;
            int xoff;
            const char *str;
            str = 0;
            u = o->u;
            xoff = 0;
            switch (u->type) {
                case UIOPT_TYPE_FUNC:
                    str = u->str;
                    break;
                case UIOPT_TYPE_BOOL:
                    str = u->str;
                    if (str) {
                        xoff = 6;
                        ui_draw_box1(x, y, x + 4, y + 4, 0x40, 0x40, ui_scale);
                        if (*u->tb.value_ro_ptr) {
                            ui_draw_filled_rect(x + 1, y + 1, x + 3, y + 3, 0x1c, ui_scale);
                        }
                    }
                    break;
                case UIOPT_TYPE_CYCLE:
                    sprintf(ui_data.strbuf, "%s: %s", u->str, u->tc.get());
                    str = ui_data.strbuf;
                    break;
                case UIOPT_TYPE_SLIDER_CALL:
                case UIOPT_TYPE_SLIDER_INT:
                    {
                        int v;
                        y -= 7;
                        v = o->value;
                        if (v > 0) {
                            ui_draw_slider(x + 44, y + 1, (v * 40) / (u->ts.vmax - u->ts.vmin), 1, 0, 0x08, ui_scale);
                        }
                        lbxfont_print_str_normal(289, y, ">", UI_SCREEN_W, ui_scale);
                        str = "<";
                        xoff = 40;
                        o += 2; i += 2; /* skip the "<" and ">" entries */
                    }
                    break;
                default:
                    break;
            }
            if (str) {
                lbxfont_print_str_normal(x + xoff, y, str, UI_SCREEN_W, ui_scale);
                y += 7;
            }
            ++o;
        }
    }
}

static bool gameopts_new_add(struct gameopts_data_s *d, const struct uiopt_s *u)
{
    struct gameopts_new_s *o;
    int y, num = d->num_newopts;
    y = (num == 0) ? 50 : d->newopt_y;
    o = &(d->newopts[num]);
    while (u->type != UIOPT_TYPE_NONE) {
        if (num >= NEWOPTS_MAX) {
            goto fail;
        }
        switch (u->type) {
            case UIOPT_TYPE_FUNC:
            case UIOPT_TYPE_BOOL:
            case UIOPT_TYPE_CYCLE:
                o->u = u;
                {
                    int x1;
                    x1 = ((u[1].type == UIOPT_TYPE_SLIDER_CALL) || (u[1].type == UIOPT_TYPE_SLIDER_INT)) ? 222 : 292;
                    o->oi = uiobj_add_mousearea(203, y, x1, y + 6, MOO_KEY_UNKNOWN);
                }
                break;
            case UIOPT_TYPE_SLIDER_CALL:
            case UIOPT_TYPE_SLIDER_INT:
                if (num >= (NEWOPTS_MAX - 2)) {
                    goto fail;
                }
                y -= 7;
                o->u = u;
                o->value = *u->ts.value_ptr;
                o->oi = uiobj_add_slider_func(247, y, u->ts.vmin, u->ts.vmax, 40, 5, &o->value, gameopts_slider_cb, d, num);
                ++num;
                ++o;
                o->u = u;
                o->oi = uiobj_add_mousearea(243, y, 246, y + 6, MOO_KEY_UNKNOWN);
                ++num;
                ++o;
                o->u = u;
                o->oi = uiobj_add_mousearea(289, y, 292, y + 6, MOO_KEY_UNKNOWN);
                break;
            default:
                break;
        }
        ++num;
        ++o;
        y += 7;
        ++u;
    }
    d->newopt_y = y;
    d->num_newopts = num;
    return true;
fail:
    log_warning("%s: BUG: NEWOPTS_MAX (%i) too small!\n", __func__, NEWOPTS_MAX);
    d->num_newopts = num;
    return false;
}

/* -------------------------------------------------------------------------- */

gameopts_act_t ui_gameopts(struct game_s *g, int *load_game_i_ptr)
{
    struct gameopts_data_s d;
    struct gameopts_new_s newopts[NEWOPTS_MAX];
    bool flag_done = false;
    gameopts_act_t ret = GAMEOPTS_DONE;
    int16_t oi_quit, oi_done, oi_load, oi_save, oi_silent, oi_fx, oi_music;
    int16_t fxmusic = opt_music_enabled ? 2 : (opt_sfx_enabled ? 1 : 0);

    load_go_data(&d);

    ui_palette_fadeout_19_19_1();
    lbxpal_select(2, -1, 0);
    ui_draw_finish_mode = 2;

    uiobj_table_clear();

    d.num_newopts = 0;
    d.newopts = newopts;
    if (ui_extra_enabled) {
        gameopts_new_add(&d, uiopts_audio);
        gameopts_new_add(&d, ui_uiopts);
        gameopts_new_add(&d, hw_uiopts);
        gameopts_new_add(&d, hw_uiopts_extra);
    }

    oi_load = uiobj_add_t0(115, 81, "", d.gfx_load, MOO_KEY_l);
    oi_save = uiobj_add_t0(115, 56, "", d.gfx_save, MOO_KEY_s);
    oi_quit = uiobj_add_t0(115, 106, "", d.gfx_quit, MOO_KEY_q);
    if (d.num_newopts == 0) {
        oi_silent = uiobj_add_t3(210, 56, "", d.gfx_silent, &fxmusic, 0, MOO_KEY_i);
        oi_fx = uiobj_add_t3(210, 81, "", d.gfx_fx, &fxmusic, 1, MOO_KEY_f);
        oi_music = uiobj_add_t3(210, 106, "", d.gfx_music, &fxmusic, 2, MOO_KEY_m);
    } else {
        oi_silent = UIOBJI_INVALID;
        oi_fx = UIOBJI_INVALID;
        oi_music = UIOBJI_INVALID;
    }
    oi_done = uiobj_add_mousearea(173, 134, 226, 150, MOO_KEY_SPACE);
    uiobj_set_downcount(1);
    uiobj_set_callback_and_delay(gameopts_draw_cb, &d, 2);

    while (!flag_done) {
        int16_t oi;
        oi = uiobj_handle_input_cond();
        ui_delay_prepare();
        if ((oi == UIOBJI_ESC) || (oi == oi_done)) {
            ui_sound_play_sfx_24();
            flag_done = true;
        } else if ((oi == oi_silent) || (oi == oi_fx) || (oi == oi_music)) {
            opt_music_enabled = (fxmusic == 2);
            opt_sfx_enabled = (fxmusic >= 1);
            ui_sound_play_sfx_24();
        } else if (oi == oi_load) {
            int loadi;
            ui_sound_play_sfx_24();
            loadi = ui_load_game();
            if (loadi >= 0) {
                *load_game_i_ptr = loadi;
                ret = GAMEOPTS_LOAD;
            }
            flag_done = true;
        } else if (oi == oi_save) {
            ui_sound_play_sfx_24();
            ui_save_game(g);
            flag_done = true;
        } else if (oi == oi_quit) {
            ret = GAMEOPTS_QUIT;
            ui_sound_play_sfx_24();
            flag_done = true;
        } else if (d.num_newopts > 0) {
            struct gameopts_new_s *o;
            o = d.newopts;
            for (int i = 0; i < d.num_newopts; ++i, ++o) {
                if (oi == o->oi) {
                    const struct uiopt_s *u;
                    u = o->u;
                    switch (u->type) {
                        case UIOPT_TYPE_FUNC:
                            if (u->tf.cb) {
                                u->tf.cb();
                                if (i < (d.num_newopts - 2)) {
                                    ++o;
                                    u = o->u;
                                    if ((u->type == UIOPT_TYPE_SLIDER_CALL) || (u->type == UIOPT_TYPE_SLIDER_INT)) {
                                        o->value = *u->ts.value_ptr;
                                    }
                                }
                            }
                            break;
                        case UIOPT_TYPE_BOOL:
                            u->tb.toggle();
                            break;
                        case UIOPT_TYPE_CYCLE:
                            u->tc.next();
                            break;
                        case UIOPT_TYPE_SLIDER_CALL:
                        case UIOPT_TYPE_SLIDER_INT:
                            /* ois: slider, -, + */
                            if (!((i < (d.num_newopts - 2)) && (u == o[1].u) && (u == o[2].u))) {
                                int v, n;
                                n = (u->ts.vmax - u->ts.vmin) / 20;
                                SETMAX(n, 1);
                                v = *u->ts.value_ptr;
                                if ((i < (d.num_newopts - 1)) && (u == o[1].u)) {
                                    --o;
                                    SUBSATT(v, n, u->ts.vmin);
                                } else {
                                    o -= 2;
                                    ADDSATT(v, n, u->ts.vmax);
                                }
                                o->value = v;
                            } /* else { slider, o->value already updated by uiobj.c } */
                            if (u->type == UIOPT_TYPE_SLIDER_CALL) {
                                u->ts.set(o->value);
                            } else {
                                *u->ts.value_ptr = o->value;
                            }
                            break;
                        default:
                            break;
                    }
                    ui_sound_play_sfx_24();
                    break;
                }
            }
        }
        if (!flag_done) {
            gameopts_draw_cb(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(2);
        }
    }

    if (ret != GAMEOPTS_QUIT) {
        ui_palette_fadeout_a_f_1();
        lbxpal_select(0, -1, 0);
    }
    ui_draw_finish_mode = 2;

    uiobj_unset_callback();
    free_go_data(&d);
    return ret;
}
