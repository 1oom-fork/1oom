#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bits.h"
#include "comp.h"
#include "game.h"
#include "game_str.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "save.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

bool libsave_is_moo13(const char *fname)
{
    uint8_t *buf = NULL;
    bool res = true;
    uint16_t w;
    int len;
    buf = lib_malloc(SAVE_MOO13_LEN);
    if (((len = util_file_try_load_len(fname, buf, SAVE_MOO13_LEN)) <= 0)
      || ((w = GET_LE_16(&buf[0xe2d2])) < 2) || (w > 6)
      || ((w = GET_LE_16(&buf[0xe2d4])) < 0) || (w > 4)
      || ((w = GET_LE_16(&buf[0xe2d6])) < 24) || (w > 108)
      || ((w = GET_LE_16(&buf[0xe2d8])) < 0) || (w > 3)
      || ((w = GET_LE_16(&buf[0xe238])) < 0) || (w > 4)
    ) {
        res = false;
    }
    lib_free(buf);
    buf = NULL;
    return res;
}

#define M13_GET_8(item_, addr_)     item_ = buf[addr_]
#define M13_GET_16(item_, addr_)    item_ = GET_LE_16(&buf[addr_])
#define M13_GET_32(item_, addr_)    item_ = GET_LE_32(&buf[addr_])
#define M13_GET_16_OWNER(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = GET_LE_16(&buf[addr_]); \
        if (t_ == 0xffff) { t_ = PLAYER_NONE; }; \
        item_ = t_; \
    } while (0)
#define M13_GET_16_KILLER(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = GET_LE_16(&buf[addr_]); \
        if (t_ == 0) { t_ = PLAYER_NONE; } else { --t_; } \
        item_ = t_; \
    } while (0)
#define M13_GET_16_CHECK(item_, addr_, l_, h_) \
    do { \
        int t_; \
        t_ = GET_LE_16(&buf[addr_]); \
        if ((t_ < l_) || (t_ > h_)) { \
            log_error( #item_ " at 0x%04x is %i and not in range %i..%i\n", addr_, t_, l_, h_); \
            return -1; \
        } \
        item_ = t_; \
    } while (0)
#define M13_GET_TBL_16(item_, addr_) \
    do { \
        for (size_t i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            item_[i_] = GET_LE_16(&buf[(addr_) + i_ * 2]); \
        } \
    } while (0)
#define M13_GET_TBL_16_OWNER(item_, addr_) \
    do { \
        for (size_t i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = GET_LE_16(&buf[(addr_) + i_ * 2]); \
            if (t_ == 0xffff) { t_ = PLAYER_NONE; }; \
            item_[i_] = t_; \
        } \
    } while (0)
#define M13_GET_TBL_16_HATED(item_, addr_) \
    do { \
        for (size_t i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = GET_LE_16(&buf[(addr_) + i_ * 2]); \
            if (t_ == 0) { t_ = PLAYER_NONE; } \
            item_[i_] = t_; \
        } \
    } while (0)
#define M13_GET_TBL_BVN_8(item_, addr_, n_) \
    do { \
        for (int i_ = 0; i_ < n_; ++i_) { \
            if (buf[(addr_) + i_]) { \
                BOOLVEC_SET1(item_, i_); \
            } \
        } \
    } while (0)
#define M13_GET_TBL_BV_16(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < PLAYER_NUM; ++i_) { \
            uint16_t t_; \
            t_ = GET_LE_16(&buf[(addr_) + i_ * 2]); \
            if (t_) { \
                BOOLVEC_SET1(item_, i_); \
            } \
        } \
    } while (0)
#define M13_GET_TBL_BVN_16(item_, addr_, n_) \
    do { \
        for (int i_ = 0; i_ < n_; ++i_) { \
            uint16_t t_; \
            t_ = GET_LE_16(&buf[(addr_) + i_ * 2]); \
            if (t_) { \
                BOOLVEC_SET1(item_, i_); \
            } \
        } \
    } while (0)
#define M13_GET_TBL_32(item_, addr_) \
    do { \
        for (size_t i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            item_[i_] = GET_LE_32(&buf[(addr_) + i_ * 4]); \
        } \
    } while (0)

static int libsave_moo13_de_sd(uint8_t *buf, shipdesign_t *sd, int sb)
{
    memcpy(sd->name, &buf[sb + 0x00], 11);
    M13_GET_16(sd->cost, sb + 0x14);
    M13_GET_16(sd->space, sb + 0x16);
    M13_GET_16(sd->hull, sb + 0x18);
    M13_GET_16(sd->look, sb + 0x1a);
    M13_GET_TBL_16(sd->wpnt, sb + 0x1c);
    M13_GET_TBL_16(sd->wpnn, sb + 0x24);
    M13_GET_16(sd->engine, sb + 0x2c);
    M13_GET_32(sd->engines, sb + 0x2e);
    M13_GET_TBL_16(sd->special, sb + 0x32);
    M13_GET_16(sd->shield, sb + 0x38);
    M13_GET_16(sd->jammer, sb + 0x3a);
    M13_GET_16(sd->comp, sb + 0x3c);
    M13_GET_16(sd->armor, sb + 0x3e);
    M13_GET_16(sd->man, sb + 0x40);
    M13_GET_16(sd->hp, sb + 0x42);
    return 0;
}

