#include "config.h"

#include <stdio.h>

#include "uixtramenu.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_planet.h"
#include "game_str.h"
#include "hw.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "uicursor.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"
#include "uisound.h"

/* -------------------------------------------------------------------------- */

#define XTRAMENU_POS_X  10
#define XTRAMENU_POS_Y  (170 - (XTRAMENU_NUM * 8))

/* -------------------------------------------------------------------------- */

static void xtramenu_eco_readjust(struct game_s *g, player_id_t pi)
{
    game_update_eco_on_waste(g, pi, false);
}

static void xtramenu_ship_everywhere(struct game_s *g, player_id_t pi)
{
    uint8_t si;
    {
        planet_t *p = &(g->planet[g->planet_focus_i[pi]]);
        if (p->owner != pi) {
            return;
        }
        si = p->buildship;
    }
    for (uint8_t i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if (p->owner == pi) {
            if (si == BUILDSHIP_STARGATE) {
                if (!p->have_stargate) {
                    p->buildship = BUILDSHIP_STARGATE;
                }
            } else {
                p->buildship = si;
            }
        }
    }
}

static void xtramenu_reloc_reloc(struct game_s *g, player_id_t pi)
{
    uint8_t target = g->planet_focus_i[pi];
    if (g->planet[target].owner != pi) {
        return;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if ((p->owner == pi) && (p->reloc != i)) {
            p->reloc = target;
        }
    }
}

static void xtramenu_reloc_all(struct game_s *g, player_id_t pi)
{
    uint8_t target = g->planet_focus_i[pi];
    if (g->planet[target].owner != pi) {
        return;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if (p->owner == pi) {
            p->reloc = target;
        }
    }
}

static void xtramenu_reloc_un(struct game_s *g, player_id_t pi)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if (p->owner == pi) {
            p->reloc = i;
        }
    }
}

/* -------------------------------------------------------------------------- */

static const struct xtramenu_s {
    mookey_t key;
    ui_main_loop_action_t act;
    void (*cb)(struct game_s *g, player_id_t pi);
} xtramenu[XTRAMENU_NUM] = {
    { MOO_KEY_b, UI_MAIN_LOOP_SCRAP_BASES, 0 },
    { MOO_KEY_c, UI_MAIN_LOOP_SPIES_CAUGHT, 0 },
    { MOO_KEY_g, UI_MAIN_LOOP_GOVERN, 0 },
    { MOO_KEY_m, UI_MAIN_LOOP_MSGFILTER, 0 },
    { MOO_KEY_r, UI_MAIN_LOOP_STARMAP, xtramenu_eco_readjust },
    { MOO_KEY_s, UI_MAIN_LOOP_STARMAP, xtramenu_ship_everywhere },
    { MOO_KEY_l, UI_MAIN_LOOP_STARMAP, xtramenu_reloc_reloc },
    { MOO_KEY_a, UI_MAIN_LOOP_STARMAP, xtramenu_reloc_all },
    { MOO_KEY_u, UI_MAIN_LOOP_STARMAP, xtramenu_reloc_un },
    { MOO_KEY_SPACE, UI_MAIN_LOOP_STARMAP, 0 }
};

/* -------------------------------------------------------------------------- */

struct xtramenu_draw_s {
    int highlight;
};

static void xtramenu_draw_cb(void *vptr)
{
    struct xtramenu_draw_s *d = vptr;
    const int x = XTRAMENU_POS_X, y = XTRAMENU_POS_Y;
    int y0 = y + 5;
    ui_draw_filled_rect(x, y, x + 100, y + 5 + XTRAMENU_NUM * 8, 0x06, ui_scale);
    for (int i = 0; i < XTRAMENU_NUM; ++i) {
        lbxfont_select(0, (d->highlight == i) ? 0x1 : 0x0 , 0, 0);
        lbxfont_print_str_normal(x + 10, y0, game_str_tbl_xtramenu[i], UI_SCREEN_W, ui_scale);
        y0 += 8;
    }
}

/* -------------------------------------------------------------------------- */

ui_main_loop_action_t ui_xtramenu(struct game_s *g, player_id_t pi)
{
    bool flag_done = false;
    ui_main_loop_action_t act = UI_MAIN_LOOP_STARMAP;
    int16_t oi_tbl[XTRAMENU_NUM];
    const int x = XTRAMENU_POS_X, y = XTRAMENU_POS_Y;
    struct xtramenu_draw_s d;

    d.highlight = -1;

    ui_draw_copy_buf();
    hw_video_copy_back_to_page2();
    uiobj_finish_frame();
    ui_cursor_setup_area(1, &ui_cursor_area_tbl[0]);

    uiobj_table_clear();
    {
        int y0 = y + 5;
        for (int i = 0; i < XTRAMENU_NUM; ++i) {
            oi_tbl[i] = uiobj_add_mousearea(x, y0, x + 100, y0 + 7, xtramenu[i].key);
            y0 += 8;
        }
    }

    uiobj_set_callback_and_delay(xtramenu_draw_cb, &d, 1);

    while (!flag_done) {
        int16_t oi, oi2;
        oi = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if (oi == UIOBJI_ESC) {
            ui_sound_play_sfx_06();
            flag_done = true;
        }
        d.highlight = -1;
        for (int i = 0; i < XTRAMENU_NUM; ++i) {
            if (oi2 == oi_tbl[i]) {
                d.highlight = i;
            }
            if (oi == oi_tbl[i]) {
                ui_sound_play_sfx_24();
                flag_done = true;
                act = xtramenu[i].act;
                if (xtramenu[i].cb) {
                    xtramenu[i].cb(g, pi);
                }
                break;
            }
        }
        if (!flag_done) {
            xtramenu_draw_cb(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(1);
        }
    }

    uiobj_unset_callback();
    uiobj_table_clear();
    hw_video_copy_back_from_page2();
    uiobj_finish_frame();
    return act;
}
