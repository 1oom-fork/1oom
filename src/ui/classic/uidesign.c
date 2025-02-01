#include "config.h"

#include <stdio.h>
#include <string.h>

#include "uidesign.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_design.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_shiptech.h"
#include "game_str.h"
#include "game_tech.h"
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
#include "uiobj.h"
#include "uipal.h"
#include "uisound.h"
#include "util.h"
#include "vgabuf.h"

/* -------------------------------------------------------------------------- */

struct design_data_s {
    struct game_s *g;
    struct game_design_s *gd;
    player_id_t api;
    int16_t oi_tbl_weap[WEAPON_SLOT_NUM];
    int16_t oi_tbl_weap_up[WEAPON_SLOT_NUM];    /* + 1 */
    int16_t oi_tbl_weap_dn[WEAPON_SLOT_NUM];    /* + 2 */
    int16_t flag_tbl_weap_up[WEAPON_SLOT_NUM];  /* + 3 */
    int16_t flag_tbl_weap_dn[WEAPON_SLOT_NUM];  /* + 4 */
    int16_t oi_hmm04;
    int16_t oi_cancel;
    int16_t oi_build;
    int16_t oi_clear;
    int16_t oi_name;
    int16_t oi_idn;
    int16_t oi_iup;
    int16_t oi_man;
    int16_t oi_comp;
    int16_t oi_jammer;
    int16_t oi_shield;
    int16_t oi_armor;
    int16_t oi_engine;
    int16_t oi_tbl_spec[SPECIAL_SLOT_NUM];
    int16_t oi_tbl_hull[SHIP_HULL_NUM];
    int16_t oi_icon;
    weapon_t last_avail_tech_weap;
    ship_special_t last_avail_tech_special;
    bool flag_disable_cspeed;
    bool flag_disable_comp;
    bool flag_disable_jammer;
    bool flag_disable_shield;
    bool flag_disable_armor;
    bool flag_disable_engine;
    bool flag_tbl_weapon[WEAPON_SLOT_NUM];
    bool flag_tbl_special[SPECIAL_SLOT_NUM];
    bool flag_tbl_hull[SHIP_HULL_NUM];
};

static const uint8_t colortbls_sd[] = {
    /*00*/ 0x45, 0x44, 0x42, 0x41, 0x40, 0x3f, 0x30, 0x26,
    /*08*/ 0x80, 0x00,
    /*0a*/ 0xbc, 0xb9, 0xb9,
    /*0d*/ 0xb6, 0xb6, 0xb6,
    /*10*/ 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
    /*15*/ 0xba, 0xba, 0xba, 0xba, 0xba,
    /*padding*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static uint8_t const * const colortbl_sd_hmm1 = &colortbls_sd[0x00];
static uint8_t const * const colortbl_sd_bc = &colortbls_sd[0x0a];
static uint8_t const * const colortbl_sd_b6 = &colortbls_sd[0x0d];
static uint8_t const * const colortbl_sd_bf = &colortbls_sd[0x10];
static uint8_t const * const colortbl_sd_ba = &colortbls_sd[0x15];

/* -------------------------------------------------------------------------- */

static void design_draw_cb(void *vptr)
{
    struct design_data_s *d = vptr;
    shipdesign_t *sd = &(d->gd->sd);
    int16_t oi;
    char buf[64];
    uint8_t extraman;

    ui_draw_filled_rect(5, 5, 315, 195, 1);
    lbxgfx_draw_frame(0, 0, ui_data.gfx.design.bg, UI_SCREEN_W);
    oi = uiobj_get_clicked_oi();
    lbxfont_select(0, 6, 0, 3);
    lbxfont_set_colors(((d->oi_comp == oi) || d->flag_disable_comp) ? colortbl_sd_ba : colortbl_sd_bf);
    lbxfont_print_str_normal(17, 22, game_str_sd_comp, UI_SCREEN_W);
    lbxfont_set_colors(((d->oi_shield == oi) || d->flag_disable_shield) ? colortbl_sd_ba : colortbl_sd_bf);
    lbxfont_print_str_normal(17, 34, game_str_sd_shield, UI_SCREEN_W);
    lbxfont_set_colors(((d->oi_jammer == oi) || d->flag_disable_jammer) ? colortbl_sd_ba : colortbl_sd_bf);
    lbxfont_print_str_normal(17, 46, game_str_sd_ecm, UI_SCREEN_W);
    lbxfont_set_colors(((d->oi_armor == oi) || d->flag_disable_armor) ? colortbl_sd_ba : colortbl_sd_bf);
    lbxfont_print_str_normal(167, 22, game_str_sd_armor, UI_SCREEN_W);
    lbxfont_set_colors(((d->oi_engine == oi) || d->flag_disable_engine) ? colortbl_sd_ba : colortbl_sd_bf);
    lbxfont_print_str_normal(167, 34, game_str_sd_engine, UI_SCREEN_W);
    lbxfont_set_colors(((d->oi_man == oi) || d->flag_disable_cspeed) ? colortbl_sd_ba : colortbl_sd_bf);
    lbxfont_print_str_normal(167, 46, game_str_sd_man, UI_SCREEN_W);
    for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
        lbxfont_set_colors(((d->oi_tbl_spec[i] == oi) || d->flag_tbl_special[i]) ? colortbl_sd_ba : colortbl_sd_bf);
        lbxfont_print_str_normal(17, 116 + i * 10, game_str_tbl_sd_spec[i], UI_SCREEN_W);
    }
    for (int i = 0; i < WEAPON_SLOT_NUM; ++i) {
        lbxfont_set_colors(((d->oi_tbl_weap[i] == oi) || d->flag_tbl_weapon[i]) ? colortbl_sd_ba : colortbl_sd_bf);
        lbxfont_print_str_normal(17, 71 + i * 10, game_str_tbl_sd_weap[i], UI_SCREEN_W);
    }

    lbxfont_select(0, 6, 0, 3);
    lbxfont_set_colors(colortbl_sd_bf);
    lbxfont_print_str_normal(59, 61, game_str_sd_count, UI_SCREEN_W);
    lbxfont_print_str_normal(88, 61, game_str_sd_sweap, UI_SCREEN_W);
    lbxfont_print_str_normal(163, 61, game_str_sd_damage, UI_SCREEN_W);
    lbxfont_print_str_normal(196, 61, game_str_sd_rng, UI_SCREEN_W);
    lbxfont_print_str_normal(215, 61, game_str_sd_notes, UI_SCREEN_W);
    lbxfont_select(0, 0, 0, 3);
    lbxfont_print_str_normal(262, 163, game_str_bc, UI_SCREEN_W);
    lbxfont_print_num_right(257, 163, sd->cost, UI_SCREEN_W);
    lbxfont_print_num_right(270, 175, game_design_get_hull_space(d->gd), UI_SCREEN_W);
    lbxfont_print_num_right(270, 187, sd->space, UI_SCREEN_W);

    {
        ship_hull_t acthull;
        acthull = sd->hull;
        for (ship_hull_t i = SHIP_HULL_SMALL; i < SHIP_HULL_NUM; ++i) {
            lbxfont_select(0, 0, 4, 7);
            if (d->flag_tbl_hull[i]) {
                lbxfont_select_subcolors_13not2();
                uiobj_ta_set_val_0(d->oi_tbl_hull[i]);
            } else {
                if (i != acthull) {
                    lbxfont_select_subcolors_13not1();
                }
                uiobj_ta_set_val_1(d->oi_tbl_hull[i]);
            }
            lbxfont_print_str_center(38, 163 + i * 7, *tbl_shiptech_hull[i].nameptr, UI_SCREEN_W);
        }
    }
    {
        ship_hull_t hull = sd->hull;
        uint8_t look, lookbase;
        lookbase = SHIP_LOOK_PER_HULL * hull + d->gd->lookbase;
        look = sd->look;
        if ((look < lookbase) || (look >= (lookbase + SHIP_LOOK_PER_HULL))) {
            sd->look = look = d->gd->tbl_shiplook_hull[hull];
        }
        lbxgfx_set_frame_0(ui_data.gfx.ships[look]);
        lbxgfx_draw_frame(93, 165, ui_data.gfx.ships[look], UI_SCREEN_W);
    }

    lbxfont_select(2, 8, 0, 8);
    lbxfont_set_colors(colortbl_sd_b6);
    for (int i = 0; i < WEAPON_SLOT_NUM; ++i) {
        weapon_t wi;
        wi = sd->wpnt[i];
        if (wi != WEAPON_NONE) {
            lbxfont_print_str_normal(88, 73 + i * 10, *tbl_shiptech_weap[wi].nameptr, UI_SCREEN_W);
        }
    }
    lbxfont_print_str_normal(200, 22, *tbl_shiptech_armor[sd->armor].nameptr, UI_SCREEN_W);
    lbxfont_print_str_normal(200, 34, *tbl_shiptech_engine[sd->engine].nameptr, UI_SCREEN_W);
    {
        uint8_t v;
        extraman = 0;
        for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
            v = tbl_shiptech_special[sd->special[i]].extraman;
            SETMAX(extraman, v);
        }
        v = extraman + sd->man + 1;
        lbxfont_print_num_right(220, 46, v, UI_SCREEN_W);
    }
    if (sd->comp) {
        lbxfont_print_str_normal(60, 22, *tbl_shiptech_comp[sd->comp].nameptr, UI_SCREEN_W);
    }
    if (sd->shield) {
        lbxfont_print_str_normal(60, 34, *tbl_shiptech_shield[sd->shield].nameptr, UI_SCREEN_W);
    }
    if (sd->jammer) {
        lbxfont_print_str_normal(60, 46, *tbl_shiptech_jammer[sd->jammer].nameptr, UI_SCREEN_W);
    }
    for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
        ship_special_t si;
        si = sd->special[i];
        if (si != SHIP_SPECIAL_NONE) {
            lbxfont_print_str_normal(61, 116 + i * 10, *tbl_shiptech_special[si].nameptr, UI_SCREEN_W);
        }
    }

    lbxfont_select(2, 8, 3, 2);
    lbxfont_set_colors(colortbl_sd_bc);
    for (int i = 0; i < WEAPON_SLOT_NUM; ++i) {
        weapon_t wi;
        wi = sd->wpnt[i];
        if (wi != WEAPON_NONE) {
            int y, dmin, dmax;
            y = 73 + i * 10;
            lbxfont_print_str_normal(215, y, *tbl_shiptech_weap[wi].extratextptr, UI_SCREEN_W);
            lbxfont_print_num_right(77, y, sd->wpnn[i], UI_SCREEN_W);
            lbxfont_print_num_right(207, y, tbl_shiptech_weap[wi].range, UI_SCREEN_W);
            dmin = tbl_shiptech_weap[wi].damagemin;
            dmax = tbl_shiptech_weap[wi].damagemax;
            if (dmin != dmax) {
                lbxfont_print_range_right(188, y, dmin, dmax, UI_SCREEN_W);
            } else {
                lbxfont_print_num_right(188, y, dmin, UI_SCREEN_W);
            }
        }
    }
    lbxfont_print_str_normal(250, 22, game_str_sd_hp, UI_SCREEN_W);
    lbxfont_print_num_right(302, 22, sd->hp, UI_SCREEN_W);
    lbxfont_print_str_normal(250, 34, game_str_sd_warp, UI_SCREEN_W);
    lbxfont_print_num_right(272, 34, tbl_shiptech_engine[sd->engine].warp, UI_SCREEN_W);
    lbxfont_print_str_normal(280, 34, game_str_sd_def, UI_SCREEN_W);
    lbxfont_print_str_normal(250, 46, game_str_sd_cspeed, UI_SCREEN_W);
    /* FIXME these calculations should be a function somewhere else */
    {
        uint8_t v;
        v = extraman + sd->man + tbl_shiptech_hull[sd->hull].defense + 1;
        lbxfont_print_num_right(302, 34, v, UI_SCREEN_W);
        v = (extraman / 2) + (sd->man + 3) / 2;
        lbxfont_print_num_right(302, 46, v, UI_SCREEN_W);
    }
    if (sd->shield) {
        uint8_t v = tbl_shiptech_shield[sd->shield].absorb;
        sprintf(buf, "%s %i %s", game_str_sd_absorbs, v, (v == 1) ? game_str_sd_hit : game_str_sd_hits);
        lbxfont_print_str_normal(101, 34, buf, UI_SCREEN_W);
    }
    lbxfont_print_str_normal(101, 46, game_str_sd_misdef, UI_SCREEN_W);
    {
        uint8_t v;
        v = extraman + sd->man + tbl_shiptech_jammer[sd->jammer].level + tbl_shiptech_hull[sd->hull].defense + 1;
        lbxfont_print_num_right(155, 46, v, UI_SCREEN_W);
    }
    lbxfont_print_str_normal(101, 22, game_str_sd_att, UI_SCREEN_W);
    {
        uint16_t v = 0;
        for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
            v |= tbl_shiptech_special[sd->special[i]].boolmask;
        }
        v = tbl_shiptech_comp[sd->comp].level + ((v & (1 << SHIP_SPECIAL_BOOL_SCANNER)) ? 1 : 0);
        lbxfont_print_num_right(155, 22, v, UI_SCREEN_W);
    }

    for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
        ship_special_t si;
        si = sd->special[i];
        if (si != SHIP_SPECIAL_NONE) {
            lbxfont_print_str_normal(172, 116 + i * 10, *tbl_shiptech_special[si].extratextptr, UI_SCREEN_W);
        }
    }
}