static int libsave_moo13_de_do(uint8_t *buf, struct game_s *g)
{
    {
        void *t = g->gaux;
        memset(g, 0, sizeof(*g));
        g->gaux = t;
    }
    M13_GET_16_CHECK(g->players, 0xe2d2, 2, 6);
    g->is_ai[0] = ((1 << g->players) - 1) & ~1;
    g->active_player = PLAYER_0;
    M13_GET_16_CHECK(g->difficulty, 0xe2d4, 0, 4);
    M13_GET_16_CHECK(g->galaxy_size, 0xe2d8, 0, 3);
    M13_GET_16_CHECK(g->nebula_num, 0xe238, 0, 4);
    M13_GET_16_CHECK(g->galaxy_stars, 0xe2d6, 24, 108);
    M13_GET_16(g->galaxy_w, 0xe2da);
    M13_GET_16(g->galaxy_h, 0xe2dc);
    M13_GET_16(g->galaxy_maxx, 0xe2de);
    M13_GET_16(g->galaxy_maxy, 0xe2e0);
    M13_GET_16(g->year, 0xe232);
    M13_GET_16_CHECK(g->enroute_num, 0xe1b6, 0, 259);
    M13_GET_16_CHECK(g->transport_num, 0xe1b8, 0, 99);
    M13_GET_16(g->end, 0xe686);
    M13_GET_16_OWNER(g->winner, 0xe688);
    M13_GET_16(g->guardian_killer, 0xe68a);
    if (g->guardian_killer == 1000) {
        g->guardian_killer = PLAYER_NONE;
    }
    M13_GET_16(g->election_held, 0xe68c);
    for (int i = 0; i < g->nebula_num; ++i) {
        M13_GET_16(g->nebula_type[i], 0xe23a + i * 2);
        M13_GET_16(g->nebula_x[i], 0xe242 + i * 2);
        M13_GET_16(g->nebula_y[i], 0xe24a + i * 2);
        for (int j = 0; j < 4; ++j) {
            M13_GET_16(g->nebula_x0[i][j], 0xe252 + (i * 4 + j) * 2);
            M13_GET_16(g->nebula_x1[i][j], 0xe272 + (i * 4 + j) * 2);
            M13_GET_16(g->nebula_y0[i][j], 0xe292 + (i * 4 + j) * 2);
            M13_GET_16(g->nebula_y1[i][j], 0xe2b2 + (i * 4 + j) * 2);
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        memcpy(g->emperor_names[i], &buf[0xe1ba + i * 15], EMPEROR_NAME_LEN - 1);
    }
    M13_GET_16(g->planet_focus_i[0], 0xe236);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        int pb;
        pb = i * 0xb8;
        memcpy(p->name, &buf[pb], PLANET_NAME_LEN - 1);
        M13_GET_16(p->x, pb + 0xc);
        M13_GET_16(p->y, pb + 0xe);
        M13_GET_16(p->star_type, pb + 0x10);
        M13_GET_16(p->look, pb + 0x12);
        M13_GET_16(p->frame, pb + 0x14);
        M13_GET_16(p->rocks, pb + 0x18);
        M13_GET_16(p->max_pop1, pb + 0x1a);
        M13_GET_16(p->max_pop2, pb + 0x1c);
        M13_GET_16(p->max_pop3, pb + 0x1e);
        M13_GET_16(p->type, pb + 0x20);
        M13_GET_16(p->battlebg, pb + 0x22);
        M13_GET_16(p->infogfx, pb + 0x24);
        M13_GET_16(p->growth, pb + 0x26);
        M13_GET_16(p->special, pb + 0x28);
        M13_GET_16(p->bc_to_ecoproj, pb + 0x2a);
        M13_GET_16(p->bc_to_ship, pb + 0x2c);
        M13_GET_16(p->bc_to_factory, pb + 0x2e);
        M13_GET_32(p->reserve, pb + 0x30);
        M13_GET_16(p->waste, pb + 0x34);
        M13_GET_16_OWNER(p->owner, pb + 0x36);
        M13_GET_16_OWNER(p->prev_owner, pb + 0x38);
        M13_GET_16_OWNER(p->claim, pb + 0xa0);
        M13_GET_16(p->pop, pb + 0x3a);
        M13_GET_16(p->pop_prev, pb + 0x3c);
        M13_GET_16(p->factories, pb + 0x3e);
        M13_GET_TBL_16(p->slider, pb + 0x50);
        M13_GET_TBL_16(p->slider_lock, pb + 0x72);
        M13_GET_16(p->buildship, pb + 0x5a);
        M13_GET_16(p->reloc, pb + 0x5c);
        M13_GET_16(p->missile_bases, pb + 0x5e);
        M13_GET_16(p->bc_to_base, pb + 0x60);
        M13_GET_16(p->bc_upgrade_base, pb + 0x62);
        M13_GET_16(p->have_stargate, pb + 0x64);
        M13_GET_16(p->shield, pb + 0x68);
        M13_GET_16(p->bc_to_shield, pb + 0x6a);
        M13_GET_16(p->trans_num, pb + 0x6c);
        M13_GET_16(p->trans_dest, pb + 0x6e);
        M13_GET_8(p->pop_tenths, pb + 0x70);
        M13_GET_TBL_BV_16(p->explored, pb + 0x7c);
        M13_GET_16(p->pop_oper_fact, pb + 0xae);
        M13_GET_16(p->bc_to_refit, pb + 0xb0);
        M13_GET_16(p->rebels, pb + 0xb2);
        M13_GET_16(p->unrest, pb + 0xb4);
        M13_GET_16(p->unrest_reported, pb + 0xb6);
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        seen_t *s = &(g->seen[PLAYER_0][i]);
        M13_GET_16_OWNER(s->owner, 0xe2e2 + i * 2);
        M13_GET_16(s->pop, 0xe3ba + i * 2);
        M13_GET_16(s->bases, 0xe492 + i * 2);
        M13_GET_16(s->factories, 0xe56a + i * 2);
    }
    for (player_id_t j = PLAYER_1; j < g->players; ++j) {
        for (int i = 0; i < g->galaxy_stars; ++i) {
            seen_t *s = &(g->seen[j][i]);
            s->owner = PLAYER_NONE;
        }
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        fleet_enroute_t *r = &(g->enroute[i]);
        int rb;
        rb = 0x4da0 + i * 0x1c;
        M13_GET_16_OWNER(r->owner, rb + 0x00);
        M13_GET_16(r->x, rb + 0x02);
        M13_GET_16(r->y, rb + 0x04);
        M13_GET_16(r->dest, rb + 0x06);
        M13_GET_8(r->speed, rb + 0x08);
        M13_GET_TBL_16(r->ships, rb + 0x0a);
    }
    for (int i = 0; i < g->transport_num; ++i) {
        transport_t *r = &(g->transport[i]);
        int rb;
        rb = 0x6a10 + i * 0x12;
        M13_GET_16_OWNER(r->owner, rb + 0x00);
        M13_GET_16(r->x, rb + 0x02);
        M13_GET_16(r->y, rb + 0x04);
        M13_GET_16(r->dest, rb + 0x06);
        M13_GET_8(r->speed, rb + 0x10);
        M13_GET_16(r->pop, rb + 0x08);
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e = &(g->eto[i]);
        int eb;
        eb = 0x7118 + i * 0xdd4;
        M13_GET_8(e->race, eb + 0x000);
        M13_GET_8(e->banner, eb + 0x002);
        M13_GET_8(e->trait1, eb + 0x004);
        M13_GET_8(e->trait2, eb + 0x006);
        M13_GET_16(e->ai_p3_countdown, eb + 0x008);
        M13_GET_16(e->ai_p2_countdown, eb + 0x00a);
        M13_GET_TBL_BV_16(e->within_frange, eb + 0x00c);
        M13_GET_TBL_16(e->relation1, eb + 0x018);
        M13_GET_TBL_16(e->relation2, eb + 0x024);
        M13_GET_TBL_16(e->diplo_type, eb + 0x030);
        M13_GET_TBL_16(e->diplo_val, eb + 0x03c);
        M13_GET_TBL_16(e->diplo_p1, eb + 0x048);
        M13_GET_TBL_16(e->diplo_p2, eb + 0x054);
        M13_GET_TBL_16(e->trust, eb + 0x06c);
        M13_GET_TBL_16(e->broken_treaty, eb + 0x078);
        M13_GET_TBL_16(e->blunder, eb + 0x084);
        M13_GET_TBL_16(e->tribute_field, eb + 0x090);
        M13_GET_TBL_16(e->tribute_tech, eb + 0x09c);
        M13_GET_TBL_16(e->mood_treaty, eb + 0x0a8);
        M13_GET_TBL_16(e->mood_trade, eb + 0x0b4);
        M13_GET_TBL_16(e->mood_tech, eb + 0x0c0);
        M13_GET_TBL_16(e->mood_peace, eb + 0x0cc);
        M13_GET_TBL_16(e->treaty, eb + 0x0d8);
        M13_GET_TBL_16(e->trade_bc, eb + 0x0e4);
        M13_GET_TBL_16(e->trade_percent, eb + 0x0f0);
        M13_GET_TBL_16(e->spymode_next, eb + 0x0fc);
        M13_GET_TBL_16(e->offer_field, eb + 0x1d4);
        M13_GET_TBL_16(e->offer_tech, eb + 0x1e0);
        M13_GET_TBL_16(e->offer_bc, eb + 0x1ec);
        M13_GET_TBL_16_HATED(e->hated, eb + 0x21c);
        M13_GET_TBL_16_HATED(e->mutual_enemy, eb + 0x228);
        M13_GET_TBL_16(e->hatred, eb + 0x270);
        M13_GET_TBL_16(e->have_met, eb + 0x27c);
        M13_GET_TBL_16(e->trade_established_bc, eb + 0x288);
        M13_GET_TBL_16(e->spying, eb + 0x2a4);
        M13_GET_TBL_16(e->spyfund, eb + 0x2b0);
        M13_GET_TBL_16(e->spymode, eb + 0x2c8);
        M13_GET_16(e->security, eb + 0x2d4);
        M13_GET_TBL_16(e->spies, eb + 0x2d6);
        M13_GET_32(e->reserve_bc, eb + 0x2fc);
        M13_GET_16(e->tax, eb + 0x300);
        M13_GET_16(e->base_shield, eb + 0x302);
        M13_GET_16(e->base_comp, eb + 0x304);
        M13_GET_16(e->base_weapon, eb + 0x306);
        M13_GET_16(e->colonist_oper_factories, eb + 0x326);
        M13_GET_TBL_16(e->tech.percent, eb + 0x332 + 0x00);
        M13_GET_TBL_16(e->tech.slider, eb + 0x332 + 0x0c);
        M13_GET_TBL_16(e->tech.slider_lock, eb + 0x332 + 0x60);
        M13_GET_TBL_32(e->tech.investment, eb + 0x332 + 0x18);
        M13_GET_TBL_16(e->tech.project, eb + 0x332 + 0x30);
        M13_GET_TBL_32(e->tech.cost, eb + 0x332 + 0x3c);
        M13_GET_TBL_16(e->tech.completed, eb + 0x332 + 0x54);
        M13_GET_16_CHECK(e->shipdesigns_num, eb + 0x3a0, 0, 6);
        for (int j = 0; j < g->galaxy_stars; ++j) {
            fleet_orbit_t *r = &(e->orbit[j]);
            int ob;
            ob = eb + 0x3a2 + j * 0x18;
            M13_GET_TBL_16(r->ships, ob + 0x0c);
        }
        M13_GET_TBL_16(g->eto[PLAYER_0].spyreportfield[i], eb + 0xdc2);
        M13_GET_16(g->eto[PLAYER_0].spyreportyear[i], eb + 0xdce);
        M13_GET_16(e->shipi_colony, eb + 0xdd0);
        M13_GET_16(e->shipi_bomber, eb + 0xdd2);
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        shipresearch_t *srd = &(g->srd[i]);
        int srdb, pos;
        srdb = 0xc410 + i * 0x468;
        for (int j = 0; j < g->eto[i].shipdesigns_num; ++j) {
            if (libsave_moo13_de_sd(buf, &(srd->design[j]), srdb + j * 0x44) != 0) {
                return -1;
            }
        }
        pos = srdb + 0x228;
        for (int f = 0; f < TECH_FIELD_NUM; ++f) {
            for (int t = 0; t < TECH_TIER_NUM; ++t) {
                for (int j = 0; j < 3; ++j) {
                    M13_GET_8(srd->researchlist[f][t][j], pos);
                    ++pos;
                }
            }
        }
        pos = srdb + 0x2dc;
        for (int f = 0; f < TECH_FIELD_NUM; ++f) {
            for (int j = 0; j < TECH_PER_FIELD; ++j) {
                M13_GET_8(srd->researchcompleted[f][j], pos);
                ++pos;
            }
        }
        M13_GET_TBL_16(srd->have_reserve_fuel, srdb + 0x444);
        M13_GET_TBL_16(srd->year, srdb + 0x450);
        /* M13_GET_TBL_16(srd->shipcount, srdb + 0x45c); */
    }
    if (libsave_moo13_de_sd(buf, &(g->current_design[PLAYER_0]), 0xe642) != 0) {
        return -1;
    }
    {
        gameevents_t *ev = &(g->evn);
        const int evb = 0xde80;
        M13_GET_16(ev->year, evb + 0x000);
        M13_GET_TBL_BVN_16(ev->done, evb + 0x004, 20);
        M13_GET_16(ev->have_plague, evb + 0x02c);
        M13_GET_16(ev->plague_player, evb + 0x02e);
        M13_GET_16(ev->plague_planet_i, evb + 0x030);
        M13_GET_16(ev->plague_val, evb + 0x032);
        M13_GET_16(ev->have_nova, evb + 0x03a);
        M13_GET_16(ev->nova_player, evb + 0x03c);
        M13_GET_16(ev->nova_planet_i, evb + 0x03e);
        M13_GET_16(ev->nova_years, evb + 0x040);
        M13_GET_16(ev->nova_val, evb + 0x042);
        M13_GET_16(ev->have_accident, evb + 0x044);
        M13_GET_16(ev->accident_planet_i, evb + 0x048);
        M13_GET_16(ev->have_comet, evb + 0x056);
        M13_GET_16(ev->comet_player, evb + 0x058);
        M13_GET_16(ev->comet_planet_i, evb + 0x05a);
        M13_GET_16(ev->comet_years, evb + 0x05c);
        M13_GET_16(ev->comet_hp, evb + 0x05e);
        M13_GET_16(ev->comet_dmg, evb + 0x060);
        M13_GET_16(ev->have_pirates, evb + 0x062);
        M13_GET_16(ev->pirates_planet_i, evb + 0x066);
        M13_GET_16(ev->pirates_hp, evb + 0x068);
        M13_GET_16(ev->crystal.exists, evb + 0x06e);
        M13_GET_16(ev->crystal.x, evb + 0x070);
        M13_GET_16(ev->crystal.y, evb + 0x072);
        M13_GET_16_KILLER(ev->crystal.killer, evb + 0x076);
        M13_GET_16(ev->crystal.dest, evb + 0x078);
        M13_GET_16(ev->crystal.counter, evb + 0x074);
        M13_GET_16(ev->crystal.nuked, evb + 0x07a);
        M13_GET_16(ev->amoeba.exists, evb + 0x07c);
        M13_GET_16(ev->amoeba.x, evb + 0x07e);
        M13_GET_16(ev->amoeba.y, evb + 0x080);
        M13_GET_16_KILLER(ev->amoeba.killer, evb + 0x084);
        M13_GET_16(ev->amoeba.dest, evb + 0x086);
        M13_GET_16(ev->amoeba.counter, evb + 0x082);
        M13_GET_16(ev->amoeba.nuked, evb + 0x088);
        M13_GET_8(ev->planet_orion_i, evb + 0x09c);
        M13_GET_8(ev->have_guardian, evb + 0x09e);
        M13_GET_TBL_16(ev->home, evb + 0x0a0);
        M13_GET_8(ev->report_stars, evb + 0x0ac);
        M13_GET_TBL_16(ev->new_ships[PLAYER_0], evb + 0x1a4);
        for (player_id_t i = PLAYER_1; i < g->players; ++i) {
            M13_GET_16(ev->spies_caught[i][PLAYER_0], evb + 0x1f2 + i * 2);
        }
        M13_GET_TBL_16(ev->spies_caught[PLAYER_0], evb + 0x1fe);
        M13_GET_TBL_16(ev->ceasefire[PLAYER_0], evb + 0x28e);
        M13_GET_TBL_BVN_8(ev->help_shown[PLAYER_0], evb + 0x2e2, 16);
        /* TODO build_finished ; is it even possible to save before clicking them away? */
        M13_GET_TBL_16_OWNER(ev->voted, evb + 0x320);
        M13_GET_16(ev->best_ecorestore[PLAYER_0], evb + 0x32c);
        M13_GET_16(ev->best_wastereduce[PLAYER_0], evb + 0x32e);
        M13_GET_16(ev->best_roboctrl[PLAYER_0], evb + 0x332);
        M13_GET_16(ev->best_terraform[PLAYER_0], evb + 0x334);
    }
    {
        uint8_t v = 0;
        int a = 0xe68e;
        for (int i = 0; i < g->galaxy_stars; ++i) {
            if ((i & 7) == 0) {
                M13_GET_8(v, a);
                ++a;
            }
            if (v & (1 << (i & 7))) {
                BOOLVEC_SET1(g->planet[i].finished, FINISHED_SHIP);
            }
        }
    }
    return 0;
}

