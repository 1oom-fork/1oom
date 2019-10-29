#include "config.h"

#include <stdio.h>

#include "uigmap.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_stat.h"
#include "game_str.h"
#include "hw.h"
#include "kbd.h"
#include "lbx.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "ui.h"
#include "uidelay.h"
#include "uidefs.h"
#include "uidraw.h"
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "uistarmap_common.h"
#include "uiplanets.h"
#include "uiswitch.h"

/* -------------------------------------------------------------------------- */

#define GMAP_UI_LIMITS  7, 7, 230, 191
#define GMAP_TEXT_LIMITS  7 * ui_scale, 7 * ui_scale, 230 * ui_scale, 191 * ui_scale
#define GMAP_LIMITS s->glx0, s->gly0, s->glx1, s->gly1

struct gmap_blink_data_s {
    struct game_s *g;
    uint8_t planet_i;
    int countdown;
};

struct gmap_scale_data_s {
    int scale;
    int sx, sy;
    int xoffs, yoffs;
    int xmax, ymax;
    int glx0, gly0, glx1, gly1;
};

struct gmap_data_s {
    const struct game_s *g;
    struct gmap_blink_data_s b;
    struct gmap_scale_data_s s;
    player_id_t api;
    int16_t mode;
    uint8_t *gfx_mapview;
    uint8_t *gfx_but_col;
    uint8_t *gfx_but_env;
    uint8_t *gfx_but_min;
    uint8_t *gfx_but_ok;
    int hl_planet;
    int hl_race;
};

struct gmap_basic_data_s {
    struct game_s *g;
    struct gmap_blink_data_s b;
    struct gmap_scale_data_s s;
    bool show_switch;
};

/* stubs for future UI options */
static bool gmap_keep_aspect_ratio = true;
static bool gmap_extra_info = false;
static bool gmap_yellow_year = true;

static void gmap_init_scale(struct gmap_scale_data_s *s, const struct game_s *g, int w, int h, int bd) {
    s->xoffs = s->yoffs = 0;
    s->xmax = g->galaxy_maxx;
    s->ymax = g->galaxy_maxy;
    s->sx = s->sy = 0;
    if (gmap_keep_aspect_ratio) {
        if (s->xmax * h > s->ymax * w) {
            s->ymax = s->xmax * h / w;
            SETMAX(s->ymax, g->galaxy_maxy);
            s->yoffs = (s->ymax - g->galaxy_maxy) / 2;
        } else if (s->xmax * h < s->ymax * w) {
            s->xmax = s->ymax * w / h;
            SETMAX(s->xmax, g->galaxy_maxx);
            s->xoffs = (s->xmax - g->galaxy_maxx) / 2;
        }
    }
    SETRANGE(gmap_scale, 0, ui_scale);
    if (!gmap_scale) gmap_scale = starmap_scale;
    if (!gmap_scale) gmap_scale = ui_scale;
    /* the smaller interturn win size determines minimum scale */
    while (2 * gmap_scale * s->xmax < 215 * ui_scale) gmap_scale++;
    while (2 * gmap_scale * s->ymax < 171 * ui_scale) gmap_scale++;
    SETMIN(gmap_scale, ui_scale);
    /* limit gmap to starmap resolution (20 px/pc); add margins if necessary */
    if (2 * gmap_scale * s->xmax < w * ui_scale) {
        s->xoffs += (w * ui_scale / gmap_scale - 2 * s->xmax + 2) / 4;
        s->xmax = (w * ui_scale + gmap_scale) / (2 * gmap_scale);
        s->sx = LBXGFX_SCALE;
    }
    if (2 * gmap_scale * s->ymax < h * ui_scale) {
        s->yoffs += (h * ui_scale / gmap_scale - 2 * s->ymax + 2) / 4;
        s->ymax = (h * ui_scale + gmap_scale) / (2 * gmap_scale);
        s->sy = LBXGFX_SCALE;
    }
    if (!s->sx) s->sx = w * ui_scale * LBXGFX_SCALE / (2 * gmap_scale * s->xmax);
    if (!s->sy) s->sy = h * ui_scale * LBXGFX_SCALE / (2 * gmap_scale * s->ymax);
    SETMIN(s->sx, LBXGFX_SCALE);
    SETMIN(s->sy, LBXGFX_SCALE);
    s->glx0 = s->gly0 = (bd * ui_scale + gmap_scale - 1) / gmap_scale; /* round up left and upper limit*/
    s->glx1 = (bd + w) * ui_scale / gmap_scale - 1;                    /* round down right and lower limit */
    s->gly1 = (bd + h) * ui_scale / gmap_scale - 1;
}

static void gmap_load_data(struct gmap_data_s *d)
{
    d->gfx_mapview = lbxfile_item_get(LBXFILE_STARMAP, 0x30);
    d->gfx_but_col = lbxfile_item_get(LBXFILE_STARMAP, 0x31);
    d->gfx_but_env = lbxfile_item_get(LBXFILE_STARMAP, 0x32);
    d->gfx_but_min = lbxfile_item_get(LBXFILE_STARMAP, 0x33);
    d->gfx_but_ok = lbxfile_item_get(LBXFILE_STARMAP, 0x34);
}

static void gmap_free_data(struct gmap_data_s *d)
{
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_mapview);
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_but_col);
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_but_env);
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_but_min);
    lbxfile_item_release(LBXFILE_STARMAP, d->gfx_but_ok);
}