static void design_clear_ois(struct design_data_s *d)
{
    for (int i = 0; i < WEAPON_SLOT_NUM; ++i) {
        d->oi_tbl_weap[i] = UIOBJI_INVALID;
        d->oi_tbl_weap_up[i] = UIOBJI_INVALID;
        d->oi_tbl_weap_dn[i] = UIOBJI_INVALID;
        d->flag_tbl_weap_up[i] = 0;
        d->flag_tbl_weap_dn[i] = 0;
        d->flag_tbl_weapon[i] = false;
    }
    d->oi_hmm04 = UIOBJI_INVALID;
    d->oi_cancel = UIOBJI_INVALID;
    d->oi_build = UIOBJI_INVALID;
    d->oi_clear = UIOBJI_INVALID;
    d->oi_name = UIOBJI_INVALID;
    d->oi_idn = UIOBJI_INVALID;
    d->oi_iup = UIOBJI_INVALID;
    d->oi_man = UIOBJI_INVALID;
    d->oi_comp = UIOBJI_INVALID;
    d->oi_jammer = UIOBJI_INVALID;
    d->oi_shield = UIOBJI_INVALID;
    d->oi_armor = UIOBJI_INVALID;
    d->oi_engine = UIOBJI_INVALID;
    for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
        d->oi_tbl_spec[i] = UIOBJI_INVALID;
    }
    for (int i = 0; i < SHIP_HULL_NUM; ++i) {
        d->oi_tbl_hull[i] = UIOBJI_INVALID;
    }
    d->oi_icon = UIOBJI_INVALID;
}