int libsave_moo13_load_do(const char *filename, struct game_s *g)
{
    uint8_t *buf = NULL;
    int res = 0;
    int len;
    buf = lib_malloc(SAVE_MOO13_LEN);
    if ((len = util_file_try_load_len(filename, buf, SAVE_MOO13_LEN)) <= 0) {
        log_error("loading MOO1 v1.3 save '%s' (got %i != %i bytes)\n", filename, len, SAVE_MOO13_LEN);
        res = -1;
    } else if (libsave_moo13_de_do(buf, g) != 0) {
        res = -1;
    }
    lib_free(buf);
    buf = NULL;
    return res;
}

#define M13_SET_8(item_, addr_)     buf[addr_] = item_
#define M13_SET_16(item_, addr_)    SET_LE_16(&buf[addr_], item_)
#define M13_SET_32(item_, addr_)    SET_LE_32(&buf[addr_], item_)
#define M13_SET_16_OWNER(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = item_; \
        if (t_ == PLAYER_NONE) { t_ = 0xffff; }; \
        SET_LE_16(&buf[addr_], t_); \
    } while (0)
#define M13_SET_16_HATED(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = item_; \
        if (t_ == PLAYER_NONE) { t_ = 0; } \
        SET_LE_16(&buf[addr_], t_); \
    } while (0)
