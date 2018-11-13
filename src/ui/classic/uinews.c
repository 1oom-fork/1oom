#include "config.h"

#include <stdio.h>

#include "ui.h"
#include "game_news.h"
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
#include "uidefs.h"
#include "uidelay.h"
#include "uidraw.h"
#include "uinews.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"

/* -------------------------------------------------------------------------- */

struct news_data_s {
    int frame;
    struct news_s *ns;
    const char *str;
};

/* -------------------------------------------------------------------------- */

static void news_free_data(void)
{
    if (ui_data.gfx.news.icon != 0) {
        lbxfile_item_release(LBXFILE_NEWSCAST, ui_data.gfx.news.icon);
        ui_data.gfx.news.icon = 0;
    }
}

static void news_load_data(news_type_t type)
{
    if (!ui_data.news.flag_also) {
        news_free_data();
        lbxgfx_set_frame_0(ui_data.gfx.news.nc);
        lbxgfx_set_frame_0(ui_data.gfx.news.world);
        lbxgfx_set_frame_0(ui_data.gfx.news.gnn);
    }
    if (type != GAME_NEWS_NONE) {
        ui_data.gfx.news.icon = lbxfile_item_get(LBXFILE_NEWSCAST, 3 + (int)type, 0);
    } else {
        news_free_data();
    }
}

static void ui_news_cb1(void *vptr)
{
    struct news_data_s *d = vptr;
    ui_draw_filled_rect(32, 142, 287, 182, 0);
    ui_draw_filled_rect(34, 184, 285, 191, 0);
    ui_draw_line1(33, 182, 286, 182, 0);
    ui_draw_line1(32, 183, 287, 183, 0);
    {
        uint8_t *gfx = ui_data.gfx.news.nc;
        int fn = lbxgfx_get_frame(gfx);
        lbxgfx_set_frame_0(gfx);
        for (int f = 0; f <= fn; ++f) {
            lbxgfx_draw_frame(14, 14, gfx, UI_SCREEN_W);
        }
    }
    if (ui_data.gfx.news.icon != 0) {
        lbxgfx_draw_frame(208, 38, ui_data.gfx.news.icon, UI_SCREEN_W);
    }
    {
        uint8_t *gfx = ui_data.gfx.news.world;
        int fn = lbxgfx_get_frame(gfx);
        lbxgfx_set_frame_0(gfx);
        for (int f = 0; f <= fn; ++f) {
            lbxgfx_draw_frame(76, 36, gfx, UI_SCREEN_W);
        }
    }
    lbxfont_select(3, 1, 0, 0);
    ui_draw_filled_rect(38, 145, 284, 190, 0);
    lbxfont_set_space_w(2);
    lbxfont_print_str_split(38, 145, 245, d->str, 3, UI_SCREEN_W, UI_SCREEN_H);
    if (d->ns->type == GAME_NEWS_STATS) {
        lbxfont_select(3, 1, 0, 0);
        for (int i = 0; i < d->ns->statsnum; ++i) {
            char buf[5];
            int x, y;
            x = 48 + (i / 3) * 122;
            y = 157 + (i % 3) * 10;
            sprintf(buf, "%i.", i + 1);
            lbxfont_print_str_right(x, y, buf, UI_SCREEN_W);
            lbxfont_print_str_normal(x + 7, y, d->ns->stats[i], UI_SCREEN_W);
        }
    }
    ++d->frame;
}

static void ui_news_draw_start_anim(void)
{
    int frame;
    ui_delay_1();
    ui_sound_stop_music();
    uiobj_table_clear();
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, ui_data.gfx.news.tv, UI_SCREEN_W);
    uiobj_finish_frame();
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, ui_data.gfx.news.tv, UI_SCREEN_W);
    ui_sound_play_music(9);
    frame = 0;
    while (frame < 25) {
        ui_delay_prepare();
        if (frame > 0) {
            uint16_t f;
            f = lbxgfx_get_frame(ui_data.gfx.news.gnn) - 1;
            lbxgfx_set_new_frame(ui_data.gfx.news.gnn, f);
            lbxgfx_draw_frame(14, 14, ui_data.gfx.news.gnn, UI_SCREEN_W);
        }
        lbxgfx_draw_frame(14, 14, ui_data.gfx.news.gnn, UI_SCREEN_W);
        ui_draw_filled_rect(32, 142, 287, 182, 0xc1);
        ui_draw_filled_rect(34, 184, 285, 191, 0xc1);
        ui_draw_line1(33, 182, 286, 182, 0xc1);
        ui_draw_line1(32, 183, 287, 183, 0xc1);
        ui_draw_finish();
        ui_delay_ticks_or_click(1);
        ++frame;
    }
}