static void design_init_ois(struct design_data_s *d)
{
    lbxfont_select(0, 1, 0, 3);
    d->oi_iup = uiobj_add_t0(143, 162, "", ui_data.gfx.design.icon_up, MOO_KEY_UNKNOWN, -1);
    d->oi_idn = uiobj_add_t0(143, 179, "", ui_data.gfx.design.icon_dn, MOO_KEY_UNKNOWN, -1);
    d->oi_icon = uiobj_add_mousearea(89, 161, 128, 192, MOO_KEY_UNKNOWN, -1);
    lbxfont_select(2, 6, 0, 3);
    d->oi_cancel = uiobj_add_t0(282, 150, game_str_sd_cancel, ui_data.gfx.design.blank, MOO_KEY_ESCAPE, -1);
    d->oi_build = uiobj_add_t0(282, 182, game_str_sd_build, ui_data.gfx.design.blank, MOO_KEY_b, -1);
    d->oi_clear = uiobj_add_t0(282, 166, game_str_sd_clear, ui_data.gfx.design.blank, MOO_KEY_c, -1);
    lbxfont_select(0, 0, 5, 3);
    d->oi_name = uiobj_add_textinput(214, 151, 56, d->gd->sd.name, SHIP_NAME_LEN - 1, 1, true, 0, colortbl_sd_hmm1, MOO_KEY_UNKNOWN, -1);
    uiobj_dec_y1(d->oi_name);
    SETMIN(d->gd->sd.man, d->gd->sd.engine);
    d->oi_man = uiobj_add_mousearea(167, 45, 305, 52, MOO_KEY_UNKNOWN, -1);
    d->oi_comp = uiobj_add_mousearea(17, 21, 155, 28, MOO_KEY_UNKNOWN, -1);
    d->oi_jammer = uiobj_add_mousearea(17, 45, 155, 52, MOO_KEY_UNKNOWN, -1);
    d->oi_shield = uiobj_add_mousearea(17, 33, 155, 48, MOO_KEY_UNKNOWN, -1);
    d->oi_armor = uiobj_add_mousearea(167, 21, 305, 28, MOO_KEY_UNKNOWN, -1);
    d->oi_engine = uiobj_add_mousearea(167, 33, 305, 40, MOO_KEY_UNKNOWN, -1);
    lbxfont_select(0, 0, 7, 5);
    for (int i = 0; i < SHIP_HULL_NUM; ++i) {
        const int y[SHIP_HULL_NUM] = { 162, 169, 176, 184 };
        d->oi_tbl_hull[i] = uiobj_add_mousearea(21, y[i], 57, y[i] + 7, MOO_KEY_UNKNOWN, -1);
    }
    for (int i = 0; i < WEAPON_SLOT_NUM; ++i) {
        d->oi_tbl_weap_dn[i] = uiobj_add_t2(59, i * 10 + 74, "", ui_data.gfx.design.count_dn, &d->flag_tbl_weap_dn[i], MOO_KEY_UNKNOWN, -1);
        d->oi_tbl_weap_up[i] = uiobj_add_t2(59, i * 10 + 69, "", ui_data.gfx.design.count_up, &d->flag_tbl_weap_up[i], MOO_KEY_UNKNOWN, -1);
        d->oi_tbl_weap[i] = uiobj_add_mousearea(16, i * 10 + 70, 305, i * 10 + 77, MOO_KEY_UNKNOWN, -1);
    }
    for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
        d->oi_tbl_spec[i] = uiobj_add_mousearea(17, 115 + i * 10, 305, 123 + i * 10, MOO_KEY_UNKNOWN, -1);
    }
}

static void design_update_haveflags(struct design_data_s *d)
{
    shipdesign_t *sd = &(d->gd->sd);
    SETRANGE(sd->man, 0, sd->engine);
    game_design_update_engines(sd);
    sd->cost = game_design_calc_cost(d->gd);
    sd->space = game_design_calc_space(d->gd);
    game_design_set_hp(sd);
    d->flag_disable_cspeed = true;
    if (sd->man < sd->engine) {
        ++sd->man;
        game_design_update_engines(sd);
        if (game_design_calc_space(d->gd) >= 0) {
            d->flag_disable_cspeed = false;
        }
        --sd->man;
    }
    d->flag_disable_comp = true;
    {
        ship_comp_t actcomp = sd->comp;
        ship_comp_t comp = actcomp;
        while (++comp < SHIP_COMP_NUM) {
            if (game_tech_player_has_tech(d->g, TECH_FIELD_COMPUTER, tbl_shiptech_comp[comp].tech_i, d->gd->player_i)) {
                sd->comp = comp;
                game_design_update_engines(sd);
                if (game_design_calc_space(d->gd) >= 0) {
                    d->flag_disable_comp = false;
                    break;
                }
            }
        }
        sd->comp = actcomp;
    }
    d->flag_disable_jammer = true;
    {
        ship_jammer_t actjammer = sd->jammer;
        ship_jammer_t jammer = actjammer;
        while (++jammer < SHIP_JAMMER_NUM) {
            if (game_tech_player_has_tech(d->g, TECH_FIELD_COMPUTER, tbl_shiptech_jammer[jammer].tech_i, d->gd->player_i)) {
                sd->jammer = jammer;
                game_design_update_engines(sd);
                if (game_design_calc_space(d->gd) >= 0) {
                    d->flag_disable_jammer = false;
                    break;
                }
            }
        }
        sd->jammer = actjammer;
    }
    d->flag_disable_shield = true;
    {
        ship_shield_t actshield = sd->shield;
        ship_shield_t shield = actshield;
        while (++shield < SHIP_SHIELD_NUM) {
            if (game_tech_player_has_tech(d->g, TECH_FIELD_FORCE_FIELD, tbl_shiptech_shield[shield].tech_i, d->gd->player_i)) {
                sd->shield = shield;
                game_design_update_engines(sd);
                if (game_design_calc_space(d->gd) >= 0) {
                    d->flag_disable_shield = false;
                    break;
                }
            }
        }
        sd->shield = actshield;
    }
    d->flag_disable_armor = true;
    {
        ship_armor_t actarmor = sd->armor;
        ship_armor_t armor = actarmor;
        while (++armor < SHIP_ARMOR_NUM) {
            if (game_tech_player_has_tech(d->g, TECH_FIELD_CONSTRUCTION, tbl_shiptech_armor[armor].tech_i, d->gd->player_i)) {
                sd->armor = armor;
                game_design_update_engines(sd);
                if (game_design_calc_space(d->gd) >= 0) {
                    d->flag_disable_armor = false;
                    break;
                }
            }
        }
        sd->armor = actarmor;
    }
    d->flag_disable_engine = true;
    {
        ship_engine_t actengine = sd->engine;
        ship_engine_t engine = actengine;
        while (++engine < SHIP_ENGINE_NUM) {
            if (game_tech_player_has_tech(d->g, TECH_FIELD_PROPULSION, tbl_shiptech_engine[engine].tech_i, d->gd->player_i)) {
                sd->engine = engine;
                game_design_update_engines(sd);
                if (game_design_calc_space(d->gd) >= 0) {
                    d->flag_disable_engine = false;
                    break;
                }
            }
        }
        sd->engine = actengine;
    }
    for (int i = 0; i < WEAPON_SLOT_NUM; ++i) {
        weapon_t wi, actwi;
        int wn;
        wi = actwi = sd->wpnt[i];
        wn = sd->wpnn[i];
        d->flag_tbl_weap_dn[i] = ((wn == 0) || (wi == WEAPON_NONE)) ? 1 : 0;
        d->flag_tbl_weap_up[i] = 1;
        if ((wi != WEAPON_NONE) && (wn < 99)) {
            sd->wpnn[i] = wn + 1;
            game_design_update_engines(sd);
            if (game_design_calc_space(d->gd) >= 0) {
                d->flag_tbl_weap_up[i] = 0;
            }
            sd->wpnn[i] = wn;
        }
        d->flag_tbl_weapon[i] = true;
        while (++wi <= d->last_avail_tech_weap) {
            tech_field_t fi;
            fi = tbl_shiptech_weap[wi].is_bio ? TECH_FIELD_PLANETOLOGY : TECH_FIELD_WEAPON;
            if (game_tech_player_has_tech(d->g, fi, tbl_shiptech_weap[wi].tech_i, d->gd->player_i)) {
                sd->wpnt[i] = wi;
                sd->wpnn[i] = 1;
                game_design_update_engines(sd);
                if (game_design_calc_space(d->gd) >= 0) {
                    d->flag_tbl_weapon[i] = false;
                    break;
                }
            }
        }
        sd->wpnt[i] = actwi;
        sd->wpnn[i] = wn;
    }
    for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
        ship_special_t otherspecial[SPECIAL_SLOT_NUM - 1];
        ship_special_t actsi, si;
        si = actsi = sd->special[i];
        {
            int j, n;
            for (j = 0, n = 0; j < SPECIAL_SLOT_NUM; ++j) {
                if (i != j) {
                    otherspecial[n++] = sd->special[j];
                }
            }
        }
        d->flag_tbl_special[i] = true;
        while (++si <= d->last_avail_tech_special) {
            bool flag_check;
            flag_check = true;
            for (int j = 0; j < SPECIAL_SLOT_NUM - 1; ++j) {
                ship_special_t osi;
                osi = otherspecial[j];
                if ((si == osi) || (tbl_shiptech_special[si].type == tbl_shiptech_special[osi].type)) {
                    flag_check = false;
                    break;
                }
            }
            if (flag_check && game_tech_player_has_tech(d->g, tbl_shiptech_special[si].field, tbl_shiptech_special[si].tech_i, d->gd->player_i)) {
                sd->special[i] = si;
                game_design_update_engines(sd);
                if (game_design_calc_space(d->gd) >= 0) {
                    d->flag_tbl_special[i] = false;
                    break;
                }
            }
        }
        sd->special[i] = actsi;
    }
    {
        ship_hull_t acthull;
        acthull = sd->hull;
        for (ship_hull_t i = SHIP_HULL_SMALL; i < SHIP_HULL_NUM; ++i) {
            sd->hull = i;
            game_design_update_engines(sd);
            d->flag_tbl_hull[i] = (game_design_calc_space(d->gd) < 0);
        }
        sd->hull = acthull;
    }
    game_design_update_engines(sd);
}