#define M13_SET_16_KILLER(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = item_; \
        if (t_ == PLAYER_NONE) { t_ = 0; } else { ++t_; } \
        SET_LE_16(&buf[addr_], t_); \
    } while (0)
#define M13_SET_16_CHECK(item_, addr_, l_, h_) \
    do { \
        uint16_t t_; \
        t_ = item_; \
        if ((t_ < l_) || (t_ > h_)) { \
            log_error( #item_ " is %i and not in range %i..%i\n", addr_, t_, l_, h_); \
            return -1; \
        } \
        SET_LE_16(&buf[addr_], t_); \
    } while (0)
#define M13_SET_TBL_16(item_, addr_) \
    do { \
        for (size_t i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = item_[i_]; \
            SET_LE_16(&buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_16_OWNER(item_, addr_) \
    do { \
        for (size_t i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = item_[i_]; \
            if (t_ == PLAYER_NONE) { t_ = 0xffff; }; \
            SET_LE_16(&buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_16_HATED(item_, addr_) \
    do { \
        for (size_t i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = item_[i_]; \
            if (t_ == PLAYER_NONE) { t_ = 0; } \
            SET_LE_16(&buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_BVN_8(item_, addr_, n_) \
    do { \
        for (int i_ = 0; i_ < n_; ++i_) { \
            uint8_t t_; \
            t_ = BOOLVEC_IS1(item_, i_); \
            buf[(addr_) + i_] = t_; \
        } \
    } while (0)
#define M13_SET_TBL_BV_16(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < PLAYER_NUM; ++i_) { \
            uint16_t t_; \
            t_ = BOOLVEC_IS1(item_, i_); \
            SET_LE_16(&buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_BVN_16(item_, addr_, n_) \
    do { \
        for (int i_ = 0; i_ < n_; ++i_) { \
            uint16_t t_; \
            t_ = BOOLVEC_IS1(item_, i_); \
            SET_LE_16(&buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_32(item_, addr_) \
    do { \
        for (size_t i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint32_t t_; \
            t_ = item_[i_]; \
            SET_LE_32(&buf[(addr_) + i_ * 4], t_); \
        } \
    } while (0)

static int libsave_moo13_en_sd(uint8_t *buf, const shipdesign_t *sd, int sb)
{
    memcpy(&buf[sb + 0x00], sd->name, 11);
    M13_SET_16(sd->cost, sb + 0x14);
    M13_SET_16(sd->space, sb + 0x16);
    M13_SET_16(sd->hull, sb + 0x18);
    M13_SET_16(sd->look, sb + 0x1a);
    M13_SET_TBL_16(sd->wpnt, sb + 0x1c);
    M13_SET_TBL_16(sd->wpnn, sb + 0x24);
    M13_SET_16(sd->engine, sb + 0x2c);
    M13_SET_32(sd->engines, sb + 0x2e);
    M13_SET_TBL_16(sd->special, sb + 0x32);
    M13_SET_16(sd->shield, sb + 0x38);
    M13_SET_16(sd->jammer, sb + 0x3a);
    M13_SET_16(sd->comp, sb + 0x3c);
    M13_SET_16(sd->armor, sb + 0x3e);
    M13_SET_16(sd->man, sb + 0x40);
    M13_SET_16(sd->hp, sb + 0x42);
    return 0;
}

static int libsave_moo13_en_do(uint8_t *buf, const struct game_s *g)
{
    memset(buf, 0, SAVE_MOO13_LEN);
    M13_SET_16(g->players, 0xe2d2);
    M13_SET_16(g->difficulty, 0xe2d4);
    M13_SET_16(g->galaxy_size, 0xe2d8);
    M13_SET_16(g->nebula_num, 0xe238);
    M13_SET_16(g->galaxy_stars, 0xe2d6);
    M13_SET_16(g->galaxy_w, 0xe2da);
    M13_SET_16(g->galaxy_h, 0xe2dc);
    M13_SET_16(g->galaxy_maxx, 0xe2de);
    M13_SET_16(g->galaxy_maxy, 0xe2e0);
    M13_SET_16(g->year, 0xe232);
    M13_SET_16_CHECK(g->enroute_num, 0xe1b6, 0, 259);
    M13_SET_16_CHECK(g->transport_num, 0xe1b8, 0, 99);
    M13_SET_16(g->end, 0xe686);
    M13_SET_16_OWNER(g->winner, 0xe688);
    M13_SET_16(g->election_held, 0xe68c);
    for (int i = 0; i < g->nebula_num; ++i) {
        M13_SET_16(g->nebula_type[i], 0xe23a + i * 2);
        M13_SET_16(g->nebula_x[i], 0xe242 + i * 2);
        M13_SET_16(g->nebula_y[i], 0xe24a + i * 2);
        for (int j = 0; j < 4; ++j) {
            M13_SET_16(g->nebula_x0[i][j], 0xe252 + (i * 4 + j) * 2);
            M13_SET_16(g->nebula_x1[i][j], 0xe272 + (i * 4 + j) * 2);
            M13_SET_16(g->nebula_y0[i][j], 0xe292 + (i * 4 + j) * 2);
            M13_SET_16(g->nebula_y1[i][j], 0xe2b2 + (i * 4 + j) * 2);
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        memcpy(&buf[0xe1ba + i * 15], g->emperor_names[i], EMPEROR_NAME_LEN - 1);
    }
    M13_SET_16(g->planet_focus_i[0], 0xe236);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        int pb;
        pb = i * 0xb8;
        memcpy(&buf[pb], p->name, PLANET_NAME_LEN - 1);
        M13_SET_16(p->x, pb + 0xc);
        M13_SET_16(p->y, pb + 0xe);
        M13_SET_16(p->star_type, pb + 0x10);
        M13_SET_16(p->look, pb + 0x12);
        M13_SET_16(p->frame, pb + 0x14);
        M13_SET_16(p->rocks, pb + 0x18);
        M13_SET_16(p->max_pop1, pb + 0x1a);
        M13_SET_16(p->max_pop2, pb + 0x1c);
        M13_SET_16(p->max_pop3, pb + 0x1e);
        M13_SET_16(p->type, pb + 0x20);
        M13_SET_16(p->battlebg, pb + 0x22);
        M13_SET_16(p->infogfx, pb + 0x24);
        M13_SET_16(p->growth, pb + 0x26);
        M13_SET_16(p->special, pb + 0x28);
        M13_SET_16(p->bc_to_ecoproj, pb + 0x2a);
        M13_SET_16(p->bc_to_ship, pb + 0x2c);
        M13_SET_16(p->bc_to_factory, pb + 0x2e);
        M13_SET_32(p->reserve, pb + 0x30);
        M13_SET_16(p->waste, pb + 0x34);
        M13_SET_16_OWNER(p->owner, pb + 0x36);
        M13_SET_16_OWNER(p->prev_owner, pb + 0x38);
        M13_SET_16_OWNER(p->claim, pb + 0xa0);
        M13_SET_16(p->pop, pb + 0x3a);
        M13_SET_16(p->pop_prev, pb + 0x3c);
        M13_SET_16(p->factories, pb + 0x3e);
        M13_SET_TBL_16(p->slider, pb + 0x50);
        M13_SET_TBL_16(p->slider_lock, pb + 0x72);
        M13_SET_16(p->buildship, pb + 0x5a);
        M13_SET_16(p->reloc, pb + 0x5c);
        M13_SET_16(p->missile_bases, pb + 0x5e);
        M13_SET_16(p->bc_to_base, pb + 0x60);
        M13_SET_16(p->bc_upgrade_base, pb + 0x62);
        M13_SET_16(p->have_stargate, pb + 0x64);
        M13_SET_16(p->shield, pb + 0x68);
        M13_SET_16(p->bc_to_shield, pb + 0x6a);
        M13_SET_16(p->trans_num, pb + 0x6c);
        M13_SET_16(p->trans_dest, pb + 0x6e);
        M13_SET_8(p->pop_tenths, pb + 0x70);
        M13_SET_TBL_BV_16(p->explored, pb + 0x7c);
        M13_SET_16(p->pop_oper_fact, pb + 0xae);
        M13_SET_16(p->bc_to_refit, pb + 0xb0);
        M13_SET_16(p->rebels, pb + 0xb2);
        M13_SET_16(p->unrest, pb + 0xb4);
        M13_SET_16(p->unrest_reported, pb + 0xb6);
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const seen_t *s = &(g->seen[PLAYER_0][i]);
        M13_SET_16_OWNER(s->owner, 0xe2e2 + i * 2);
        M13_SET_16(s->pop, 0xe3ba + i * 2);
        M13_SET_16(s->bases, 0xe492 + i * 2);
        M13_SET_16(s->factories, 0xe56a + i * 2);
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        int rb;
        rb = 0x4da0 + i * 0x1c;
        M13_SET_16_OWNER(r->owner, rb + 0x00);
        M13_SET_16(r->x, rb + 0x02);
        M13_SET_16(r->y, rb + 0x04);
        M13_SET_16(r->dest, rb + 0x06);
        M13_SET_8(r->speed, rb + 0x08);
        M13_SET_TBL_16(r->ships, rb + 0x0a);
    }
    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &(g->transport[i]);
        int rb;
        rb = 0x6a10 + i * 0x12;
        M13_SET_16_OWNER(r->owner, rb + 0x00);
        M13_SET_16(r->x, rb + 0x02);
        M13_SET_16(r->y, rb + 0x04);
        M13_SET_16(r->dest, rb + 0x06);
        M13_SET_8(r->speed, rb + 0x10);
        M13_SET_16(r->pop, rb + 0x08);
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        const empiretechorbit_t *e = &(g->eto[i]);
        int eb;
        eb = 0x7118 + i * 0xdd4;
        M13_SET_8(e->race, eb + 0x000);
        M13_SET_8(e->banner, eb + 0x002);
        M13_SET_8(e->trait1, eb + 0x004);
        M13_SET_8(e->trait2, eb + 0x006);
        M13_SET_16(e->ai_p3_countdown, eb + 0x008);
        M13_SET_16(e->ai_p2_countdown, eb + 0x00a);
        M13_SET_TBL_BV_16(e->within_frange, eb + 0x00c);
        M13_SET_TBL_16(e->relation1, eb + 0x018);
        M13_SET_TBL_16(e->relation2, eb + 0x024);
        M13_SET_TBL_16(e->diplo_type, eb + 0x030);
        M13_SET_TBL_16(e->diplo_val, eb + 0x03c);
        M13_SET_TBL_16(e->diplo_p1, eb + 0x048);
        M13_SET_TBL_16(e->diplo_p2, eb + 0x054);
        M13_SET_TBL_16(e->trust, eb + 0x06c);
        M13_SET_TBL_16(e->broken_treaty, eb + 0x078);
        M13_SET_TBL_16(e->blunder, eb + 0x084);
        M13_SET_TBL_16(e->tribute_field, eb + 0x090);
        M13_SET_TBL_16(e->tribute_tech, eb + 0x09c);
        M13_SET_TBL_16(e->mood_treaty, eb + 0x0a8);
        M13_SET_TBL_16(e->mood_trade, eb + 0x0b4);
        M13_SET_TBL_16(e->mood_tech, eb + 0x0c0);
        M13_SET_TBL_16(e->mood_peace, eb + 0x0cc);
        M13_SET_TBL_16(e->treaty, eb + 0x0d8);
        M13_SET_TBL_16(e->trade_bc, eb + 0x0e4);
        M13_SET_TBL_16(e->trade_percent, eb + 0x0f0);
        M13_SET_TBL_16(e->spymode_next, eb + 0x0fc);
        M13_SET_TBL_16(e->offer_field, eb + 0x1d4);
        M13_SET_TBL_16(e->offer_tech, eb + 0x1e0);
        M13_SET_TBL_16(e->offer_bc, eb + 0x1ec);
        M13_SET_TBL_16_HATED(e->hated, eb + 0x21c);
        M13_SET_TBL_16_HATED(e->mutual_enemy, eb + 0x228);
        M13_SET_TBL_16(e->hatred, eb + 0x270);
        M13_SET_TBL_16(e->have_met, eb + 0x27c);
        M13_SET_TBL_16(e->trade_established_bc, eb + 0x288);
        M13_SET_TBL_16(e->spying, eb + 0x2a4);
        M13_SET_TBL_16(e->spyfund, eb + 0x2b0);
        M13_SET_TBL_16(e->spymode, eb + 0x2c8);
        M13_SET_16(e->security, eb + 0x2d4);
        M13_SET_TBL_16(e->spies, eb + 0x2d6);
        M13_SET_32(e->reserve_bc, eb + 0x2fc);
        M13_SET_16(e->tax, eb + 0x300);
        M13_SET_16(e->base_shield, eb + 0x302);
        M13_SET_16(e->base_comp, eb + 0x304);
        M13_SET_16(e->base_weapon, eb + 0x306);
        M13_SET_16(e->colonist_oper_factories, eb + 0x326);
        M13_SET_TBL_16(e->tech.percent, eb + 0x332 + 0x00);
        M13_SET_TBL_16(e->tech.slider, eb + 0x332 + 0x0c);
        M13_SET_TBL_16(e->tech.slider_lock, eb + 0x332 + 0x60);
        M13_SET_TBL_32(e->tech.investment, eb + 0x332 + 0x18);
        M13_SET_TBL_16(e->tech.project, eb + 0x332 + 0x30);
        M13_SET_TBL_32(e->tech.cost, eb + 0x332 + 0x3c);
        M13_SET_TBL_16(e->tech.completed, eb + 0x332 + 0x54);
        M13_SET_16_CHECK(e->shipdesigns_num, eb + 0x3a0, 0, 6);
        for (int j = 0; j < g->galaxy_stars; ++j) {
            const fleet_orbit_t *r = &(e->orbit[j]);
            int ob;
            ob = eb + 0x3a2 + j * 0x18;
            M13_SET_TBL_16(r->ships, ob + 0x0c);
        }
        M13_SET_TBL_16(g->eto[PLAYER_0].spyreportfield[i], eb + 0xdc2);
        M13_SET_16(g->eto[PLAYER_0].spyreportyear[i], eb + 0xdce);
        M13_SET_16(e->shipi_colony, eb + 0xdd0);
        M13_SET_16(e->shipi_bomber, eb + 0xdd2);
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        const shipresearch_t *srd = &(g->srd[i]);
        int srdb, pos;
        srdb = 0xc410 + i * 0x468;
        for (int j = 0; j < g->eto[i].shipdesigns_num; ++j) {
            if (libsave_moo13_en_sd(buf, &(srd->design[j]), srdb + j * 0x44) != 0) {
                return -1;
            }
        }
        pos = srdb + 0x228;
        for (int f = 0; f < TECH_FIELD_NUM; ++f) {
            for (int t = 0; t < TECH_TIER_NUM; ++t) {
                for (int j = 0; j < 3; ++j) {
                    M13_SET_8(srd->researchlist[f][t][j], pos);
                    ++pos;
                }
            }
        }
        pos = srdb + 0x2dc;
        for (int f = 0; f < TECH_FIELD_NUM; ++f) {
            for (int j = 0; j < TECH_PER_FIELD; ++j) {
                M13_SET_8(srd->researchcompleted[f][j], pos);
                ++pos;
            }
        }
        M13_SET_TBL_16(srd->have_reserve_fuel, srdb + 0x444);
        M13_SET_TBL_16(srd->year, srdb + 0x450);
        /* M13_SET_TBL_16(srd->shipcount, srdb + 0x45c); */
    }
    if (libsave_moo13_en_sd(buf, &(g->current_design[PLAYER_0]), 0xe642) != 0) {
        return -1;
    }
    {
        const gameevents_t *ev = &(g->evn);
        const int evb = 0xde80;
        M13_SET_16(ev->year, evb + 0x000);
        M13_SET_TBL_BVN_16(ev->done, evb + 0x004, 20);
        M13_SET_16(ev->have_plague, evb + 0x02c);
        M13_SET_16(ev->plague_player, evb + 0x02e);
        M13_SET_16(ev->plague_planet_i, evb + 0x030);
        M13_SET_16(ev->plague_val, evb + 0x032);
        M13_SET_16(ev->have_nova, evb + 0x03a);
        M13_SET_16(ev->nova_player, evb + 0x03c);
        M13_SET_16(ev->nova_planet_i, evb + 0x03e);
        M13_SET_16(ev->nova_years, evb + 0x040);
        M13_SET_16(ev->nova_val, evb + 0x042);
        M13_SET_16(ev->have_accident, evb + 0x044);
        M13_SET_16(ev->accident_planet_i, evb + 0x048);
        M13_SET_16(ev->have_comet, evb + 0x056);
        M13_SET_16(ev->comet_player, evb + 0x058);
        M13_SET_16(ev->comet_planet_i, evb + 0x05a);
        M13_SET_16(ev->comet_years, evb + 0x05c);
        M13_SET_16(ev->comet_hp, evb + 0x05e);
        M13_SET_16(ev->comet_dmg, evb + 0x060);
        M13_SET_16(ev->have_pirates, evb + 0x062);
        M13_SET_16(ev->pirates_planet_i, evb + 0x066);
        M13_SET_16(ev->pirates_hp, evb + 0x068);
        M13_SET_16(ev->crystal.exists, evb + 0x06e);
        M13_SET_16(ev->crystal.x, evb + 0x070);
        M13_SET_16(ev->crystal.y, evb + 0x072);
        M13_SET_16_KILLER(ev->crystal.killer, evb + 0x076);
        M13_SET_16(ev->crystal.dest, evb + 0x078);
        M13_SET_16(ev->crystal.counter, evb + 0x074);
        M13_SET_16(ev->crystal.nuked, evb + 0x07a);
        M13_SET_16(ev->amoeba.exists, evb + 0x07c);
        M13_SET_16(ev->amoeba.x, evb + 0x07e);
        M13_SET_16(ev->amoeba.y, evb + 0x080);
        M13_SET_16_KILLER(ev->amoeba.killer, evb + 0x084);
        M13_SET_16(ev->amoeba.dest, evb + 0x086);
        M13_SET_16(ev->amoeba.counter, evb + 0x082);
        M13_SET_16(ev->amoeba.nuked, evb + 0x088);
        M13_SET_8(ev->planet_orion_i, evb + 0x09c);
        M13_SET_8(ev->have_guardian, evb + 0x09e);
        M13_SET_TBL_16(ev->home, evb + 0x0a0);
        M13_SET_8(ev->report_stars, evb + 0x0ac);
        M13_SET_TBL_16(ev->new_ships[PLAYER_0], evb + 0x1a4);
        for (player_id_t i = PLAYER_1; i < g->players; ++i) {
            M13_SET_16(ev->spies_caught[i][PLAYER_0], evb + 0x1f2 + i * 2);
        }
        M13_SET_TBL_16(ev->spies_caught[PLAYER_0], evb + 0x1fe);
        M13_SET_TBL_16(ev->ceasefire[PLAYER_0], evb + 0x28e);
        M13_SET_TBL_BVN_8(ev->help_shown[PLAYER_0], evb + 0x2e2, 16);
        /* TODO build_finished ; is it even possible to save before clicking them away? */
        M13_SET_TBL_16_OWNER(ev->voted, evb + 0x320);
        M13_SET_16(ev->best_ecorestore[PLAYER_0], evb + 0x32c);
        M13_SET_16(ev->best_wastereduce[PLAYER_0], evb + 0x32e);
        M13_SET_16(ev->best_roboctrl[PLAYER_0], evb + 0x332);
        M13_SET_16(ev->best_terraform[PLAYER_0], evb + 0x334);
    }
    {
        uint8_t v = 0;
        int i, a = 0xe68e;
        for (i = 0; i < g->galaxy_stars; ++i) {
            if (BOOLVEC_IS1(g->planet[i].finished, FINISHED_SHIP)) {
                v |= (1 << (i & 7));
            }
            if ((i & 7) == 7) {
                M13_SET_8(v, a);
                ++a;
                v = 0;
            }
        }
        if ((i & 7) != 0) {
            M13_SET_8(v, a);
            ++a;
        }
    }
    M13_SET_16((g->guardian_killer == PLAYER_NONE) ? 1000 : g->guardian_killer, 0xe68a);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        const empiretechorbit_t *e = &(g->eto[i]);
        int srdb;
        srdb = 0xc410 + 0x468 * i;
        for (int j = 0; j < SHIP_NAME_NUM; ++j) {
            strncpy((char *)&(buf[srdb + 0x198 + j * 12]), game_str_tbl_ship_names[e->race * SHIP_NAME_NUM + j], 11);
        }
    }
    return 0;
}

int libsave_moo13_save_do(const char *filename, const struct game_s *g)
{
    uint8_t *buf = NULL;
    int res = 0;
    buf = lib_malloc(SAVE_MOO13_LEN);
    if (libsave_moo13_en_do(buf, g) != 0) {
        res = -1;
    } else if (util_file_save(filename, buf, SAVE_MOO13_LEN) != 0) {
        log_error("Save: failed to save '%s'\n", filename);
        res = -1;
    }
    lib_free(buf);
    buf = NULL;
    return res;
}