static void gmap_blink_step(struct gmap_blink_data_s *b)
{
    if (b->countdown < 0) {
        struct game_s *g = b->g;
        b->planet_i = rnd_0_nm1(g->galaxy_stars, &ui_data.seed);
        b->countdown = 3;
    } else {
        --b->countdown;
    }
}

#define SX1(x) ((224 * ((x) + s->xoffs) / s->xmax + 7) * ui_scale / gmap_scale)
#define SY1(y) ((185 * ((y) + s->yoffs) / s->ymax + 7) * ui_scale / gmap_scale)
#define UX1(x) (224 * ((x) + s->xoffs) / s->xmax + 7)
#define UY1(y) (185 * ((y) + s->yoffs) / s->ymax + 7)

#define GM_TAG_PLAYER 15
#define GM_TAG_NAME   16
#define GM_TAG_EVENT  32
#define GM_TAG_ATK    64
#define GM_TAG_INV   128

static void gmap_draw_cb(void *vptr)
{
    struct gmap_data_s *d = vptr;
    const struct game_s *g = d->g;
    const struct gmap_scale_data_s *s = &d->s;
    player_id_t tbl_races[PLAYER_NUM];
    uint8_t tbl_tags[PLANETS_MAX];
    uint32_t tbl_fthres[PLAYER_NUM];
    int racesnum = 1;

    tbl_races[0] = d->api;
    tbl_fthres[d->api] = game_weight_fleet(g, d->api);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if ((i != d->api) && IS_ALIVE(g, i)) {
            tbl_races[racesnum++] = i;
            tbl_fthres[i] = (game_weight_fleet(g, i) + tbl_fthres[d->api]) / 10;
        }
    }
    tbl_fthres[d->api] /= 5;

    uiobj_set_limits(GMAP_UI_LIMITS);
    ui_draw_erase_buf();
    lbxgfx_draw_frame(0, 0, ui_data.gfx.starmap.sky, UI_SCREEN_W, ui_scale);
    lbxgfx_draw_frame(0, 0, d->gfx_mapview, UI_SCREEN_W, ui_scale);

    for (int i = 0; i < g->nebula_num; ++i) {
        int x = g->nebula_x[i], y = g->nebula_y[i];
        uint8_t *gfx = ui_data.gfx.starmap.nebula[i];
        lbxgfx_draw_frame_scale(SX1(x), SY1(y), s->sx, s->sy, gfx, UI_SCREEN_W, gmap_scale);
    }

    if (ui_extra_enabled) {
        int x0 = 225 * ui_scale / gmap_scale;
        int y0 = 187 * ui_scale / gmap_scale;
        int x5 = x0 - ((50 * s->sx) >> LBXGFX_SLD);
        int y5 = y0 - ((50 * s->sy) >> LBXGFX_SLD);
        int col = gmap_keep_aspect_ratio ? 0x44 : 0;
        ui_draw_box1(x0 - 2, y0 - 2, x0 + 2, y0 + 2, 6, 4, gmap_scale);
        ui_draw_line1(x0 -2, y0, x5, y0, 6, gmap_scale);
        ui_draw_line1(x0, y0 - 2, x0, y5, 6, gmap_scale);
        ui_draw_line1(x5, y0 + 1, x5, y0 -1, 6, gmap_scale);
        ui_draw_line1(x0 + 1, y5, x0 - 1, y5, 6, gmap_scale);
        ui_draw_filled_rect(x0 - 1, y0 - 1, x0 + 1, y0 + 1, col, gmap_scale);
        lbxfont_select(0, 7, 0, 0);
        lbxfont_print_str_center(x5 - 4, y0 - 2, "5", UI_SCREEN_W, gmap_scale);
        lbxfont_print_str_center(x0, y5 - 7, "5", UI_SCREEN_W, gmap_scale);
    }

    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        if (BOOLVEC_IS1(r->visible, d->api)) {
            uint8_t *gfx = ui_data.gfx.starmap.tinyship[g->eto[r->owner].banner];
            int w = game_weight_ships(g, r->owner, r->ships);
            if (d->mode == 0 && gmap_extra_info && g->eto[d->api].have_ia_scanner) {
                const planet_t *p = &g->planet[r->dest];
                int col = tbl_banner_color[g->eto[r->owner].banner];
                if (0
                  || (d->hl_planet == r->dest)
                  || (d->hl_race >= 0 &&  tbl_races[d->hl_race] == r->owner)
                  || (d->hl_planet < 0 && d->hl_race < 0 && p->owner == d->api && r->owner != d->api)
                  || (d->hl_planet < 0 && d->hl_race < 0 && w >= tbl_fthres[r->owner])
                ) {
                    ui_draw_line1(SX1(r->x), SY1(r->y) + 1, SX1(p->x) + 3, SY1(p->y) + 3, col, gmap_scale);
                }
            }
            lbxgfx_draw_frame_offs(SX1(r->x), SY1(r->y), gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
            if (gmap_extra_info && w >= tbl_fthres[r->owner]) {
                lbxgfx_draw_frame_offs(SX1(r->x)+1, SY1(r->y), gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
            }
        }
    }

    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &(g->transport[i]);
        if (BOOLVEC_IS1(r->visible, d->api)) {
            uint8_t *gfx = ui_data.gfx.starmap.tinytran[g->eto[r->owner].banner];
            if (d->mode == 0 && gmap_extra_info && g->eto[d->api].have_ia_scanner) {
                const planet_t *p = &g->planet[r->dest];
                int col = tbl_banner_color[g->eto[r->owner].banner];
                if (0
                  || (d->hl_planet == r->dest)
                  || (d->hl_race >= 0 &&  tbl_races[d->hl_race] == r->owner)
                  || (d->hl_planet < 0 && d->hl_race < 0 && p->owner == d->api && r->owner != d->api)
                ) {
                    ui_draw_line1(SX1(r->x), SY1(r->y) + 1, SX1(p->x) + 3, SY1(p->y) + 3, col, gmap_scale);
                }
            }
            lbxgfx_draw_frame_offs(SX1(r->x), SY1(r->y), gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
        }
    }

    if (ui_extra_enabled && !gmap_yellow_year) {
        char buf[32];
        lib_sprintf(buf, sizeof(buf), "Year %i", g->year+2299);
        lbxfont_select(0, 6, 0, 0);
        lbxfont_set_color_c_n(0, 5);
        lbxfont_print_str_normal(9, 9, buf, UI_SCREEN_W, ui_scale);
        lbxfont_set_color_c_n(194, 5);
        lbxfont_print_str_normal(8, 8, buf, UI_SCREEN_W, ui_scale);
    }

    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if (BOOLVEC_IS1(p->within_srange, d->api)) {
            player_id_t tbl_have_orbit_owner[PLAYER_NUM];
            bool tbl_have_orbit_sod[PLAYER_NUM];
            int have_orbit_num;

            have_orbit_num = 0;
            for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                const fleet_orbit_t *r = &(g->eto[j].orbit[i]);
                int w = game_weight_ships(g, j, r->ships);
                if (w) {
                    tbl_have_orbit_sod[have_orbit_num] = (w >= tbl_fthres[j]);
                    tbl_have_orbit_owner[have_orbit_num++] = j;
                }
            }

            for (int j = 0; j < have_orbit_num; ++j) {
                uint8_t *gfx = ui_data.gfx.starmap.tinyship[g->eto[tbl_have_orbit_owner[j]].banner];
                lbxgfx_draw_frame_offs(SX1(p->x)+7, SY1(p->y) + 1 + 2 * j, gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
                if (gmap_extra_info && tbl_have_orbit_sod[j]) {
                    lbxgfx_draw_frame_offs(SX1(p->x)+8, SY1(p->y) + 1 + 2 * j, gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
                }
            }
        }
        tbl_tags[i] = 0;
    }
    
    for (int i = 0; i < PLAYER_NUM; ++i) {
        int pi = g->evn.home[i];
        if (pi != PLANET_NONE) tbl_tags[g->evn.home[i]] = GM_TAG_NAME;
    }
    tbl_tags[g->evn.planet_orion_i] = GM_TAG_NAME;

    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        uint8_t *gfx;
        int x = SX1(p->x);
        int y = SY1(p->y);
        if (0
          || ((d->mode > 0) && BOOLVEC_IS1(p->explored, d->api))
          || ((d->mode == 0) && (p->type || !gmap_extra_info || BOOLVEC_IS0(p->explored, d->api)))
        ) {
            gfx = ui_data.gfx.starmap.smstars[p->star_type];
            if ((d->b.planet_i == i) && (d->b.countdown > 0)) {
                lbxgfx_set_new_frame(gfx, d->b.countdown);
            } else {
                lbxgfx_set_frame_0(gfx);
            }
            lbxgfx_draw_frame(x, y, gfx, UI_SCREEN_W, gmap_scale);
        } else {
            gfx = ui_data.gfx.starmap.smallstr;
            lbxgfx_set_frame_0(gfx);
            lbxgfx_draw_frame(x + 1, y + 1, gfx, UI_SCREEN_W, gmap_scale);
        }
        if (p->unrest == PLANET_UNREST_REBELLION) tbl_tags[i] = GM_TAG_EVENT;
        if (p->unrest == PLANET_UNREST_UNREST) tbl_tags[i] = GM_TAG_EVENT;
    }
    if (g->evn.have_plague) tbl_tags[g->evn.plague_planet_i] = GM_TAG_EVENT;
    if (g->evn.have_nova) tbl_tags[g->evn.nova_planet_i] = GM_TAG_EVENT;
    if (g->evn.have_comet) tbl_tags[g->evn.comet_planet_i] = GM_TAG_EVENT;
    if (g->evn.have_pirates) tbl_tags[g->evn.pirates_planet_i] = GM_TAG_EVENT;
    if (g->evn.have_accident) tbl_tags[g->evn.accident_planet_i] = GM_TAG_EVENT;

    if (g->eto[d->api].have_ia_scanner) {
        for (int j = 0; j < g->enroute_num; ++j) {
            const fleet_enroute_t *r = &(g->enroute[j]);
            player_id_t pown = g->planet[r->dest].owner;
            if (BOOLVEC_IS1(r->visible, d->api) && (pown != PLAYER_NONE) && (r->owner != pown)) {
                tbl_tags[r->dest] = (r->owner | GM_TAG_ATK);
            }
        }
        for (int j = 0; j < g->transport_num; ++j) {
            const transport_t *r = &(g->transport[j]);
            player_id_t pown = g->planet[r->dest].owner;
            if (BOOLVEC_IS1(r->visible, d->api) && (pown != PLAYER_NONE) && (r->owner != pown)) {
                tbl_tags[r->dest] = (r->owner | GM_TAG_INV);
            }
        }
    }

    if (d->hl_planet >= 0) tbl_tags[d->hl_planet] = GM_TAG_NAME;
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        uint8_t *gfx;
        player_id_t owner;
        int x = SX1(p->x);
        int y = SY1(p->y);
        owner = (BOOLVEC_IS1(p->within_srange, d->api)) ? p->owner : g->seen[d->api][i].owner;
        if (owner != PLAYER_NONE) {
            gfx = ui_data.gfx.starmap.smalflag[g->eto[owner].banner];
            lbxgfx_draw_frame(x + 2, y - 3, gfx, UI_SCREEN_W, gmap_scale);
        }
        switch (d->mode) {
            case 0:
                if (gmap_extra_info && tbl_tags[i]) {
                    if (0
                      || BOOLVEC_IS1(p->explored, d->api)
                      || BOOLVEC_IS1(p->within_srange, d->api)
                      || BOOLVEC_IS1(g->eto[d->api].contact, p->owner)
                      || (p->within_frange[d->api] == 1)
                    ) {
                        const char *str = 0;
                        if (tbl_tags[i] & GM_TAG_NAME) {
                            str = p->name;
                            lbxfont_select(2, d->hl_planet == i ? 4 : 7, 0, 0);
                        } else if(tbl_tags[i] & GM_TAG_EVENT) {
                            str = planets_get_notes_str(g, i, 0, 0, 0);
                            lbxfont_select(2, 9, 0, 0);
                        } else {
                            int col = g->eto[tbl_tags[i] & GM_TAG_PLAYER].banner;
                            str = (tbl_tags[i] & GM_TAG_ATK) ? "att" : "inv";
                            lbxfont_select(0, 6, 0, 0);
                            lbxfont_print_str_center_limit(x + 4, y + 8, str, GMAP_TEXT_LIMITS, UI_SCREEN_W, gmap_scale);
                            lbxfont_select(0, tbl_banner_fontparam[col], 0, 0);
                            lbxfont_select(0, p->owner == d->api ? 5 : 2, 0, 0);
                        }
                        lbxfont_print_str_center_limit(x + 3, y + 7, str, GMAP_TEXT_LIMITS, UI_SCREEN_W, gmap_scale);
                    }
                }
                break;
            case 1:
                if (BOOLVEC_IS1(p->explored, d->api)) {
                    char buf[2] = { 0, 0 };
                    int pt;
                    lbxfont_select_set_12_1(2, (p->type < g->eto[PLAYER_0].have_colony_for) ? 5 : 0xe, 0, 0);
                    pt = (PLANET_TYPE_TERRAN - p->type);
                    SETMAX(pt, 0);
                    buf[0] = game_str_gm_tchar[pt];
                    lbxfont_print_str_normal_limit(x + 7, y + 1, buf, GMAP_TEXT_LIMITS, UI_SCREEN_W, gmap_scale);
                }
                break;
            case 2:
                if (BOOLVEC_IS1(p->explored, d->api) && (p->special != PLANET_SPECIAL_NORMAL)) {
                    if (p->special > PLANET_SPECIAL_ARTIFACTS) {
                        lbxfont_select(2, 1, 0, 0);
                    } else {
                        lbxfont_select_set_12_1(2, (p->special != PLANET_SPECIAL_ARTIFACTS) ? 5 : 0xe, 0, 0);
                    }
                    if (p->special == PLANET_SPECIAL_4XTECH) {
                        lbxfont_select(2, 0xe, 0, 0);
                    }
                    lbxfont_print_str_center_limit(x + 2, y + 7, game_str_tbl_gm_spec[p->special], GMAP_TEXT_LIMITS, UI_SCREEN_W, gmap_scale);
                }
                break;
            default:
                break;
        }
    }
    switch (d->mode) {
        case 0:
            for (int i = 0; i < racesnum; ++i) {
                const empiretechorbit_t *e;
                e = &(g->eto[tbl_races[i]]);
                lbxgfx_draw_frame(245, 104 + 10 * i, ui_data.gfx.starmap.smalflag[e->banner], UI_SCREEN_W, ui_scale);
                lbxfont_select(0, d->hl_race == i ? tbl_banner_fontparam[e->banner] : 6, 0, 0);
                lbxfont_print_str_normal(260, 105 + 10 * i, game_str_tbl_race[e->race], UI_SCREEN_W, ui_scale);
            }
            if (ui_extra_enabled) {
                if (gmap_extra_info) {
                    lbxfont_select(0, 10, 0, 0);
                    if (g->eto[d->api].have_ia_scanner) {
                        lbxfont_print_str_normal(245, 165, "\002att\001ack", UI_SCREEN_W, ui_scale);
                        lbxfont_print_str_normal(280, 165, "\002inv\001asion", UI_SCREEN_W, ui_scale);
                    } else {
                        lbxfont_print_str_normal(245, 165, "\002no target scan", UI_SCREEN_W, ui_scale);
                    }
                } else {
                    lbxfont_select(2, 10, 0, 0);
                    lbxfont_print_str_normal(245, 165, "tactical info off", UI_SCREEN_W, ui_scale);
                }
            }
            break;
        case 1:
            for (int i = 0; i < 7; ++i) {
                char buf[2] = { 0, 0 };
                buf[0] = game_str_gm_tchar[i];
                lbxfont_select(2, ((PLANET_TYPE_TERRAN - i) < g->eto[d->api].have_colony_for) ? 5 : 0xe, 0, 0);
                lbxfont_print_str_normal(241, 105 + 7 * i, buf, UI_SCREEN_W, ui_scale);
                lbxfont_select(2, 6, 0, 0);
                lbxfont_print_str_normal(247, 105 + 7 * i, game_str_tbl_sm_pltype[13 - i], UI_SCREEN_W, ui_scale);
            }
            for (int i = 7; i < 14; ++i) {
                char buf[2] = { 0, 0 };
                const char *str;
                buf[0] = game_str_gm_tchar[i];
                lbxfont_select(2, ((PLANET_TYPE_TERRAN - i) < g->eto[d->api].have_colony_for) ? 5 : 0xe, 0, 0);
                lbxfont_print_str_normal(276, 105 + 7 * (i - 7), buf, UI_SCREEN_W, ui_scale);
                lbxfont_select(2, 6, 0, 0);
                if (i == 13) {
                    str = game_str_st_none2;
                } else {
                    str = game_str_tbl_sm_pltype[13 - i];
                }
                lbxfont_print_str_normal(282, 105 + 7 * (i - 7), str, UI_SCREEN_W, ui_scale);
            }
            lbxfont_print_str_normal(247, 160, game_str_gm_unable, UI_SCREEN_W, ui_scale);
            ui_draw_pixel(241, 161, 0x44, ui_scale);
            ui_draw_pixel(241, 162, 0x44, ui_scale);
            ui_draw_pixel(241, 163, 0x44, ui_scale);
            ui_draw_pixel(242, 161, 0x44, ui_scale);
            ui_draw_pixel(242, 162, 0x44, ui_scale);
            ui_draw_pixel(242, 163, 0x44, ui_scale);
            ui_draw_pixel(243, 161, 0x44, ui_scale);
            ui_draw_pixel(243, 162, 0x44, ui_scale);
            ui_draw_pixel(243, 163, 0x44, ui_scale);
            break;
        case 2:
            lbxfont_select_set_12_1(2, 5, 0, 0);
            lbxfont_print_str_normal(240, 103, game_str_tbl_sm_pspecial[0], UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(240, 111, game_str_tbl_sm_pspecial[1], UI_SCREEN_W, ui_scale);
            lbxfont_select_set_12_1(2, 1, 0, 0);
            lbxfont_print_str_normal(240, 119, game_str_tbl_sm_pspecial[4], UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(240, 127, game_str_tbl_sm_pspecial[5], UI_SCREEN_W, ui_scale);
            lbxfont_select_set_12_1(2, 0xe, 0, 0);
            lbxfont_print_str_normal(240, 135, game_str_tbl_sm_pspecial[3], UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(240, 143, game_str_tbl_gm_spec[6], UI_SCREEN_W, ui_scale);
            lbxfont_select(2, 6, 0, 0);
            lbxfont_print_str_normal(295, 103, game_str_gm_prod, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(295, 111, game_str_gm_prod, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(295, 119, game_str_gm_prod, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(295, 127, game_str_gm_prod, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(295, 135, game_str_gm_tech, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_normal(295, 143, game_str_gm_tech, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_right(292, 103, game_str_gm_1_3, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_right(292, 111, game_str_gm_1_2, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_right(295, 119, game_str_gm_2x, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_right(295, 127, game_str_gm_3x, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_right(295, 135, game_str_gm_2x, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_right(295, 143, game_str_gm_4x, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_center(275, 152, game_str_gm_prodb1, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_center(276, 159, game_str_gm_prodb2, UI_SCREEN_W, ui_scale);
            lbxfont_print_str_center(275, 166, game_str_gm_prodb3, UI_SCREEN_W, ui_scale);
            break;
        default:
            break;
    }

    lbxfont_set_temp_color(0x2b);
    lbxfont_select_set_12_4(4, 0xf, 0, 0);
    lbxfont_print_str_normal(242, 8, game_str_gm_gmap, UI_SCREEN_W, ui_scale);
    if (ui_extra_enabled && gmap_yellow_year) {
        char buf[16];
        lib_sprintf(buf, sizeof(buf), "Year %d", g->year + YEAR_BASE);
        lbxfont_print_str_normal(248, 89, buf, UI_SCREEN_W, ui_scale);
    } else {
        lbxfont_print_str_normal(250, 88, game_str_gm_mapkey, UI_SCREEN_W, ui_scale);
    }
    lbxfont_set_temp_color(0x00);
    gmap_blink_step(&(d->b));
    if (ui_extra_enabled) {
        const uint8_t ctblf[5] = { 0xbd, 0xbc, 0xbb, 0xba, 0xb9 };
        const uint8_t ctblb[5] = { 0xba, 0xbb, 0xbc, 0xbd, 0xb9 };
        int x0, y0, x1, y1, pos;
        x0 = SX1(ui_data.starmap.x);
        y0 = SY1(ui_data.starmap.y);
        x1 = SX1(ui_data.starmap.x + ((108 * ui_scale) / starmap_scale));
        SETMIN(x1, s->glx1);
        y1 = SY1(ui_data.starmap.y + ((86 * ui_scale) / starmap_scale));
        SETMIN(y1, s->gly1);
        pos = d->b.countdown + 1;
        ui_draw_line_ctbl(x0, y0, x0 + 4, y0, ctblf, 5, pos, gmap_scale);
        ui_draw_line_ctbl(x0, y0, x0, y0 + 4, ctblf, 5, pos, gmap_scale);
        ui_draw_line_ctbl(x1 - 1, y0, x1 - 4, y0, ctblb, 5, 4 - pos, gmap_scale);
        ui_draw_line_ctbl(x1, y0, x1, y0 + 4, ctblf, 5, pos, gmap_scale);
        ui_draw_line_ctbl(x0, y1, x0 + 4, y1, ctblf, 5, pos, gmap_scale);
        ui_draw_line_ctbl(x0, y1, x0, y1 - 4, ctblf, 5, pos, gmap_scale);
        ui_draw_line_ctbl(x1 - 1, y1, x1 - 4, y1, ctblb, 5, 4 - pos, gmap_scale);
        ui_draw_line_ctbl(x1, y1, x1, y1 - 4, ctblf, 5, pos, gmap_scale);
    } else {
        int x, y;
        x = UX1(ui_data.starmap.x);
        y = UY1(ui_data.starmap.y);
        lbxgfx_draw_frame(x, y, ui_data.gfx.starmap.bmap, UI_SCREEN_W, ui_scale);
    }
}

#define SX2(x) ((215 * ((x) + s->xoffs) / s->xmax + 6) * ui_scale / gmap_scale)
#define SY2(y) ((171 * ((y) + s->yoffs) / s->ymax + 6) * ui_scale / gmap_scale)
#define UX2(x) (215 * ((x) + s->xoffs) / s->xmax + 6)
#define UY2(y) (171 * ((y) + s->yoffs) / s->ymax + 6)

static void ui_gmap_basic_draw_galaxy(struct gmap_basic_data_s *d)
{
    const struct game_s *g = d->g;
    const struct gmap_scale_data_s *s = &d->s;
    ui_draw_filled_rect(6, 6, 221, 177, 0, ui_scale);
    /*uiobj_set_limits(6, 6, 221, 177);*/
    lbxgfx_draw_frame_offs(0, 0, ui_data.gfx.starmap.sky, 6, 6, 221, 177, UI_SCREEN_W, ui_scale);
    for (int i = 0; i < g->nebula_num; ++i) {
        int x = g->nebula_x[i], y = g->nebula_y[i];
        uint8_t *gfx = ui_data.gfx.starmap.nebula[i];
        lbxgfx_draw_frame_scale(SX2(x), SY2(y), s->sx, s->sy, gfx, UI_SCREEN_W, gmap_scale);
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        uint8_t *gfx;
        gfx = ui_data.gfx.starmap.smstars[p->star_type];
        if ((d->b.planet_i == i) && (d->b.countdown > 0)) {
            lbxgfx_set_new_frame(gfx, d->b.countdown);
        } else {
            lbxgfx_set_frame_0(gfx);
        }
        lbxgfx_draw_frame(SX2(p->x), SY2(p->y), gfx, UI_SCREEN_W, gmap_scale);
    }
}

/* -------------------------------------------------------------------------- */

bool ui_gmap(struct game_s *g, player_id_t active_player)
{
    struct gmap_data_s d;
    const struct gmap_scale_data_s *s = &d.s;
    bool flag_done = false, flag_do_focus = false;
    int16_t /*oi_col, oi_env, oi_min,*/ oi_ok, oi_tbl_planet[PLANETS_MAX];
    int16_t oi_ratio = UIOBJI_INVALID, oi_scale = UIOBJI_INVALID;
    int16_t oi_tactical = UIOBJI_INVALID, oi_year = UIOBJI_INVALID;
    int16_t oi_tbl_race[PLAYER_NUM], dummy;
    uint8_t val_scale;

    if (!ui_extra_enabled) {
        gmap_keep_aspect_ratio = false;
        gmap_extra_info = false;
        gmap_yellow_year = false;
    }
    for (int i = 0; i < PLAYER_NUM; ++i) oi_tbl_race[i] = UIOBJI_INVALID;
    gmap_load_data(&d);
    gmap_init_scale(&d.s, g, 224, 185, 7);
    val_scale = gmap_scale;
    d.g = g;
    d.api = active_player;
    d.mode = 0;
    d.b.g = g;
    d.b.countdown = -1;
    d.b.planet_i = 0;
    d.hl_planet = -1;
    d.hl_race = -1;
    

    uiobj_table_clear();

    oi_ok = uiobj_add_t0(246, 181, "", d.gfx_but_ok, MOO_KEY_SPACE);
    if (ui_extra_enabled) {
       oi_ratio = uiobj_add_mousearea(223, 185, 227, 189, MOO_KEY_r);
       oi_tactical = uiobj_add_mousearea(244, 164, 308, 170, MOO_KEY_t);
       oi_year = uiobj_add_mousearea(248, 88, 304, 98, MOO_KEY_y);
       oi_tbl_race[0] = uiobj_add_mousearea(244, 104, 290, 112, MOO_KEY_UNKNOWN);
       for (int i = PLAYER_0, j = 1; i < g->players; ++i) {
           if ((i != d.api) && IS_ALIVE(g, i)) {
               oi_tbl_race[j] = uiobj_add_mousearea(244, 104 + 10 * j, 308, 112 + 10 * j, MOO_KEY_UNKNOWN);
               ++j;
           }
        }
    }

    /*oi_col =*/ uiobj_add_t3(246, 27, "", d.gfx_but_col, &(d.mode), 0, MOO_KEY_c);
    /*oi_env =*/ uiobj_add_t3(246, 47, "", d.gfx_but_env, &(d.mode), 1, MOO_KEY_e);
    /*oi_min =*/ uiobj_add_t3(246, 67, "", d.gfx_but_min, &(d.mode), 2, MOO_KEY_m);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        int x, y, d = 6 * gmap_scale / ui_scale + 2;
        x = UX1(p->x);
        y = UY1(p->y);
        oi_tbl_planet[i] = uiobj_add_mousearea(x, y, x + d, y + d, MOO_KEY_UNKNOWN);
    }
    if (ui_scale > 1) oi_scale = uiobj_add_tb(7, 7, 224, 185, 1, 1, &dummy, &dummy, &val_scale, ui_scale);

    uiobj_set_help_id(9);
    uiobj_set_callback_and_delay(gmap_draw_cb, &d, 4);
    uiobj_set_downcount(1);

    while (!flag_done) {
        int16_t oi, oi2;
        oi = uiobj_handle_input_cond();
        oi2 = uiobj_at_cursor();
        ui_delay_prepare();
        if ((oi == oi_ok) || (oi == UIOBJI_ESC)) {
            flag_done = true;
        } else if(oi == oi_ratio) {
            gmap_keep_aspect_ratio = !gmap_keep_aspect_ratio;
            gmap_init_scale(&d.s, g, 224, 185, 7);
        } else if(oi == oi_scale) {
            gmap_scale = val_scale;
            SETRANGE(gmap_scale, 1, ui_scale);
            gmap_init_scale(&d.s, g, 224, 185, 7);
            val_scale = gmap_scale;
            oi = 0; /* no sound as in starmap */
        } else if(oi == oi_tactical) {
            gmap_extra_info = !gmap_extra_info;
        } else if(oi == oi_year) {
            gmap_yellow_year = !gmap_yellow_year;
        }
        if (oi != 0) {
            ui_sound_play_sfx_24();
        }
        d.hl_planet = -1;
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if (oi == oi_tbl_planet[i]) {
                g->planet_focus_i[active_player] = i;
                flag_do_focus = true;
                flag_done = true;
                break;
            }
            if (d.mode == 0 && gmap_extra_info && oi2 == oi_tbl_planet[i]) {
                d.hl_planet = i;
                break;
            }
        }
        d.hl_race = -1;
        for (int i = 0; i < g->players && oi_tbl_race[i] != UIOBJI_INVALID; ++i) {
            if (d.mode == 0 && gmap_extra_info && oi2 == oi_tbl_race[i]) {
                d.hl_race = i;
            }
        }
        if (!flag_done) {
            gmap_draw_cb(&d);
            ui_draw_finish();
            ui_delay_ticks_or_click(4);
        }
    }

    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    gmap_free_data(&d);
    return flag_do_focus;
}

void *ui_gmap_basic_init(struct game_s *g, bool show_player_switch)
{
    static struct gmap_basic_data_s ctx; /* HACK */
    ctx.g = g;
    ctx.b.g = g;
    ctx.b.countdown = -1;
    ctx.b.planet_i = 0;
    ctx.show_switch = show_player_switch;
    if (!show_player_switch) {
        ui_draw_copy_buf();
        ui_starmap_draw_button_text(0/*unused*/, false);
        hw_video_copy_back_to_page2();
    }
    gmap_init_scale(&ctx.s, g, 215, 171, 6);
    return &ctx;
}

void ui_gmap_basic_shutdown(void *ctx)
{
}

void ui_gmap_basic_start_player(void *ctx, int pi)
{
    struct gmap_basic_data_s *d = ctx;
    if (d->show_switch) {
        ui_switch_1(d->g, pi);
        ui_draw_copy_buf();
        hw_video_copy_back_to_page2();
    }
}

void ui_gmap_basic_start_frame(void *ctx, int pi)
{
    /*struct gmap_basic_data_s *d = ctx;*/
    ui_delay_prepare();
    hw_video_copy_back_from_page2();
}

void ui_gmap_basic_draw_frame(void *ctx, int pi/*player_i*/)
{
    struct gmap_basic_data_s *d = ctx;
    const struct gmap_scale_data_s *s = &d->s;
    const struct game_s *g = d->g;
    ui_gmap_basic_draw_galaxy(d);
    if (pi >= 0) {
        for (int i = 0; i < g->enroute_num; ++i) {
            const fleet_enroute_t *r = &(g->enroute[i]);
            if (BOOLVEC_IS1(r->visible, pi)) {
                uint8_t *gfx = ui_data.gfx.starmap.tinyship[g->eto[r->owner].banner];
                lbxgfx_draw_frame_offs(SX2(r->x), SY2(r->y), gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
            }
        }
        for (int i = 0; i < g->transport_num; ++i) {
            const transport_t *r = &(g->transport[i]);
            if (BOOLVEC_IS1(r->visible, pi)) {
                uint8_t *gfx = ui_data.gfx.starmap.tinytran[g->eto[r->owner].banner];
                lbxgfx_draw_frame_offs(SX2(r->x), SY2(r->y), gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
            }
        }
        for (int i = 0; i < g->galaxy_stars; ++i) {
            const planet_t *p = &(g->planet[i]);
            if (BOOLVEC_IS1(p->within_srange, pi)) {
                player_id_t tbl_have_orbit_owner[PLAYER_NUM];
                int have_orbit_num;
                have_orbit_num = 0;
                for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                    const fleet_orbit_t *r = &(g->eto[j].orbit[i]);
                    for (int k = 0; k < g->eto[j].shipdesigns_num; ++k) {
                        if (r->ships[k] != 0) {
                            tbl_have_orbit_owner[have_orbit_num++] = j;
                            break;
                        }
                    }
                }
                for (int j = 0; j < have_orbit_num; ++j) {
                    uint8_t *gfx = ui_data.gfx.starmap.tinyship[g->eto[tbl_have_orbit_owner[j]].banner];
                    lbxgfx_draw_frame_offs(SX2(p->x) + 7, SY2(p->y) + 1 + 2 * j, gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
                }
            }
        }
        for (int i = 0; i < 2; ++i) {
            const monster_t *r;
            r = (i == 0) ? &(g->evn.crystal) : &(g->evn.amoeba);
            if (r->exists && (r->killer == PLAYER_NONE)) {
                uint8_t *gfx = ui_data.gfx.planets.tmonster;
                lbxgfx_draw_frame_offs(SX2(r->x), SY2(r->y), gfx, GMAP_LIMITS, UI_SCREEN_W, ui_scale);
            }
        }
    }
    gmap_blink_step(&(d->b));
}

void ui_gmap_basic_draw_only(void *ctx, int pi/*planet_i*/)
{
    struct gmap_basic_data_s *d = ctx;
    const struct gmap_scale_data_s *s = &d->s;
    const struct game_s *g = d->g;
    ui_gmap_basic_draw_galaxy(d);
    {
        const planet_t *p = &(g->planet[pi]);
        player_id_t tbl_have_orbit_owner[PLAYER_NUM];
        int have_orbit_num;
        have_orbit_num = 0;
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            const fleet_orbit_t *r = &(g->eto[j].orbit[pi]);
            for (int k = 0; k < g->eto[j].shipdesigns_num; ++k) {
                if (r->ships[k] != 0) {
                    tbl_have_orbit_owner[have_orbit_num++] = j;
                    break;
                }
            }
        }
        for (int j = 0; j < have_orbit_num; ++j) {
            uint8_t *gfx = ui_data.gfx.starmap.tinyship[g->eto[tbl_have_orbit_owner[j]].banner];
            lbxgfx_draw_frame_offs(SX2(p->x) + 7, SY2(p->y) + 1 + 2 * j, gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
        }
        for (int i = 0; i < 2; ++i) {
            const monster_t *r;
            r = (i == 0) ? &(g->evn.crystal) : &(g->evn.amoeba);
            if (r->exists && (r->killer == PLAYER_NONE)) {
                uint8_t *gfx = ui_data.gfx.planets.tmonster;
                lbxgfx_draw_frame_offs(SX2(r->x), SY2(r->y), gfx, GMAP_LIMITS, UI_SCREEN_W, ui_scale);
            }
        }
    }
    gmap_blink_step(&(d->b));
}

void ui_gmap_basic_finish_frame(void *ctx, int pi)
{
    /*struct gmap_basic_data_s *d = ctx;*/
    ui_draw_finish();
    ui_delay_ticks_or_click(2);
}

void ui_gmap_draw_planet_border(void *ctx,const struct game_s *g, uint8_t planet_i)
{
    struct gmap_basic_data_s *d = ctx;
    const struct gmap_scale_data_s *s = &d->s;
    const planet_t *p = &(g->planet[planet_i]);
    int x, y;
    x = SX2(p->x) - 1;
    y = SY2(p->y) - 1;
    lbxgfx_draw_frame_offs(x , y , ui_data.gfx.starmap.slanbord, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
}

void ui_gmap_draw_planet_flag(void *ctx,const struct game_s *g, uint8_t planet_i)
{
    struct gmap_basic_data_s *d = ctx;
    const struct gmap_scale_data_s *s = &d->s;
    const planet_t *p = &(g->planet[planet_i]);
    uint8_t *gfx;
    int x, y;
    if (p->owner != PLAYER_NONE) {
        gfx = ui_data.gfx.starmap.smalflag[g->eto[p->owner].banner];
        x = SX2(p->x) + 2;
        y = SY2(p->y) - 3;
        lbxgfx_draw_frame_offs(x , y , gfx, GMAP_LIMITS, UI_SCREEN_W, gmap_scale);
    }
}

int16_t ui_gmap_add_planet_mousearea(void *ctx, const struct game_s *g, uint8_t planet_i)
{
    struct gmap_basic_data_s *d = ctx;
    const struct gmap_scale_data_s *s = &d->s;
    const planet_t *p = &(g->planet[planet_i]);
    int x, y;
    x = UX2(p->x);
    y = UY2(p->y);
    /* use same mouse area as in galaxy map */
    return uiobj_add_mousearea(x , y , x + 4, y + 4, MOO_KEY_UNKNOWN);
}