static void design_init_maxtech_haveflags(struct design_data_s *d)
{
    {
        weapon_t j = 0;
        for (weapon_t wi = WEAPON_NUCLEAR_BOMB; wi < WEAPON_NUM; ++wi) {
            tech_field_t fi;
            fi = tbl_shiptech_weap[wi].is_bio ? TECH_FIELD_PLANETOLOGY : TECH_FIELD_WEAPON;
            if (game_tech_player_has_tech(d->g, fi, tbl_shiptech_weap[wi].tech_i, d->gd->player_i)) {
                j = wi;
            }
        }
        d->last_avail_tech_weap = j;
    }
    {
        ship_special_t j = 0;
        for (ship_special_t si = SHIP_SPECIAL_RESERVE_FUEL_TANKS; si < SHIP_SPECIAL_NUM; ++si) {
            if (game_tech_player_has_tech(d->g, tbl_shiptech_special[si].field, tbl_shiptech_special[si].tech_i, d->gd->player_i)) {
                j = si;
            }
        }
        d->last_avail_tech_special = j;
    }
    design_update_haveflags(d);
}

struct xy_s {
    int x;
    int y;
};

static struct xy_s ui_design_draw_selbox(int xpos, int xoff1, int xoff2, int xoff3, int n, const char *str)
{
    struct xy_s xy;
    int x0, y0, x1, y1;

    x0 = (UI_SCREEN_W / 2 - 1) - (xpos + xoff1) / 2;
    x1 = x0 + xpos + 20;
    y0 = (UI_SCREEN_H / 2 - 1) - (n * 8 + 20) / 2;
    y1 = y0 + n * 8;
    SETMAX(x0, 0);
    SETMIN(x1, 159);
    SETMAX(y0, 0);

    ui_cursor_erase1(); /* HACK should not be needed */
    ui_draw_box_grain(x0 + 4, y0 + 4, x0 + xpos + xoff2, y1 + 20, 1, 2, 0x37);
    uiobj_set_limits(x0, y0, x1, y1);
    lbxgfx_draw_frame_offs(x0, y0, ui_data.gfx.design.pop1_ul, UI_SCREEN_W);
    uiobj_set_limits(x1, y0, UI_SCREEN_W - 1, y1);
    lbxgfx_draw_frame_offs(x0 + xpos + xoff3, y0, ui_data.gfx.design.pop1_ur, UI_SCREEN_W);
    uiobj_set_limits(x0, y1, x1, UI_SCREEN_H - 1);
    lbxgfx_draw_frame_offs(x0, y1, ui_data.gfx.design.pop1_dl, UI_SCREEN_W);
    uiobj_set_limits(x1, y1, UI_SCREEN_W - 1, UI_SCREEN_H - 1);
    lbxgfx_draw_frame_offs(x0 + xpos + xoff3, y1, ui_data.gfx.design.pop1_dr, UI_SCREEN_W);
    uiobj_set_limits_all();
    lbxgfx_draw_frame(118, y0 + 3, ui_data.gfx.design.titlebox, UI_SCREEN_W);
    lbxfont_select(0, 0xe, 0xe, 0xe);
    lbxfont_print_str_center(159, y0 + 5, str, UI_SCREEN_W);

    vgabuf_copy_back_to_page2();

    lbxfont_select(2, 0, 4, 0xe);
    uiobj_set_hmm8_0();

    xy.x = x0;
    xy.y = y0;
    return xy;
}

static void ui_design_sel_comp(struct design_data_s *d)
{
    int8_t havebuf[SHIP_COMP_NUM];
    bool flag_tbl_enable[SHIP_COMP_NUM];
    ship_comp_t tbl_comp[SHIP_COMP_NUM];
    int xpos, n = 0;
    int16_t curcomp;
    char titlebuf[0x80];
    char linebuf[SHIP_COMP_NUM * 0x50];
    const char *lineptr[SHIP_COMP_NUM + 1];
    shipdesign_t *sd = &(d->gd->sd);
    ship_comp_t actcomp = sd->comp;

    lbxfont_select(2, 0, 4, 0xe);

    {
        int havelast, space, cost, bufpos = 0;
        char s1[3] = "\x1dX";
        char s2[3] = "\x1dX";
        char s3[3] = "\x1dX";
        char s4[3] = "\x1dX";

        xpos = 0;
        for (int i = 0; i < SHIP_COMP_NUM; ++i) {
            int w;
            w = lbxfont_calc_str_width(*tbl_shiptech_comp[i].nameptr);
            SETMAX(xpos, w);
        }
        xpos += 20;

        s1[1] = (char)(xpos + 8);
        s2[1] = (char)(xpos + 33);
        s3[1] = (char)(xpos + 56);
        s4[1] = (char)(xpos + 85);

        sd->comp = 0;
        game_design_update_engines(sd);
        space = game_design_calc_space(d->gd);
        cost = game_design_calc_cost(d->gd);
        havelast = game_design_build_tbl_fit_comp(d->g, d->gd, havebuf);

        for (int i = 0; i <= havelast; ++i) {
            if (havebuf[i] >= 0) {
                int space2, power, cost2, sizei, len;
                flag_tbl_enable[n] = (havebuf[i] > 0);
                if (i == actcomp) {
                    curcomp = n;
                }
                tbl_comp[n] = i;
                sd->comp = i;
                game_design_update_engines(sd);
                space2 = space - game_design_calc_space(d->gd);
                power = tbl_shiptech_comp[i].power[sd->hull];
                cost2 = game_design_calc_cost(d->gd) - cost;
                sizei = game_design_calc_space_item(d->gd, DESIGN_SLOT_COMP, i);
                len = sprintf(&linebuf[bufpos], "%s%s%i%s%i%s%i%s%i", *tbl_shiptech_comp[i].nameptr, s1, cost2, s2, sizei, s3, power, s4, space2);
                lineptr[n] = &linebuf[bufpos];
                bufpos += len + 1;
                ++n;
            }
        }
        lineptr[n] = 0;
        sprintf(titlebuf, "%s%s%s%s%s%s%s%s%s", game_str_sd_comptype, s1, game_str_sd_cost, s2, game_str_sd_size, s3, game_str_sd_power, s4, game_str_sd_space);
    }
    {
        int listi;
        struct xy_s xy;
        xy = ui_design_draw_selbox(xpos, 130, 125, -30, n + 2, game_str_sd_comps);
        listi = uiobj_select_from_list1(xy.x + 14, xy.y + 20, xpos + 103, titlebuf, lineptr, &curcomp, flag_tbl_enable, 1, 0, 0x60, 0, 0, 0, -1);
        if (listi < 0) {
            sd->comp = actcomp;
        } else {
            sd->comp = tbl_comp[listi];
        }
        game_design_update_engines(sd);
    }
}