/* -------------------------------------------------------------------------- */

void ui_news_won(bool flag_good)
{
    bool flag_skip = false, flag_hmm;
    struct news_data_s d;
    struct news_s ns;

    lbxpal_select(0, -1, 0);
    lbxpal_set_update_range(0, 255);

    ui_draw_finish_mode = 2;

    d.str = flag_good ? game_str_gnn_end_good : game_str_gnn_end_tyrant;
    d.ns = &ns;
    ns.type = GAME_NEWS_NONE;
    news_load_data(GAME_NEWS_NONE);

    ui_news_draw_start_anim();

    uiobj_table_clear();
    uiobj_add_mousearea(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, MOO_KEY_UNKNOWN, -1);
    uiobj_set_downcount(1);
    uiobj_set_callback_and_delay(ui_news_cb1, &d, 3);

    flag_hmm = true;
    d.frame = 0;
    for (int i = 0; (i < 0x46) && !flag_skip; ++i) {
        int16_t oi;
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if (oi != 0) {
            flag_skip = true;
            break;
        }
        ui_news_cb1(&d);
        ui_delay_ticks_or_click(3);
        if (flag_hmm) {
            /*ui_news_sub2(); FIXME TODO */
        } else {
            ui_draw_finish();
        }
        flag_hmm = false;
    }

    hw_audio_music_fadeout();
    uiobj_unset_callback();
    ui_data.news.flag_also = false;
}

void ui_news(struct game_s *g, struct news_s *ns)
{
    bool flag_skip = false, flag_hmm;
    struct news_data_s d;
    d.ns = ns;
    if (!ui_data.news.flag_also) {
        if (ui_draw_finish_mode == 0) {
            ui_palette_fadeout_a_f_1();
        }
        ui_draw_finish_mode = 2;
        ui_news_draw_start_anim();
        flag_hmm = true;
    } else {
        d.str = game_str_gnn_also;
        for (int i = 0; (i < 5) && !flag_skip; ++i) {
            int16_t oi;
            ui_delay_prepare();
            oi = uiobj_handle_input_cond();
            if (oi != 0) {
                flag_skip = true;
            }
            if (!flag_skip) {
                ui_news_cb1(&d);
                ui_delay_ticks_or_click(3);
                ui_draw_finish();
            }
        }
        flag_hmm = false;
    }
    game_news_get_msg(g, ns, ui_data.strbuf);
    d.str = ui_data.strbuf;

    news_load_data(ns->type);
    uiobj_table_clear();
    uiobj_add_mousearea(0, 0, UI_SCREEN_W - 1, UI_SCREEN_H - 1, MOO_KEY_UNKNOWN, -1);
    uiobj_set_downcount(1);
    uiobj_set_callback_and_delay(ui_news_cb1, &d, 3);

    flag_skip = false;
    while (!flag_skip) {
        int16_t oi;
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if (oi != 0) {
            flag_skip = true;
            break;
        }
        ui_news_cb1(&d);
        ui_delay_ticks_or_click(3);
        if (flag_hmm) {
            /*ui_news_sub2(); FIXME TODO */
        } else {
            ui_draw_finish();
        }
        flag_hmm = false;
    }
    ui_data.news.flag_also = true;
    hw_audio_music_fadeout();
    uiobj_unset_callback();
    uiobj_table_clear();
}

void ui_news_start(void)
{
    ui_data.news.flag_also = false;
}

void ui_news_end(void)
{
    if (ui_data.news.flag_also) {
        ui_data.news.flag_also = false;
        ui_palette_fadeout_a_f_1();
        ui_draw_finish_mode = 2;
    }
}