static void ui_design_sel_shield(struct design_data_s *d)
{
    int8_t havebuf[SHIP_SHIELD_NUM];
    bool flag_tbl_enable[SHIP_SHIELD_NUM];
    ship_shield_t tbl_shield[SHIP_SHIELD_NUM];
    int xpos, n = 0;
    int16_t curshield;
    char titlebuf[0x80];
    char linebuf[SHIP_SHIELD_NUM * 0x50];
    const char *lineptr[SHIP_SHIELD_NUM + 1];
    shipdesign_t *sd = &(d->gd->sd);
    ship_shield_t actshield = sd->shield;

    lbxfont_select(2, 0, 4, 0xe);

    {
        int havelast, space, cost, bufpos = 0;
        char s1[3] = "\x1dX";
        char s2[3] = "\x1dX";
        char s3[3] = "\x1dX";
        char s4[3] = "\x1dX";

        xpos = 0;
        for (int i = 0; i < SHIP_SHIELD_NUM; ++i) {
            int w;
            w = lbxfont_calc_str_width(*tbl_shiptech_shield[i].nameptr);
            SETMAX(xpos, w);
        }
        xpos += 20;

        s1[1] = (char)(xpos + 8);
        s2[1] = (char)(xpos + 33);
        s3[1] = (char)(xpos + 56);
        s4[1] = (char)(xpos + 85);

        sd->shield = 0;
        game_design_update_engines(sd);
        space = game_design_calc_space(d->gd);
        cost = game_design_calc_cost(d->gd);
        havelast = game_design_build_tbl_fit_shield(d->g, d->gd, havebuf);

        for (int i = 0; i <= havelast; ++i) {
            if (havebuf[i] >= 0) {
                int space2, power, cost2, sizei, len;
                flag_tbl_enable[n] = (havebuf[i] > 0);
                if (i == actshield) {
                    curshield = n;
                }
                tbl_shield[n] = i;
                sd->shield = i;
                game_design_update_engines(sd);
                space2 = space - game_design_calc_space(d->gd);
                power = tbl_shiptech_shield[i].power[sd->hull];
                cost2 = game_design_calc_cost(d->gd) - cost;
                sizei = game_design_calc_space_item(d->gd, DESIGN_SLOT_SHIELD, i);
                len = sprintf(&linebuf[bufpos], "%s%s%i%s%i%s%i%s%i", *tbl_shiptech_shield[i].nameptr, s1, cost2, s2, sizei, s3, power, s4, space2);
                lineptr[n] = &linebuf[bufpos];
                bufpos += len + 1;
                ++n;
            }
        }
        lineptr[n] = 0;
        sprintf(titlebuf, "%s%s%s%s%s%s%s%s%s", game_str_sd_shieldtype, s1, game_str_sd_cost, s2, game_str_sd_size, s3, game_str_sd_power, s4, game_str_sd_space);
    }
    {
        int listi;
        struct xy_s xy;
        xy = ui_design_draw_selbox(xpos, 130, 125, -30, n + 2, game_str_sd_shields);
        listi = uiobj_select_from_list1(xy.x + 14, xy.y + 20, xpos + 103, titlebuf, lineptr, &curshield, flag_tbl_enable, 1, 0, 0x60, 0, 0, 0, -1);
        if (listi < 0) {
            sd->shield = actshield;
        } else {
            sd->shield = tbl_shield[listi];
        }
        game_design_update_engines(sd);
    }
}

static void ui_design_sel_jammer(struct design_data_s *d)
{
    int8_t havebuf[SHIP_JAMMER_NUM];
    bool flag_tbl_enable[SHIP_JAMMER_NUM];
    ship_jammer_t tbl_jammer[SHIP_JAMMER_NUM];
    int xpos, n = 0;
    int16_t curjammer;
    char titlebuf[0x80];
    char linebuf[SHIP_JAMMER_NUM * 0x50];
    const char *lineptr[SHIP_SHIELD_NUM + 1];
    shipdesign_t *sd = &(d->gd->sd);
    ship_jammer_t actjammer = sd->jammer;

    lbxfont_select(2, 0, 4, 0xe);

    {
        int havelast, space, cost, bufpos = 0;
        char s1[3] = "\x1dX";
        char s2[3] = "\x1dX";
        char s3[3] = "\x1dX";
        char s4[3] = "\x1dX";

        xpos = 0;
        for (int i = 0; i < SHIP_JAMMER_NUM; ++i) {
            int w;
            w = lbxfont_calc_str_width(*tbl_shiptech_jammer[i].nameptr);
            SETMAX(xpos, w);
        }
        xpos += 20;

        s1[1] = (char)(xpos + 8);
        s2[1] = (char)(xpos + 33);
        s3[1] = (char)(xpos + 56);
        s4[1] = (char)(xpos + 85);

        sd->jammer = 0;
        game_design_update_engines(sd);
        space = game_design_calc_space(d->gd);
        cost = game_design_calc_cost(d->gd);
        havelast = game_design_build_tbl_fit_jammer(d->g, d->gd, havebuf);

        for (int i = 0; i <= havelast; ++i) {
            if (havebuf[i] >= 0) {
                int space2, power, cost2, sizei, len;
                flag_tbl_enable[n] = (havebuf[i] > 0);
                if (i == actjammer) {
                    curjammer = n;
                }
                tbl_jammer[n] = i;
                sd->jammer = i;
                game_design_update_engines(sd);
                space2 = space - game_design_calc_space(d->gd);
                power = tbl_shiptech_jammer[i].power[sd->hull];
                cost2 = game_design_calc_cost(d->gd) - cost;
                sizei = game_design_calc_space_item(d->gd, DESIGN_SLOT_JAMMER, i);
                len = sprintf(&linebuf[bufpos], "%s%s%i%s%i%s%i%s%i", *tbl_shiptech_jammer[i].nameptr, s1, cost2, s2, sizei, s3, power, s4, space2);
                lineptr[n] = &linebuf[bufpos];
                bufpos += len + 1;
                ++n;
            }
        }
        lineptr[n] = 0;
        sprintf(titlebuf, "%s%s%s%s%s%s%s%s%s", game_str_sd_ecmtype, s1, game_str_sd_cost, s2, game_str_sd_size, s3, game_str_sd_power, s4, game_str_sd_space);
    }
    {
        int listi;
        struct xy_s xy;
        xy = ui_design_draw_selbox(xpos, 130, 125, -30, n + 2, game_str_sd_ecm2);
        listi = uiobj_select_from_list1(xy.x + 14, xy.y + 20, xpos + 103, titlebuf, lineptr, &curjammer, flag_tbl_enable, 1, 0, 0x60, 0, 0, 0, -1);
        if (listi < 0) {
            sd->jammer = actjammer;
        } else {
            sd->jammer = tbl_jammer[listi];
        }
        game_design_update_engines(sd);
    }
}

static void ui_design_sel_armor(struct design_data_s *d)
{
    int8_t havebuf[SHIP_ARMOR_NUM];
    bool flag_tbl_enable[SHIP_ARMOR_NUM];
    ship_armor_t tbl_armor[SHIP_ARMOR_NUM];
    int xpos, n = 0;
    int16_t curarmor;
    char titlebuf[0x80];
    char linebuf[SHIP_ARMOR_NUM * 0x50];
    const char *lineptr[SHIP_ARMOR_NUM + 1];
    shipdesign_t *sd = &(d->gd->sd);
    ship_armor_t actarmor = sd->armor;

    lbxfont_select(2, 0, 4, 0xe);

    {
        int havelast, bufpos = 0;
        char s1[3] = "\x1dX";
        char s2[3] = "\x1dX";

        xpos = 0;
        for (int i = 0; i < SHIP_ARMOR_NUM; ++i) {
            int w;
            w = lbxfont_calc_str_width(*tbl_shiptech_armor[i].nameptr);
            SETMAX(xpos, w);
        }

        s1[1] = (char)(xpos + 25);
        s2[1] = (char)(xpos + 50);

        havelast = game_design_build_tbl_fit_armor(d->g, d->gd, havebuf);
        for (int i = 0; i <= havelast; ++i) {
            if (havebuf[i] >= 0) {
                int cost2, sizei, len;
                flag_tbl_enable[n] = (havebuf[i] > 0);
                if (i == actarmor) {
                    curarmor = n;
                }
                tbl_armor[n] = i;
                sd->armor = i;
                game_design_update_engines(sd);
                cost2 = game_design_calc_cost_item(d->gd, DESIGN_SLOT_ARMOR, i);
                sizei = game_design_calc_space_item(d->gd, DESIGN_SLOT_ARMOR, i);
                len = sprintf(&linebuf[bufpos], "%s%s%i%s%i", *tbl_shiptech_armor[i].nameptr, s1, cost2, s2, sizei);
                lineptr[n] = &linebuf[bufpos];
                bufpos += len + 1;
                ++n;
            }
        }
        lineptr[n] = 0;
        sprintf(titlebuf, "%s%s%s%s%s", game_str_sd_armortype, s1, game_str_sd_cost, s2, game_str_sd_size);
    }
    {
        int listi;
        struct xy_s xy;
        xy = ui_design_draw_selbox(xpos, 90, 85, -70, n + 2, game_str_sd_armor2);
        listi = uiobj_select_from_list1(xy.x + 14, xy.y + 20, xpos + 62, titlebuf, lineptr, &curarmor, flag_tbl_enable, 1, 0, 0x60, 0, 0, 0, -1);
        if (listi < 0) {
            sd->armor = actarmor;
        } else {
            sd->armor = tbl_armor[listi];
        }
        game_design_update_engines(sd);
    }
}

static void ui_design_sel_engine(struct design_data_s *d)
{
    int8_t havebuf[SHIP_ENGINE_NUM];
    bool flag_tbl_enable[SHIP_ENGINE_NUM];
    ship_engine_t tbl_engine[SHIP_ENGINE_NUM];
    int xpos, n = 0;
    int16_t curengine;
    char titlebuf[0x80];
    char linebuf[SHIP_ENGINE_NUM * 0x50];
    const char *lineptr[SHIP_ENGINE_NUM + 1];
    shipdesign_t *sd = &(d->gd->sd);
    ship_engine_t actengine = sd->engine;

    lbxfont_select(2, 0, 4, 0xe);

    {
        int havelast, bufpos = 0;
        char s1[3] = "\x1dX";
        char s2[3] = "\x1dX";
        char s3[3] = "\x1dX";
        char s4[3] = "\x1dX";

        xpos = 0;
        for (int i = 0; i < SHIP_ENGINE_NUM; ++i) {
            int w;
            w = lbxfont_calc_str_width(*tbl_shiptech_engine[i].nameptr);
            SETMAX(xpos, w);
        }

        s1[1] = (char)(xpos + 8);
        s2[1] = (char)(xpos + 33);
        s3[1] = (char)(xpos + 56);
        s4[1] = (char)(xpos + 105);

        sd->engine = 0;
        game_design_update_engines(sd);
        havelast = game_design_build_tbl_fit_engine(d->g, d->gd, havebuf);

        for (int i = 0; i <= havelast; ++i) {
            if (havebuf[i] >= 0) {
                int cost2, sizei, sizet, ne, len;
                flag_tbl_enable[n] = (havebuf[i] > 0);
                if (i == actengine) {
                    curengine = n;
                }
                tbl_engine[n] = i;
                sd->engine = i;
                game_design_update_engines(sd);
                ne = sd->engines;
                sizei = game_design_calc_space_item(d->gd, DESIGN_SLOT_ENGINE, i);
                sizet = (sizei * ne) / 10;
                cost2 = game_design_calc_cost_item(d->gd, DESIGN_SLOT_ENGINE, i);
                len = sprintf(&linebuf[bufpos], "%s%s%i%s%i%s%i.%i%s%i", *tbl_shiptech_engine[i].nameptr, s1, cost2, s2, sizei, s3, ne / 10, ne % 10, s4, sizet);
                lineptr[n] = &linebuf[bufpos];
                bufpos += len + 1;
                ++n;
            }
        }
        lineptr[n] = 0;
        sprintf(titlebuf, "%s%s%s%s%s%s%s%s%s", game_str_sd_engtype, s1, game_str_sd_cost, s2, game_str_sd_size, s3, game_str_sd_numengs, s4, game_str_sd_space);
    }
    {
        int listi;
        struct xy_s xy;
        xy = ui_design_draw_selbox(xpos, 155, 145, -7, n + 2, game_str_sd_engs);
        listi = uiobj_select_from_list1(xy.x + 14, xy.y + 20, xpos + 124, titlebuf, lineptr, &curengine, flag_tbl_enable, 1, 0, 0x60, 0, 0, 0, -1);
        if (listi < 0) {
            sd->engine = actengine;
        } else {
            sd->engine = tbl_engine[listi];
        }
        game_design_update_engines(sd);
    }
}

static void ui_design_sel_man(struct design_data_s *d)
{
    bool flag_tbl_enable[SHIP_ENGINE_NUM];
    int xpos, n = 0;
    int16_t curman;
    char titlebuf[0x80];
    char linebuf[SHIP_ENGINE_NUM * 0x50];
    const char *lineptr[SHIP_ENGINE_NUM + 1];
    shipdesign_t *sd = &(d->gd->sd);
    ship_engine_t actman = sd->man;

    lbxfont_select(2, 0, 4, 0xe);

    {
        int havelast = sd->engine, bufpos = 0, powperwarp, space, cost;
        char s1[3] = "\x1dX";
        char s2[3] = "\x1dX";
        char s3[3] = "\x1dX";
        char s4[3] = "\x1dX";

        xpos = 0;
        for (int i = 0; i <= havelast; ++i) {
            int w;
            sprintf(linebuf, "%s %s", game_str_sd_class, game_str_tbl_roman[i + 1]);
            w = lbxfont_calc_str_width(linebuf);
            SETMAX(xpos, w);
        }
        xpos += 5;

        s1[1] = (char)(xpos + 9);
        s2[1] = (char)(xpos + 38);
        s3[1] = (char)(xpos + 69);
        s4[1] = (char)(xpos + 99);

        sd->man = 0;
        curman = 0;
        game_design_update_engines(sd);
        powperwarp = tbl_shiptech_hull[sd->hull].power / tbl_shiptech_engine[sd->engine].warp;
        sd->engines = sd->engines - (powperwarp * 10) / tbl_shiptech_engine[sd->engine].power;
        space = game_design_calc_space(d->gd);
        cost = game_design_calc_cost(d->gd);

        for (int i = 0; i <= havelast; ++i) {
            int cost2, space2, spacet, len;
            sd->man = i;
            game_design_update_engines(sd);
            spacet = game_design_calc_space(d->gd);
            space2 = space - spacet;
            SETMAX(space2, 1);
            flag_tbl_enable[n] = (spacet >= 0);
            if (i == actman) {
                curman = n;
            }
            powperwarp = (tbl_shiptech_hull[sd->hull].power * (i + 1)) / tbl_shiptech_engine[sd->engine].warp;
            SETMAX(powperwarp, 1);
            cost2 = game_design_calc_cost(d->gd) - cost;
            SETMAX(cost2, 1);
            len = sprintf(&linebuf[bufpos], "%s %s%s%i%s%i%s%i%s%i", game_str_sd_class, game_str_tbl_roman[i + 1], s1, (i + 3) / 2, s2, cost2, s3, powperwarp, s4, space2);
            lineptr[n] = &linebuf[bufpos];
            bufpos += len + 1;
            ++n;
        }
        lineptr[n] = 0;
        sprintf(titlebuf, "%s%s%s%s%s%s%s%s%s", game_str_sd_man1, s1, game_str_sd_speed, s2, game_str_sd_cost, s3, game_str_sd_power, s4, game_str_sd_space);
    }
    {
        int listi;
        struct xy_s xy;
        xy = ui_design_draw_selbox(xpos, 150, 138, -15, n + 2, game_str_sd_man2);
        listi = uiobj_select_from_list1(xy.x + 14, xy.y + 20, xpos + 117, titlebuf, lineptr, &curman, flag_tbl_enable, 1, 0, 0x60, 0, 0, 0, -1);
        if (listi < 0) {
            sd->man = actman;
        } else {
            sd->man = curman;
        }
        game_design_update_engines(sd);
    }
}

static void ui_design_sel_weapon(struct design_data_s *d, int wslot)
{
    int8_t havebuf[WEAPON_NUM];
    bool flag_tbl_enable[WEAPON_NUM];
    weapon_t tbl_weapon[WEAPON_NUM];
    int xpos, xpos2, n = 0, numlines = 0;
    int16_t curweap;
    char titlebuf[0x80];
    char linebuf[WEAPON_NUM * 0x58];
    const char *lineptr[WEAPON_NUM + 1];
    shipdesign_t *sd = &(d->gd->sd);
    weapon_t actwpnt = sd->wpnt[wslot];
    uint8_t actwpnn = sd->wpnn[wslot];

    lbxfont_select(2, 0, 4, 0xe);

    {
        int havelast, bufpos = 0, space, cost;
        char s1[3] = "\x1dX";
        char s2[3] = "\x1dX";
        char s3[3] = "\x1dX";
        char s4[3] = "\x1dX";
        char s5[3] = "\x1dX";
        char s6[3] = "\x1dX";
        char s7[3] = "\x1dX";

        xpos = xpos2 = 0;
        for (int i = 0; i < WEAPON_NUM; ++i) {
            int w;
            w = lbxfont_calc_str_width(*tbl_shiptech_weap[i].nameptr);
            SETMAX(xpos, w);
            w = lbxfont_calc_str_width(*tbl_shiptech_weap[i].extratextptr);
            SETMAX(xpos2, w);
        }

        s1[1] = (char)(15);
        s2[1] = (char)(xpos + 19);
        xpos += xpos2 + 14;
        s3[1] = (char)(xpos + 12);
        s4[1] = (char)(xpos + 44);
        s5[1] = (char)(xpos + 65);
        s6[1] = (char)(xpos + 86);
        s7[1] = (char)(xpos + 95);

        sd->wpnt[wslot] = 0;
        sd->wpnn[wslot] = 0;
        game_design_update_engines(sd);
        space = game_design_calc_space(d->gd);
        cost = game_design_calc_cost(d->gd);
        havelast = game_design_build_tbl_fit_weapon(d->g, d->gd, havebuf, wslot);

        {
            int i = havelast + 1, firsti;
            if ((game_num_weapon_list_max > 0) && (game_num_weapon_list_max < WEAPON_NUM)) {
                int j;
                for (j = 0; (j < game_num_weapon_list_max) && (i > 0); ) {
                    --i;
                    if (havebuf[i] >= 0) {
                        ++j;
                    }
                }
                firsti = i;
                if (firsti > 0) {
                    i = 0;
                }
            } else {
                firsti = 0;
                i = 0;
            }
            while (i <= havelast) {
                if (havebuf[i] >= 0) {
                    int space2, cost2, power, sizei, len, dmin, dmax;
                    ++numlines;
                    sd->wpnt[wslot] = i;
                    sd->wpnn[wslot] = 1;
                    game_design_update_engines(sd);
                    space2 = space - game_design_calc_space(d->gd);
                    flag_tbl_enable[n] = (havebuf[i] > 0);
                    if ((i == actwpnt) && (actwpnn != 0)) {
                        curweap = n;
                    }
                    tbl_weapon[n] = i;
                    power = tbl_shiptech_weap[i].power;
                    cost2 = game_design_calc_cost(d->gd) - cost;
                    if (i == 0) {
                        havebuf[i] = 0;
                    }
                    sizei = game_design_calc_space_item(d->gd, DESIGN_SLOT_WEAPON1, i);
                    dmin = tbl_shiptech_weap[i].damagemin;
                    dmax = tbl_shiptech_weap[i].damagemax;
                    lineptr[n] = &linebuf[bufpos];
                    len = sprintf(&linebuf[bufpos], "%i%s%s%s%s%s%i", havebuf[i], s1, *tbl_shiptech_weap[i].nameptr, s2, *tbl_shiptech_weap[i].extratextptr, s3, dmin);
                    bufpos += len;
                    if (dmin != dmax) {
                        len = sprintf(&linebuf[bufpos], "-%i", dmax);
                        bufpos += len;
                    }
                    len = sprintf(&linebuf[bufpos], "%s%i%s%i%s%i%s      %i", s4, cost2, s5, sizei, s6, power, s7, space2);
                    bufpos += len + 1;
                    ++n;
                }
                ++i;
                if ((firsti > 0) && (i == 1)) {
                    i = firsti;
                }
            }
        }
        lineptr[n] = 0;
        sprintf(titlebuf, "%s %s%s%s%s%s%s%s%s%s%s%s%s      %s", game_str_sd_max, game_str_sd_weapname, s2, game_str_sd_descr, s3, game_str_sd_dmg, s4, game_str_sd_cost, s5, game_str_sd_size, s6, game_str_sd_power, s7, game_str_sd_space);
    }
    {
        int listi;
        struct xy_s xy;
        SETMIN(n, 18);
        xy = ui_design_draw_selbox(xpos, 160, 160, 0, n + 2, game_str_sd_weaps);
        if (numlines < 18) {
            listi = uiobj_select_from_list1(xy.x + 14, xy.y + 20, xpos + 131, titlebuf, lineptr, &curweap, flag_tbl_enable, 1, 0, 0x60, 0, 0, 0, -1);
        } else {
            listi = uiobj_select_from_list2(xy.x + 14, xy.y + 20, xpos + 131, titlebuf, lineptr, &curweap, flag_tbl_enable, 18, 312, 19, ui_data.gfx.design.popscrol_u, 313, 183, ui_data.gfx.design.popscrol_d, 1, 0, 0x60, 0, 0, 0, -1);
        }
        if (listi < 0) {
            sd->wpnt[wslot] = actwpnt;
            sd->wpnn[wslot] = actwpnn;
        } else {
            weapon_t wi = tbl_weapon[listi];
            uint8_t wn = havebuf[wi];
            sd->wpnt[wslot] = wi;
            SETMIN(wn, actwpnn);
            SETMAX(wn, 1);
            sd->wpnn[wslot] = wn;
        }
        game_design_update_engines(sd);
    }
}

static void ui_design_sel_special(struct design_data_s *d, int sslot)
{
    int8_t havebuf[SHIP_SPECIAL_NUM];
    bool flag_tbl_enable[SHIP_SPECIAL_NUM];
    ship_special_t tbl_special[SHIP_SPECIAL_NUM];
    int xpos, xpos2, n = 0, numlines = 0;
    int16_t curspec;
    char titlebuf[0x80];
    char linebuf[SHIP_SPECIAL_NUM * 0x5d];
    const char *lineptr[SHIP_SPECIAL_NUM + 1];
    shipdesign_t *sd = &(d->gd->sd);
    ship_special_t actspec = sd->special[sslot];

    lbxfont_select(2, 0, 4, 0xe);

    {
        int havelast, bufpos = 0, space, cost;
        char s1[3] = "\x1dX";
        char s2[3] = "\x1dX";
        char s3[3] = "\x1dX";
        char s4[3] = "\x1dX";
        char s5[3] = "\x1dX";

        xpos = xpos2 = 0;
        for (int i = 0; i < SHIP_SPECIAL_NUM; ++i) {
            int w;
            w = lbxfont_calc_str_width(*tbl_shiptech_special[i].nameptr);
            SETMAX(xpos, w);
            w = lbxfont_calc_str_width(*tbl_shiptech_special[i].extratextptr);
            SETMAX(xpos2, w);
        }

        s1[1] = (char)(xpos + 4);
        xpos += xpos2 + 4;
        s2[1] = (char)(xpos + 3);
        s3[1] = (char)(xpos + 22);
        s4[1] = (char)(xpos + 28);
        s5[1] = (char)(xpos + 30);

        sd->special[sslot] = 0;
        game_design_update_engines(sd);
        space = game_design_calc_space(d->gd);
        cost = game_design_calc_cost(d->gd);
        havelast = game_design_build_tbl_fit_special(d->g, d->gd, havebuf, sslot);

        for (int i = 0; i <= havelast; ++i) {
            if (havebuf[i] >= 0) {
                int space2, cost2, power, sizei, len;
                ++numlines;
                sd->special[sslot] = i;
                game_design_update_engines(sd);
                space2 = space - game_design_calc_space(d->gd);
                flag_tbl_enable[n] = (havebuf[i] > 0);
                if (i == actspec) {
                    curspec = n;
                }
                tbl_special[n] = i;
                power = tbl_shiptech_special[i].power[sd->hull];
                cost2 = game_design_calc_cost(d->gd) - cost;
                sizei = game_design_calc_space_item(d->gd, DESIGN_SLOT_SPECIAL1, i);
                lineptr[n] = &linebuf[bufpos];
                len = sprintf(&linebuf[bufpos], "%s%s%s%s", *tbl_shiptech_special[i].nameptr, s1, *tbl_shiptech_special[i].extratextptr, s2);
                bufpos += len;
                len = sprintf(&linebuf[bufpos], "%i%s%i%s    %i%s           %i", cost2, s3, sizei, s4, power, s5, space2);
                bufpos += len + 1;
                ++n;
            }
        }
        lineptr[n] = 0;
        sprintf(titlebuf, "%s%s%s%s%s%s%s%s    %s%s           %s", game_str_sd_specname, s1, game_str_sd_descr, s2, game_str_sd_cost, s3, game_str_sd_size, s4, game_str_sd_power, s5, game_str_sd_space);
    }
    {
        int listi;
        struct xy_s xy;
        SETMIN(n, 18);
        xy = ui_design_draw_selbox(xpos, 150, 152, -54, n + 2, game_str_sd_specs);
        if (numlines < 18) {
            listi = uiobj_select_from_list1(xy.x + 14, xy.y + 20, xpos + 81, titlebuf, lineptr, &curspec, flag_tbl_enable, 1, 0, 0x60, 0, 0, 0, -1);
        } else {
            listi = uiobj_select_from_list2(xy.x + 14, xy.y + 20, xpos + 81, titlebuf, lineptr, &curspec, flag_tbl_enable, 18, 312, 19, ui_data.gfx.design.popscrol_u, 313, 183, ui_data.gfx.design.popscrol_d, 1, 0, 0x60, 0, 0, 0, -1);
        }
        if (listi < 0) {
            sd->special[sslot] = actspec;
        } else {
            sd->special[sslot] = tbl_special[listi];
        }
        game_design_update_engines(sd);
    }
}

static void design_draw_sub_cb(void *vptr)
{
    vgabuf_copy_back_from_page2();
}

static void ui_design_sub(struct design_data_s *d, design_slot_t selmode)
{
    uiobj_unset_callback();
    uiobj_set_callback_and_delay(design_draw_sub_cb, 0, 1);
    switch (selmode) {
        case DESIGN_SLOT_COMP:
            ui_design_sel_comp(d);
            break;
        case DESIGN_SLOT_SHIELD:
            ui_design_sel_shield(d);
            break;
        case DESIGN_SLOT_JAMMER:
            ui_design_sel_jammer(d);
            break;
        case DESIGN_SLOT_ARMOR:
            ui_design_sel_armor(d);
            break;
        case DESIGN_SLOT_ENGINE:
            ui_design_sel_engine(d);
            break;
        case DESIGN_SLOT_MAN:
            ui_design_sel_man(d);
            break;
        case DESIGN_SLOT_WEAPON1:
        case DESIGN_SLOT_WEAPON2:
        case DESIGN_SLOT_WEAPON3:
        case DESIGN_SLOT_WEAPON4:
            ui_design_sel_weapon(d, selmode - DESIGN_SLOT_WEAPON1);
            break;
        case DESIGN_SLOT_SPECIAL1:
        case DESIGN_SLOT_SPECIAL2:
        case DESIGN_SLOT_SPECIAL3:
            ui_design_sel_special(d, selmode - DESIGN_SLOT_SPECIAL1);
            break;
        default:
            break;
    }
    uiobj_unset_callback();
    uiobj_table_clear();
    uiobj_set_help_id(-1);
    uiobj_set_callback_and_delay(design_draw_cb, d, 1);
    design_init_ois(d);
    design_update_haveflags(d);
    uiobj_set_help_id(5);
}

/* -------------------------------------------------------------------------- */

bool ui_design(struct game_s *g, struct game_design_s *gd, player_id_t active_player)
{
    struct design_data_s d;
    shipdesign_t *sd = &(gd->sd);
    bool flag_done = false, flag_ret = false, flag_copy = false, flag_name = false;

    d.g = g;
    d.gd = gd;
    d.api = active_player;

    /* design_hmma6 = a6; always 0 */

    design_clear_ois(&d);

    sd->look = gd->tbl_shiplook_hull[sd->hull];

    uiobj_table_clear();
    uiobj_set_xyoff(0, 0);

    design_init_ois(&d);
    design_init_maxtech_haveflags(&d);

    uiobj_set_help_id(5);
    uiobj_set_callback_and_delay(design_draw_cb, &d, 1);

    while (!flag_done) {
        int16_t oi;
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if (oi == d.oi_name) {
            ui_sound_play_sfx_24();
            util_trim_whitespace(sd->name);
            if (sd->name[0] == '\0') {
                strcpy(sd->name, gd->names[sd->hull]);
            }
            flag_name = true;
        }
        if (!flag_name) {
            strcpy(sd->name, gd->names[sd->hull]);
        }
        if ((oi == d.oi_cancel) || (oi == UIOBJI_ESC)) {
            ui_sound_play_sfx_06();
            flag_done = true;
        } else if (oi == d.oi_build) {
            ui_sound_play_sfx_24();
            flag_done = true;
            flag_ret = true;
        } else if (oi == d.oi_clear) {
            ui_sound_play_sfx_24();
            game_design_clear(gd);
            design_update_haveflags(&d);
        }
        for (ship_hull_t i = SHIP_HULL_SMALL; i < SHIP_HULL_NUM; ++i) {
            if ((oi == d.oi_tbl_hull[i]) && !d.flag_tbl_hull[i]) {
                ui_sound_play_sfx_24();
                sd->hull = i;
                sd->look = gd->tbl_shiplook_hull[i];
                design_update_haveflags(&d);
            }
        }
        for (int i = 0; i < WEAPON_SLOT_NUM; ++i) {
            if ((oi == d.oi_tbl_weap_up[i]) && (sd->wpnn[i] < 99)) {
                int space;
                ui_sound_play_sfx_24();
                ++sd->wpnn[i];
                game_design_update_engines(sd);
                space = game_design_calc_space(gd);
                if (space >= 0) {
                    sd->space = space;
                    design_update_haveflags(&d);
                } else {
                    --sd->wpnn[i];
                    game_design_update_engines(sd);
                }
                break;
            } else if ((oi == d.oi_tbl_weap_dn[i]) && (sd->wpnn[i] > 0)) {
                ui_sound_play_sfx_24();
                --sd->wpnn[i];
                design_update_haveflags(&d);
                d.flag_tbl_weap_dn[i] = (sd->wpnn[i] == 0);
                break;
            } else if (oi == d.oi_tbl_weap[i]) {
                ui_sound_play_sfx_24();
                ui_design_sub(&d, DESIGN_SLOT_WEAPON1 + i);
                break;
            }
        }
        if ((oi == d.oi_iup) || (oi == d.oi_icon)) {
            ui_sound_play_sfx_24();
            game_design_look_next(gd);
        } else if (oi == d.oi_idn) {
            ui_sound_play_sfx_24();
            game_design_look_prev(gd);
        } else if (oi == d.oi_armor) {
            ui_sound_play_sfx_24();
            ui_design_sub(&d, DESIGN_SLOT_ARMOR);
        } else if (oi == d.oi_shield) {
            ui_sound_play_sfx_24();
            ui_design_sub(&d, DESIGN_SLOT_SHIELD);
        } else if (oi == d.oi_man) {
            ui_sound_play_sfx_24();
            ui_design_sub(&d, DESIGN_SLOT_MAN);
        } else if (oi == d.oi_engine) {
            ui_sound_play_sfx_24();
            ui_design_sub(&d, DESIGN_SLOT_ENGINE);
        } else if (oi == d.oi_comp) {
            ui_sound_play_sfx_24();
            ui_design_sub(&d, DESIGN_SLOT_COMP);
        } else if (oi == d.oi_jammer) {
            ui_sound_play_sfx_24();
            ui_design_sub(&d, DESIGN_SLOT_JAMMER);
        }
        for (int i = 0; i < SPECIAL_SLOT_NUM; ++i) {
            if (oi == d.oi_tbl_spec[i]) {
                ui_sound_play_sfx_24();
                ui_design_sub(&d, DESIGN_SLOT_SPECIAL1 + i);
                break;
            }
        }
        design_draw_cb(&d);
        ui_palette_set_n();
        uiobj_finish_frame();
        if (!flag_copy) {
            ui_draw_copy_buf();
            flag_copy = true;
        }
        ui_delay_ticks_or_click(1);
    }
    game_design_compact_slots(sd);
    uiobj_table_clear();
    uiobj_unset_callback();
    uiobj_set_help_id(-1);
    return flag_ret;
}
