#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "bits.h"
#include "comp.h"
#include "hw.h"
#include "game/game.h"
#include "game/game_aux.h"
#include "game/game_save.h"
#include "game/game_str.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "ui.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

const char *idstr_main = "saveconv";

bool main_use_lbx = false;
bool ui_use_audio = false;

/* -------------------------------------------------------------------------- */

static struct game_s *gameptr = 0;

static uint8_t *save2buf = 0;
static int save2len = 0;
static FILE *save2fd = 0;
static const char *save2fname = 0;

typedef enum {
    SAVETYPE_SMART = 0,
    SAVETYPE_MOO13,
    SAVETYPE_1OOM0,
    SAVETYPE_TEXT,
    SAVETYPE_DUMMY,
    SAVETYPE_NUM
} savetype_t;

#define SAVETYPE_NATIVE     SAVETYPE_1OOM0

#define SAVETYPE_F_OPTOUT   (1 << 1)

static int savetype_de_smart(struct game_s *g, const char *fname);
static int savetype_de_moo13(struct game_s *g, const char *fname);
static int savetype_en_moo13(struct game_s *g, const char *fname);
static int savetype_de_1oom0(struct game_s *g, const char *fname);
static int savetype_en_1oom0(struct game_s *g, const char *fname);
static int savetype_en_text(struct game_s *g, const char *fname);

static const struct {
    const char *name;
    int (*decode)(struct game_s *g, const char *fname); /* to native */
    int (*encode)(struct game_s *g, const char *fname);
    uint8_t flags;
    savetype_t othertype;
} savetype[SAVETYPE_NUM] = {
    { /* SAVETYPE_SMART */
        "smart",
        savetype_de_smart,
        0,
        0, SAVETYPE_NUM
    },
    { /* SAVETYPE_MOO13 */
        "MOO v1.3",
        savetype_de_moo13,
        savetype_en_moo13,
        0, SAVETYPE_1OOM0
    },
    { /* SAVETYPE_1OOM0 */
        "1oom save version 0",
        savetype_de_1oom0,
        savetype_en_1oom0,
        0, SAVETYPE_MOO13
    },
    { /* SAVETYPE_TEXT  */
        "text",
        0,
        savetype_en_text,
        SAVETYPE_F_OPTOUT, SAVETYPE_NUM
    },
    { /* SAVETYPE_DUMMY */
        "dummy",
        0,
        0,
        0, SAVETYPE_NUM
    }
};

static const struct {
    char const * const str;
    const savetype_t type;
} savetype_match[] = {
    { "m", SAVETYPE_MOO13 },
    { "1", SAVETYPE_1OOM0 },
    { "s", SAVETYPE_SMART },
    { "t", SAVETYPE_TEXT },
    { "d", SAVETYPE_DUMMY },
    { 0, 0 }
};

static savetype_t savetypei = SAVETYPE_SMART;
static savetype_t savetypeo = SAVETYPE_SMART;
static int main_fname_num = 0;
static const char *fnames[2] = { 0/*in*/, 0/*out*/ };

static char savename[SAVE_NAME_LEN] = "";

static bool opt_use_configmoo = false;

/* -------------------------------------------------------------------------- */

static int save_out_open(const char *fname)
{
    if (fname && (strcmp(fname, "-") != 0))  {
        save2fd = fopen(fname, "wb+");
        if (!save2fd) {
            log_error("opening file '%s' for output!\n", fname);
            return -1;
        }
        save2fname = fname;
    } else {
        save2fd = stdout;
        save2fname = 0;
    }
    return 0;
}

static int save_out_flush(void)
{
    if (save2len && fwrite(save2buf, save2len, 1, save2fd) != 1) {
        log_error("writing to file '%s'!\n", save2fname ? save2fname : "(stdout)");
        return -1;
    }
    save2len = 0;
    return 0;
}

static int save_out_close(void)
{
    if (save2fd && save2fname) {
        fclose(save2fd);
        save2fd = 0;
        save2fname = 0;
    }
    return 0;
}

static int save_out_write(const char *fname)
{
    return save_out_open(fname) || save_out_flush() || save_out_close();
}

/* -------------------------------------------------------------------------- */

static int savetype_de_smart(struct game_s *g, const char *fname)
{
    FILE *fd;
    int res;
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname));
    if ((fd = fopen(fname, "rb")) == 0) {
        log_error("opening file '%s'\n", fname);
        return -1;
    }
    fclose(fd);
    fd = NULL;
    if ((fd = game_save_open_check_header(fname, -1, false, 0)) != 0) {
        fclose(fd);
        fd = NULL;
        savetypei = SAVETYPE_NATIVE;
        res = savetype[SAVETYPE_NATIVE].decode(g, fname);
    } else if ((res = savetype_de_moo13(g, fname)) == 0) {
        savetypei = SAVETYPE_MOO13;
    } else {
        log_error("file '%s' type autodetection failed\n", fname);
        return -1;
    }
    LOG_DEBUG((1, "%s: i '%s' o '%s'\n", __func__, savetype[savetypei].name, savetype[savetypeo].name));
    if (savetypeo == SAVETYPE_SMART) {
        savetype_t typeo = savetype[savetypei].othertype;
        if (typeo == SAVETYPE_NUM) {
            log_error("BUG: no other type for type '%s'\n", savetype[savetypei].name);
            return -1;
        }
        savetypeo = typeo;
        LOG_DEBUG((1, "%s: diverted to '%s'\n", __func__, savetype[typeo].name));
    }
    return res;
}

/* -------------------------------------------------------------------------- */

static int try_load_len(const char *fname, uint8_t *buf, int wantlen)
{
    FILE *fd = 0;
    int len = -1;
    if (0
      || ((fd = fopen(fname, "rb")) == 0)
      || ((len = fread(buf, 1, wantlen + 1, fd)) != wantlen)
    ) {
        LOG_DEBUG((1, "%s: loading '%s' (got %i != %i bytes)\n", __func__, fname, len, wantlen));
        len = -1;
    }
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    return len;
}

/* -------------------------------------------------------------------------- */

#define SAVE_MOO13_LEN  59036
#define SAVE_CMOO_LEN   154

#define M13_GET_8(item_, addr_)     item_ = save2buf[addr_]
#define M13_GET_16(item_, addr_)    item_ = GET_LE_16(&save2buf[addr_])
#define M13_GET_32(item_, addr_)    item_ = GET_LE_32(&save2buf[addr_])
#define M13_GET_16_OWNER(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = GET_LE_16(&save2buf[addr_]); \
        if (t_ == 0xffff) { t_ = PLAYER_NONE; }; \
        item_ = t_; \
    } while (0)
#define M13_GET_16_KILLER(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = GET_LE_16(&save2buf[addr_]); \
        if (t_ == 0) { t_ = PLAYER_NONE; } else { --t_; } \
        item_ = t_; \
    } while (0)
#define M13_GET_16_CHECK(item_, addr_, l_, h_) \
    do { \
        int t_; \
        t_ = GET_LE_16(&save2buf[addr_]); \
        if ((t_ < l_) || (t_ > h_)) { \
            log_error( #item_ " at 0x%04x is %i and not in range %i..%i\n", addr_, t_, l_, h_); \
            return -1; \
        } \
        item_ = t_; \
    } while (0)
#define M13_GET_TBL_16(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            item_[i_] = GET_LE_16(&save2buf[(addr_) + i_ * 2]); \
        } \
    } while (0)
#define M13_GET_TBL_16_OWNER(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = GET_LE_16(&save2buf[(addr_) + i_ * 2]); \
            if (t_ == 0xffff) { t_ = PLAYER_NONE; }; \
            item_[i_] = t_; \
        } \
    } while (0)
#define M13_GET_TBL_16_HATED(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = GET_LE_16(&save2buf[(addr_) + i_ * 2]); \
            if (t_ == 0) { t_ = PLAYER_NONE; } \
            item_[i_] = t_; \
        } \
    } while (0)
#define M13_GET_TBL_BVN_8(item_, addr_, n_) \
    do { \
        for (int i_ = 0; i_ < n_; ++i_) { \
            if (save2buf[(addr_) + i_]) { \
                BOOLVEC_SET1(item_, i_); \
            } \
        } \
    } while (0)
#define M13_GET_TBL_BV_16(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < PLAYER_NUM; ++i_) { \
            uint16_t t_; \
            t_ = GET_LE_16(&save2buf[(addr_) + i_ * 2]); \
            if (t_) { \
                BOOLVEC_SET1(item_, i_); \
            } \
        } \
    } while (0)
#define M13_GET_TBL_BVN_16(item_, addr_, n_) \
    do { \
        for (int i_ = 0; i_ < n_; ++i_) { \
            uint16_t t_; \
            t_ = GET_LE_16(&save2buf[(addr_) + i_ * 2]); \
            if (t_) { \
                BOOLVEC_SET1(item_, i_); \
            } \
        } \
    } while (0)
#define M13_GET_TBL_32(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            item_[i_] = GET_LE_32(&save2buf[(addr_) + i_ * 4]); \
        } \
    } while (0)

static int savetype_de_moo13_sd(shipdesign_t *sd, int sb)
{
    memcpy(sd->name, &save2buf[sb + 0x00], 11);
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

static int savetype_de_moo13(struct game_s *g, const char *fname)
{
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname));
    {
        int len;
        if ((len = try_load_len(fname, save2buf, SAVE_MOO13_LEN)) <= 0) {
            log_error("loading MOO1 v1.3 save '%s' (got %i != %i bytes)\n", fname, len, SAVE_MOO13_LEN);
            return -1;
        }
        save2len = len;
    }
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
    for (int i = 0; i < g->players; ++i) {
        memcpy(g->emperor_names[i], &save2buf[0xe1ba + i * 15], EMPEROR_NAME_LEN - 1);
    }
    M13_GET_16(g->planet_focus_i[0], 0xe236);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        int pb;
        pb = i * 0xb8;
        memcpy(p->name, &save2buf[pb], PLANET_NAME_LEN - 1);
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
    for (int j = PLAYER_1; j < g->players; ++j) {
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
    for (int i = 0; i < g->players; ++i) {
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
        M13_GET_TBL_16(e->hmm06c, eb + 0x06c);
        M13_GET_TBL_16(e->broken_treaty, eb + 0x078);
        M13_GET_TBL_16(e->hmm084, eb + 0x084);
        M13_GET_TBL_16(e->tribute_field, eb + 0x090);
        M13_GET_TBL_16(e->tribute_tech, eb + 0x09c);
        M13_GET_TBL_16(e->hmm0a8, eb + 0x0a8);
        M13_GET_TBL_16(e->hmm0b4, eb + 0x0b4);
        M13_GET_TBL_16(e->hmm0c0, eb + 0x0c0);
        M13_GET_TBL_16(e->hmm0cc, eb + 0x0cc);
        M13_GET_TBL_16(e->treaty, eb + 0x0d8);
        M13_GET_TBL_16(e->trade_bc, eb + 0x0e4);
        M13_GET_TBL_16(e->trade_percent, eb + 0x0f0);
        M13_GET_TBL_16(e->spymode_next, eb + 0x0fc);
        M13_GET_TBL_16(e->offer_field, eb + 0x1d4);
        M13_GET_TBL_16(e->offer_tech, eb + 0x1e0);
        M13_GET_TBL_16(e->offer_bc, eb + 0x1ec);
        M13_GET_TBL_16_HATED(e->hated, eb + 0x21c);
        M13_GET_TBL_16_HATED(e->mutual_enemy, eb + 0x228);
        M13_GET_TBL_16(e->hmm270, eb + 0x270);
        M13_GET_TBL_16(e->hmm27c, eb + 0x27c);
        M13_GET_TBL_16(e->hmm288, eb + 0x288);
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
        M13_GET_TBL_16(e->spyreportfield[PLAYER_0], eb + 0xdc2);
        M13_GET_16(e->spyreportyear[PLAYER_0], eb + 0xdce);
        M13_GET_16(e->shipi_colony, eb + 0xdd0);
        M13_GET_16(e->shipi_bomber, eb + 0xdd2);
    }
    for (int i = 0; i < g->players; ++i) {
        shipresearch_t *srd = &(g->srd[i]);
        int srdb, pos;
        srdb = 0xc410 + i * 0x468;
        for (int j = 0; j < g->eto[i].shipdesigns_num; ++j) {
            if (savetype_de_moo13_sd(&(srd->design[j]), srdb + j * 0x44) != 0) {
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
        M13_GET_TBL_16(srd->shipcount, srdb + 0x45c);
    }
    if (savetype_de_moo13_sd(&(g->current_design[PLAYER_0]), 0xe642) != 0) {
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
        M13_GET_8(ev->report_stars, evb + 0x09e);
        M13_GET_TBL_16(ev->new_ships[PLAYER_0], evb + 0x1a4);
        for (int i = 1; i < g->players; ++i) {
            M13_GET_16(ev->spies_caught[i][PLAYER_0], evb + 0x1f2 + i * 2);
        }
        M13_GET_TBL_16(ev->spies_caught[PLAYER_0], evb + 0x1fe);
        M13_GET_TBL_16(ev->hmm28e[PLAYER_0], evb + 0x28e); /* FIXME check index order */
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
    if (savename[0] == 0) {
        char *dir;
        char *fnam;
        char savei;
        util_fname_split(fname, &dir, &fnam);
        savei = fnam[4] - '0';
        fnam[4] = 'x';
        if (strcasecmp(fnam, "savex.gam") != 0) {
            savei = 0;
        }
        if (opt_use_configmoo && (savei >= 1) && (savei <= 6)) {
            bool found = true;
            uint8_t *cmoobuf = &save2buf[SAVE_MOO13_LEN];
            char *fullname = 0;
            const char *tryname;
            tryname = "config.moo";
            if (dir) {
                tryname = fullname = util_concat(dir, FSDEV_DIR_SEP_STR, tryname, NULL);
            }
            if (try_load_len(tryname, cmoobuf, SAVE_CMOO_LEN) <= 0) {
                tryname = "CONFIG.MOO";
                if (dir) {
                    lib_free(fullname);
                    tryname = fullname = util_concat(dir, FSDEV_DIR_SEP_STR, tryname, NULL);
                }
                if (try_load_len(tryname, cmoobuf, SAVE_CMOO_LEN) <= 0) {
                    found = false;
                }
            }
            if (found) {
                strncpy(savename, (const char *)&cmoobuf[0x22 + 20 * (savei - 1)], SAVE_NAME_LEN);
                savename[SAVE_NAME_LEN - 1] = '\0';
                LOG_DEBUG((1, "found '%s' slot %i '%s'\n", tryname, savei, savename));
            }
            if (fullname) {
                lib_free(fullname);
                fullname = 0;
            }
        }
        lib_free(fnam);
        lib_free(dir);
        if ((savename[0] == 0) && (savei >= 1) && (savei <= 7)) {
            sprintf(savename, "v1.3 SAVE%i.GAM", savei);
            LOG_DEBUG((1, "generated '%s' -> '%s'\n", fname, savename));
        }
    }
    return 0;
}

#define M13_SET_8(item_, addr_)     save2buf[addr_] = item_
#define M13_SET_16(item_, addr_)    SET_LE_16(&save2buf[addr_], item_)
#define M13_SET_32(item_, addr_)    SET_LE_32(&save2buf[addr_], item_)
#define M13_SET_16_OWNER(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = item_; \
        if (t_ == PLAYER_NONE) { t_ = 0xffff; }; \
        SET_LE_16(&save2buf[addr_], t_); \
    } while (0)
#define M13_SET_16_HATED(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = item_; \
        if (t_ == PLAYER_NONE) { t_ = 0; } \
        SET_LE_16(&save2buf[addr_], t_); \
    } while (0)
#define M13_SET_16_KILLER(item_, addr_) \
    do { \
        uint16_t t_; \
        t_ = item_; \
        if (t_ == PLAYER_NONE) { t_ = 0; } else { ++t_; } \
        SET_LE_16(&save2buf[addr_], t_); \
    } while (0)
#define M13_SET_16_CHECK(item_, addr_, l_, h_) \
    do { \
        uint16_t t_; \
        t_ = item_; \
        if ((t_ < l_) || (t_ > h_)) { \
            log_error( #item_ " is %i and not in range %i..%i\n", addr_, t_, l_, h_); \
            return -1; \
        } \
        SET_LE_16(&save2buf[addr_], t_); \
    } while (0)
#define M13_SET_TBL_16(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = item_[i_]; \
            SET_LE_16(&save2buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_16_OWNER(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = item_[i_]; \
            if (t_ == PLAYER_NONE) { t_ = 0xffff; }; \
            SET_LE_16(&save2buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_16_HATED(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint16_t t_; \
            t_ = item_[i_]; \
            if (t_ == PLAYER_NONE) { t_ = 0; } \
            SET_LE_16(&save2buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_BVN_8(item_, addr_, n_) \
    do { \
        for (int i_ = 0; i_ < n_; ++i_) { \
            uint8_t t_; \
            t_ = BOOLVEC_IS1(item_, i_); \
            save2buf[(addr_) + i_] = t_; \
        } \
    } while (0)
#define M13_SET_TBL_BV_16(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < PLAYER_NUM; ++i_) { \
            uint16_t t_; \
            t_ = BOOLVEC_IS1(item_, i_); \
            SET_LE_16(&save2buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_BVN_16(item_, addr_, n_) \
    do { \
        for (int i_ = 0; i_ < n_; ++i_) { \
            uint16_t t_; \
            t_ = BOOLVEC_IS1(item_, i_); \
            SET_LE_16(&save2buf[(addr_) + i_ * 2], t_); \
        } \
    } while (0)
#define M13_SET_TBL_32(item_, addr_) \
    do { \
        for (int i_ = 0; i_ < TBLLEN(item_); ++i_) { \
            uint32_t t_; \
            t_ = item_[i_]; \
            SET_LE_32(&save2buf[(addr_) + i_ * 4], t_); \
        } \
    } while (0)

static int savetype_en_moo13_sd(const shipdesign_t *sd, int sb)
{
    memcpy(&save2buf[sb + 0x00], sd->name, 11);
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

static int savetype_en_moo13(struct game_s *g, const char *fname)
{
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname ? fname : "(null)"));
    memset(save2buf, 0, SAVE_MOO13_LEN);
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
    for (int i = 0; i < g->players; ++i) {
        memcpy(&save2buf[0xe1ba + i * 15], g->emperor_names[i], EMPEROR_NAME_LEN - 1);
    }
    M13_SET_16(g->planet_focus_i[0], 0xe236);
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        int pb;
        pb = i * 0xb8;
        memcpy(&save2buf[pb], p->name, PLANET_NAME_LEN - 1);
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
    for (int i = 0; i < g->players; ++i) {
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
        M13_SET_TBL_16(e->hmm06c, eb + 0x06c);
        M13_SET_TBL_16(e->broken_treaty, eb + 0x078);
        M13_SET_TBL_16(e->hmm084, eb + 0x084);
        M13_SET_TBL_16(e->tribute_field, eb + 0x090);
        M13_SET_TBL_16(e->tribute_tech, eb + 0x09c);
        M13_SET_TBL_16(e->hmm0a8, eb + 0x0a8);
        M13_SET_TBL_16(e->hmm0b4, eb + 0x0b4);
        M13_SET_TBL_16(e->hmm0c0, eb + 0x0c0);
        M13_SET_TBL_16(e->hmm0cc, eb + 0x0cc);
        M13_SET_TBL_16(e->treaty, eb + 0x0d8);
        M13_SET_TBL_16(e->trade_bc, eb + 0x0e4);
        M13_SET_TBL_16(e->trade_percent, eb + 0x0f0);
        M13_SET_TBL_16(e->spymode_next, eb + 0x0fc);
        M13_SET_TBL_16(e->offer_field, eb + 0x1d4);
        M13_SET_TBL_16(e->offer_tech, eb + 0x1e0);
        M13_SET_TBL_16(e->offer_bc, eb + 0x1ec);
        M13_SET_TBL_16_HATED(e->hated, eb + 0x21c);
        M13_SET_TBL_16_HATED(e->mutual_enemy, eb + 0x228);
        M13_SET_TBL_16(e->hmm270, eb + 0x270);
        M13_SET_TBL_16(e->hmm27c, eb + 0x27c);
        M13_SET_TBL_16(e->hmm288, eb + 0x288);
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
        M13_SET_TBL_16(e->spyreportfield[PLAYER_0], eb + 0xdc2);
        M13_SET_16(e->spyreportfield[PLAYER_0][i], eb + 0xdce);
        M13_SET_16(e->shipi_colony, eb + 0xdd0);
        M13_SET_16(e->shipi_bomber, eb + 0xdd2);
    }
    for (int i = 0; i < g->players; ++i) {
        const shipresearch_t *srd = &(g->srd[i]);
        int srdb, pos;
        srdb = 0xc410 + i * 0x468;
        for (int j = 0; j < g->eto[i].shipdesigns_num; ++j) {
            if (savetype_en_moo13_sd(&(srd->design[j]), srdb + j * 0x44) != 0) {
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
        M13_SET_TBL_16(srd->shipcount, srdb + 0x45c);
    }
    save2len = SAVE_MOO13_LEN;
    if (savetype_en_moo13_sd(&(g->current_design[PLAYER_0]), 0xe642) != 0) {
        return -1;
    }
    {
        gameevents_t *ev = &(g->evn);
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
        M13_SET_8(ev->report_stars, evb + 0x09e);
        M13_SET_TBL_16(ev->new_ships[PLAYER_0], evb + 0x1a4);
        for (int i = 1; i < g->players; ++i) {
            M13_SET_16(ev->spies_caught[i][PLAYER_0], evb + 0x1f2 + i * 2);
        }
        M13_SET_TBL_16(ev->spies_caught[PLAYER_0], evb + 0x1fe);
        M13_SET_TBL_16(ev->hmm28e[PLAYER_0], evb + 0x28e); /* FIXME check index order */
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
    M13_SET_16(1000, 0xe68a);
    for (int i = 0; i < g->players; ++i) {
        const empiretechorbit_t *e = &(g->eto[i]);
        int srdb;
        srdb = 0xc410 + 0x468 * i;
        for (int j = 0; j < SHIP_NAME_NUM; ++j) {
            strncpy((char *)&(save2buf[srdb + 0x198 + j * 12]), game_str_tbl_ship_names[e->race * SHIP_NAME_NUM + j], 11);
        }
    }
    if (save_out_write(fname) != 0) {
        return -1;
    }
    if (opt_use_configmoo) {
        char *dir;
        char *fnam;
        char savei;
        util_fname_split(fname, &dir, &fnam);
        savei = fnam[4] - '0';
        fnam[4] = 'x';
        if (strcasecmp(fnam, "savex.gam") != 0) {
            savei = 0;
        }
        if (opt_use_configmoo && (savei >= 1) && (savei <= 6)) {
            bool found = true;
            uint8_t *cmoobuf = &save2buf[SAVE_MOO13_LEN];
            char *fullname = 0;
            const char *tryname;
            tryname = "config.moo";
            if (dir) {
                tryname = fullname = util_concat(dir, FSDEV_DIR_SEP_STR, tryname, NULL);
            }
            if (try_load_len(tryname, cmoobuf, SAVE_CMOO_LEN) <= 0) {
                tryname = "CONFIG.MOO";
                if (dir) {
                    lib_free(fullname);
                    tryname = fullname = util_concat(dir, FSDEV_DIR_SEP_STR, tryname, NULL);
                }
                if (try_load_len(tryname, cmoobuf, SAVE_CMOO_LEN) <= 0) {
                    found = false;
                }
            }
            if (found) {
                strncpy((char *)&cmoobuf[0x22 + 20 * (savei - 1)], savename, 20);
                cmoobuf[0x22 + 20 * (savei - 1) + 20 - 1] = '\0';
                cmoobuf[0x16 + 2 * (savei - 1)] = 1;    /* set have save flag */
                LOG_DEBUG((1, "set '%s' slot %i '%s'\n", tryname, savei, savename));
                if (util_file_save(tryname, cmoobuf, SAVE_CMOO_LEN)) {
                    log_warning("failed to save '%s'\n", tryname);
                }
            }
            if (fullname) {
                lib_free(fullname);
                fullname = 0;
            }
        }
        lib_free(fnam);
        lib_free(dir);
    }
    return 0;
}

/* -------------------------------------------------------------------------- */

#define OUTADD  save2len += sprintf((char *)&save2buf[save2len],
#define OUTPRE()    OUTADD "%s", tp->buf)
#define OUTLINE OUTPRE(); OUTADD
#define OUTTBL(_name_, _num_, _var_) \
    do { \
        OUTADD "%s[] = { ", _name_); \
        for (int i_ = 0; i_ < _num_; ++i_) { \
            OUTADD "%i", _var_[i_]); \
            if (i_ != (_num_ - 1)) { OUTADD ", "); } \
        } \
        OUTADD " }"); \
    } while (0)
#define OUTLINETBL(_name_, _num_, _var_) OUTPRE(); OUTTBL(_name_, _num_, _var_); OUTADD "\n")
#define OUTLINETBLNON0(_name_, _num_, _var_) \
    do { \
        for (int j_ = 0; j_ < _num_; ++j_) { \
            if (_var_[j_]) { OUTLINETBL(_name_, _num_, _var_); break; } \
        } \
    } while (0)
#define OUTFLUSH() do { if (save_out_flush()) { return -1; } } while (0)

struct text_dump_prefix_s {
    char buf[128];
    int pos[10];
    int num;
};

static void text_dump_prefix_init(struct text_dump_prefix_s *tp)
{
    tp->num = 0;
    tp->pos[0] = 0;
    tp->buf[0] = 0;
}

static void text_dump_prefix_add(struct text_dump_prefix_s *tp, const char *str, const char *sep)
{
    int num, pos;
    num = tp->num++;
    pos = tp->pos[num];
    pos += sprintf(&(tp->buf[pos]), "%s%s", str, sep);
    tp->pos[num + 1] = pos;
}

static void text_dump_prefix_add_tbl(struct text_dump_prefix_s *tp, const char *str, const char *sep, int index)
{
    char buf[128];
    sprintf(buf, "%s[%i]", str, index);
    text_dump_prefix_add(tp, buf, sep);
}

static void text_dump_prefix_del(struct text_dump_prefix_s *tp)
{
    int pos;
    pos = tp->pos[--tp->num];
    tp->buf[pos] = 0;
}

static const char *savetype_en_bv(const BOOLVEC_PTRPARAMI(bv), int len)
{
    static char buf[128]; /* HACK */
    char *p = &(buf[127]);
    *p-- = 0;
    for (int i = 0; i < len; ++i) {
        char c;
        c = BOOLVEC_IS1(bv, i) ? '1' : '0';
        *p-- = c;
    }
    *p-- = 'b';
    *p = '0';
    return p;
}

static void savetype_en_text_sd(const shipdesign_t *sd, struct text_dump_prefix_s *tp)
{
    OUTLINE "name = \"%s\"\n", sd->name);
    OUTLINE "cost = %i\n", sd->cost);
    OUTLINE "space = %i\n", sd->space);
    OUTLINE "hull = %i\n", sd->hull);
    OUTLINE "look = %i\n", sd->look);
    OUTLINETBL("wpnt", WEAPON_SLOT_NUM, sd->wpnt);
    OUTLINETBL("wpnn", WEAPON_SLOT_NUM, sd->wpnn);
    OUTLINE "engine = %i\n", sd->engine);
    OUTLINE "engines = %i\n", sd->engines);
    OUTLINETBL("special", SPECIAL_SLOT_NUM, sd->special);
    OUTLINE "shield = %i\n", sd->shield);
    OUTLINE "jammer = %i\n", sd->jammer);
    OUTLINE "comp = %i\n", sd->comp);
    OUTLINE "armor = %i\n", sd->armor);
    OUTLINE "man = %i\n", sd->man);
    OUTLINE "hp = %i\n", sd->hp);
}

static void savetype_en_text_monster(const monster_t *m, struct text_dump_prefix_s *tp)
{
    OUTLINE "exists = %i\n", m->exists);
    OUTLINE "x = %i\n", m->x);
    OUTLINE "y = %i\n", m->y);
    OUTLINE "killer = %i\n", m->killer);
    OUTLINE "dest = %i\n", m->dest);
    OUTLINE "counter = %i\n", m->counter);
    OUTLINE "nuked = %i\n", m->nuked);
}

static int savetype_en_text(struct game_s *g, const char *fname)
{
    struct text_dump_prefix_s tp[1];
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname));
    save2len = 0;
    if (save_out_open(fname)) {
        return -1;
    }
    text_dump_prefix_init(tp);
    OUTLINE "// savename = \"%s\"\n", savename);
    text_dump_prefix_add(tp, "g", "->");
    OUTLINE "players = %i\n", g->players);
    OUTLINE "is_ai = %s\n", savetype_en_bv(g->is_ai, g->players));
    OUTLINE "active_player = %i\n", g->active_player);
    OUTLINE "difficulty = %i // %s\n", g->difficulty, game_str_tbl_diffic[g->difficulty]);
    OUTLINE "galaxy_size = %i // %s\n", g->galaxy_size, game_str_tbl_gsize[g->galaxy_size]);
    OUTLINE "galaxy_w = %i\n", g->galaxy_w);
    OUTLINE "galaxy_h = %i\n", g->galaxy_h);
    OUTLINE "galaxy_stars = %i\n", g->galaxy_stars);
    OUTLINE "galaxy_maxx = %i\n", g->galaxy_maxx);
    OUTLINE "galaxy_maxy = %i\n", g->galaxy_maxy);
    OUTLINE "galaxy_seed = 0x%x\n", g->galaxy_seed);
    OUTLINE "seed = 0x%x\n", g->seed);
    OUTLINE "year = %i // %i\n", g->year, g->year + YEAR_BASE);
    OUTLINE "enroute_num = %i\n", g->enroute_num);
    OUTLINE "transport_num = %i\n", g->transport_num);
    OUTLINE "end = %i\n", g->end);
    OUTLINE "winner = %i\n", g->winner);
    OUTLINE "election_held = %i\n", g->election_held);
    OUTFLUSH();
    OUTLINE "nebula_num = %i\n", g->nebula_num);
    for (int i = 0; i < g->nebula_num; ++i) {
        OUTLINE "nebula[%i] = { .x = %i, .y = %i", i, g->nebula_x[i], g->nebula_y[i]);
        OUTADD ", .x0 = { %i, %i, %i, %i }", g->nebula_x0[i][0], g->nebula_x0[i][1], g->nebula_x0[i][2], g->nebula_x0[i][3]);
        OUTADD ", .x1 = { %i, %i, %i, %i }", g->nebula_x1[i][0], g->nebula_x1[i][1], g->nebula_x1[i][2], g->nebula_x1[i][3]);
        OUTADD ", .y0 = { %i, %i, %i, %i }", g->nebula_y0[i][0], g->nebula_y0[i][1], g->nebula_y0[i][2], g->nebula_y0[i][3]);
        OUTADD ", .y1 = { %i, %i, %i, %i } }\n", g->nebula_y1[i][0], g->nebula_y1[i][1], g->nebula_y1[i][2], g->nebula_y1[i][3]);
    }
    for (int i = 0; i < g->players; ++i) {
        OUTLINE "emperor_names[%i] = \"%s\"\n", i, g->emperor_names[i]);
    }
    for (int i = 0; i < g->players; ++i) {
        OUTLINE "planet_focus_i[%i] = %i\n", i, g->planet_focus_i[i]);
    }
    OUTFLUSH();
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        text_dump_prefix_add_tbl(tp, "planet", ".", i);
        OUTLINE "name = \"%s\"\n", p->name);
        OUTLINE "x = %i\n", p->x);
        OUTLINE "y = %i\n", p->y);
        OUTLINE "star_type = %i\n", p->star_type);
        OUTLINE "look = %i\n", p->look);
        OUTLINE "frame = %i\n", p->frame);
        OUTLINE "rocks = %i\n", p->rocks);
        OUTLINE "max_pop1 = %i\n", p->max_pop1);
        OUTLINE "max_pop2 = %i\n", p->max_pop2);
        OUTLINE "max_pop3 = %i\n", p->max_pop3);
        OUTLINE "type = %i // %s\n", p->type, game_str_tbl_sm_pltype[p->type]);
        OUTLINE "battlebg = %i\n", p->battlebg);
        OUTLINE "infogfx = %i\n", p->infogfx);
        OUTLINE "growth = %i // %s\n", p->growth, game_str_tbl_sm_pgrowth[p->growth]);
        OUTLINE "special = %i // %s\n", p->special, game_str_tbl_sm_pspecial[p->special]);
        OUTLINE "owner = %i\n", p->owner);
        OUTLINE "prev_owner = %i\n", p->prev_owner);
        OUTLINE "claim = %i\n", p->claim);
        OUTLINE "waste = %i\n", p->waste);
        OUTLINE "explored = %s\n", savetype_en_bv(p->explored, g->players));
        if (p->owner != PLAYER_NONE) {
            OUTLINE "bc_to_ecoproj = %i\n", p->bc_to_ecoproj);
            OUTLINE "bc_to_ship = %i\n", p->bc_to_ship);
            OUTLINE "bc_to_factory = %i\n", p->bc_to_factory);
            OUTLINE "reserve = %i\n", p->reserve);
            OUTLINE "pop = %i\n", p->pop);
            OUTLINE "pop_prev = %i\n", p->pop_prev);
            OUTLINE "factories = %i\n", p->factories);
            OUTLINE "slider[] = { %i, %i, %i, %i, %i }\n", p->slider[0], p->slider[1], p->slider[2], p->slider[3], p->slider[4]);
            OUTLINE "slider_lock[] = { %i, %i, %i, %i, %i }\n", p->slider_lock[0], p->slider_lock[1], p->slider_lock[2], p->slider_lock[3], p->slider_lock[4]);
            OUTLINE "buildship = %i\n", p->buildship);
            OUTLINE "reloc = %i\n", p->reloc);
            OUTLINE "missile_bases = %i\n", p->missile_bases);
            OUTLINE "bc_to_base = %i\n", p->bc_to_base);
            OUTLINE "bc_upgrade_base = %i\n", p->bc_upgrade_base);
            OUTLINE "have_stargate = %i\n", p->have_stargate);
            OUTLINE "shield = %i\n", p->shield);
            OUTLINE "bc_to_shield = %i\n", p->bc_to_shield);
            OUTLINE "trans_num = %i\n", p->trans_num);
            OUTLINE "trans_dest = %i\n", p->trans_dest);
            OUTLINE "pop_tenths = %i\n", p->pop_tenths);
            OUTLINE "pop_oper_fact = %i\n", p->pop);
            OUTLINE "bc_to_refit = %i\n", p->bc_to_refit);
            OUTLINE "rebels = %i\n", p->rebels);
            OUTLINE "unrest = %i\n", p->unrest);
            OUTLINE "unrest_reported = %i\n", p->unrest_reported);
            OUTLINE "finished = %s\n", savetype_en_bv(p->finished, FINISHED_NUM));
        }
        text_dump_prefix_del(tp);
        OUTFLUSH();
    }
    for (int j = 0; j < g->players; ++j) {
        for (int i = 0; i < g->galaxy_stars; ++i) {
            const seen_t *s = &(g->seen[j][i]);
            OUTLINE "seen[%i][%i] = { .owner = %i, .pop = %i, .bases = %i, .factories = %i }\n", j, i, s->owner, s->pop, s->bases, s->factories);
        }
        OUTFLUSH();
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        OUTLINE "enroute[%i] = { .owner = %i, .x = %i, .y = %i, .dest = %i, .speed = %i", i, r->owner, r->x, r->y, r->dest, r->speed);
        OUTADD ", .ships = { %i, %i, %i, %i, %i, %i } }\n", r->ships[0], r->ships[1], r->ships[2], r->ships[3], r->ships[4], r->ships[5]);
    }
    OUTFLUSH();
    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &(g->transport[i]);
        OUTLINE "transport[%i] = { .owner = %i, .x = %i, .y = %i, .dest = %i, .speed = %i", i, r->owner, r->x, r->y, r->dest, r->speed);
        OUTADD ", .pop = %i }\n", r->pop);
    }
    OUTFLUSH();
    for (int pl = 0; pl < g->players; ++pl) {
        const empiretechorbit_t *e = &(g->eto[pl]);
        text_dump_prefix_add_tbl(tp, "eto", ".", pl);
        OUTLINE "race = %i // %s\n", e->race, game_str_tbl_race[e->race]);
        OUTLINE "banner = %i // %s\n", e->banner, game_str_tbl_banner[e->banner]);
        OUTLINE "trait1 = %i // %s\n", e->trait1, game_str_tbl_trait1[e->trait1]);
        OUTLINE "trait2 = %i // %s\n", e->trait2, game_str_tbl_trait2[e->trait2]);
        OUTLINE "ai_p3_countdown = %i\n", e->ai_p3_countdown);
        OUTLINE "ai_p2_countdown = %i\n", e->ai_p2_countdown);
        OUTLINE "within_frange = %s\n", savetype_en_bv(e->within_frange, g->players));
        OUTLINETBL("relation1", g->players, e->relation1);
        OUTLINETBL("relation2", g->players, e->relation2);
        OUTLINETBL("diplo_type", g->players, e->diplo_type);
        OUTLINETBL("diplo_val", g->players, e->diplo_val);
        OUTLINETBL("diplo_p1", g->players, e->diplo_p1);
        OUTLINETBL("diplo_p2", g->players, e->diplo_p2);
        OUTLINETBL("hmm06c", g->players, e->hmm06c);
        OUTLINETBL("broken_treaty", g->players, e->broken_treaty);
        OUTLINETBL("hmm084", g->players, e->hmm084);
        OUTLINETBL("tribute_field", g->players, e->tribute_field);
        OUTLINETBL("tribute_tech", g->players, e->tribute_tech);
        OUTLINETBL("hmm0a8", g->players, e->hmm0a8);
        OUTLINETBL("hmm0b4", g->players, e->hmm0b4);
        OUTLINETBL("hmm0c0", g->players, e->hmm0c0);
        OUTLINETBL("hmm0cc", g->players, e->hmm0cc);
        OUTLINETBL("treaty", g->players, e->treaty);
        OUTLINETBL("trade_bc", g->players, e->trade_bc);
        OUTLINETBL("trade_percent", g->players, e->trade_percent);
        OUTLINETBL("spymode_next", g->players, e->spymode_next);
        OUTLINETBL("offer_field", g->players, e->offer_field);
        OUTLINETBL("offer_tech", g->players, e->offer_tech);
        OUTLINETBL("offer_bc", g->players, e->offer_bc);
        OUTLINETBL("hated", g->players, e->hated);
        OUTLINETBL("mutual_enemy", g->players, e->mutual_enemy);
        OUTLINETBL("hmm270", g->players, e->hmm270);
        OUTLINETBL("hmm27c", g->players, e->hmm27c);
        OUTLINETBL("hmm288", g->players, e->hmm288);
        OUTLINETBL("spying", g->players, e->spying);
        OUTLINETBL("spyfund", g->players, e->spyfund);
        OUTLINETBL("spymode", g->players, e->spymode);
        OUTLINE "security = %i\n", e->security);
        OUTLINETBL("spies", g->players, e->spies);
        OUTLINE "reserve_bc = %i\n", e->reserve_bc);
        OUTLINE "tax = %i\n", e->tax);
        OUTLINE "base_shield = %i\n", e->base_shield);
        OUTLINE "base_comp = %i\n", e->base_comp);
        OUTLINE "base_weapon = %i\n", e->base_weapon);
        OUTLINE "colonist_oper_factories = %i\n", e->colonist_oper_factories);
        OUTLINETBL("tech.percent", TECH_FIELD_NUM, e->tech.percent);
        OUTLINETBL("tech.slider", TECH_FIELD_NUM, e->tech.slider);
        OUTLINETBL("tech.slider_lock", TECH_FIELD_NUM, e->tech.slider_lock);
        OUTLINETBL("tech.investment", TECH_FIELD_NUM, e->tech.investment);
        OUTLINETBL("tech.project", TECH_FIELD_NUM, e->tech.project);
        OUTLINETBL("tech.cost", TECH_FIELD_NUM, e->tech.cost);
        OUTLINETBL("tech.completed", TECH_FIELD_NUM, e->tech.completed);
        OUTLINE "shipdesigns_num = %i\n", e->shipdesigns_num);
        OUTFLUSH();
        for (int i = 0; i < g->galaxy_stars; ++i) {
            text_dump_prefix_add_tbl(tp, "orbit", ".", i);
            OUTLINETBLNON0("ships", e->shipdesigns_num, e->orbit[i].ships);
            text_dump_prefix_del(tp);
        }
        OUTFLUSH();
        for (int i = 0; i < g->players; ++i) {
            if (i != pl) {
                text_dump_prefix_add_tbl(tp, "spyreportfield", "", i);
                OUTLINETBL("", TECH_FIELD_NUM, e->spyreportfield[i]);
                text_dump_prefix_del(tp);
            }
        }
        OUTLINETBL("spyreportyear", g->players, e->spyreportyear);
        OUTLINE "shipi_colony = %i\n", e->shipi_colony);
        OUTLINE "shipi_bomber = %i\n", e->shipi_bomber);
        text_dump_prefix_del(tp);
        OUTFLUSH();
    }
    for (int pl = 0; pl < g->players; ++pl) {
        const shipresearch_t *srd = &(g->srd[pl]);
        const empiretechorbit_t *e = &(g->eto[pl]);
        text_dump_prefix_add_tbl(tp, "srd", ".", pl);
        for (int i = 0; i < g->eto[pl].shipdesigns_num; ++i) {
            text_dump_prefix_add_tbl(tp, "design", ".", i);
            savetype_en_text_sd(&(srd->design[i]), tp);
            text_dump_prefix_del(tp);
        }
        OUTFLUSH();
        for (int f = 0; f < TECH_FIELD_NUM; ++f) {
            text_dump_prefix_add_tbl(tp, "researchlist", "", f);
            for (int t = 0; t < TECH_TIER_NUM; ++t) {
                text_dump_prefix_add_tbl(tp, "", "", t);
                OUTLINETBL("", 3, srd->researchlist[f][t]);
                text_dump_prefix_del(tp);
            }
            text_dump_prefix_del(tp);
            OUTFLUSH();
        }
        for (int f = 0; f < TECH_FIELD_NUM; ++f) {
            text_dump_prefix_add_tbl(tp, "researchcompleted", "", f);
            OUTLINETBL("", e->tech.completed[f], srd->researchcompleted[f]);
            text_dump_prefix_del(tp);
        }
        OUTFLUSH();
        OUTLINETBL("have_reserve_fuel", e->shipdesigns_num, srd->have_reserve_fuel);
        OUTLINETBL("year", e->shipdesigns_num, srd->year);
        OUTLINETBL("shipcount", e->shipdesigns_num, srd->shipcount);
        text_dump_prefix_del(tp);
    }
    OUTFLUSH();
    for (int pl = 0; pl < g->players; ++pl) {
        if (IS_HUMAN(g, pl)) {
            text_dump_prefix_add_tbl(tp, "current_design", ".", pl);
            savetype_en_text_sd(&(g->current_design[pl]), tp);
            text_dump_prefix_del(tp);
        }
    }
    OUTFLUSH();
    {
        const gameevents_t *ev = &(g->evn);
        text_dump_prefix_add(tp, "evn", ".");
        OUTLINE "year = %i // %i\n", ev->year, ev->year + YEAR_BASE);
        OUTLINE "done = %s\n", savetype_en_bv(ev->done, GAME_EVENT_TBL_NUM));
        OUTLINE "have_plague = %i\n", ev->have_plague);
        OUTLINE "plague_player = %i\n", ev->plague_player);
        OUTLINE "plague_planet_i = %i\n", ev->plague_planet_i);
        OUTLINE "plague_val = %i\n", ev->plague_val);
        OUTLINE "have_nova = %i\n", ev->have_nova);
        OUTLINE "nova_player = %i\n", ev->nova_player);
        OUTLINE "nova_planet_i = %i\n", ev->nova_planet_i);
        OUTLINE "nova_years = %i\n", ev->nova_years);
        OUTLINE "nova_val = %i\n", ev->nova_val);
        OUTLINE "have_accident = %i\n", ev->have_accident);
        OUTLINE "accident_planet_i = %i\n", ev->accident_planet_i);
        OUTLINE "have_comet = %i\n", ev->have_comet);
        OUTLINE "comet_player = %i\n", ev->comet_player);
        OUTLINE "comet_planet_i = %i\n", ev->comet_planet_i);
        OUTLINE "comet_years = %i\n", ev->comet_years);
        OUTLINE "comet_hp = %i\n", ev->comet_hp);
        OUTLINE "comet_dmg = %i\n", ev->comet_dmg);
        OUTLINE "have_pirates = %i\n", ev->have_pirates);
        OUTLINE "pirates_planet_i = %i\n", ev->pirates_planet_i);
        OUTLINE "pirates_hp = %i\n", ev->pirates_hp);
        text_dump_prefix_add(tp, "crystal", ".");
        savetype_en_text_monster(&(ev->crystal), tp);
        text_dump_prefix_del(tp);
        text_dump_prefix_add(tp, "amoeba", ".");
        savetype_en_text_monster(&(ev->amoeba), tp);
        text_dump_prefix_del(tp);
        OUTLINE "planet_orion_i = %i\n", ev->planet_orion_i);
        OUTLINE "have_guardian = %i\n", ev->have_guardian);
        OUTLINETBL("home", g->players, ev->home);
        OUTLINE "report_stars = %i\n", ev->report_stars);
        OUTFLUSH();
        for (int pl = 0; pl < g->players; ++pl) {
            text_dump_prefix_add_tbl(tp, "new_ships", "", pl);
            OUTLINETBL("", NUM_SHIPDESIGNS, ev->new_ships[pl]);
            text_dump_prefix_del(tp);
        }
        for (int pl = 0; pl < g->players; ++pl) {
            text_dump_prefix_add_tbl(tp, "spies_caught", "", pl);
            OUTLINETBL("", g->players, ev->spies_caught[pl]);
            text_dump_prefix_del(tp);
        }
        for (int pl = 0; pl < g->players; ++pl) {
            text_dump_prefix_add_tbl(tp, "hmm28e", "", pl);
            OUTLINETBL("", g->players, ev->hmm28e[pl]);
            text_dump_prefix_del(tp);
        }
        for (int pl = 0; pl < g->players; ++pl) {
            if (IS_HUMAN(g, pl)) {
                OUTLINE "help_shown[%i] = %s\n", pl, savetype_en_bv(ev->help_shown[pl], HELP_SHOWN_NUM));
            }
        }
        OUTLINETBL("build_finished_num", g->players, ev->build_finished_num);
        OUTLINETBL("voted", g->players, ev->voted);
        OUTLINETBL("best_ecorestore", g->players, ev->best_ecorestore);
        OUTLINETBL("best_wastereduce", g->players, ev->best_wastereduce);
        OUTLINETBL("best_roboctrl", g->players, ev->best_roboctrl);
        OUTLINETBL("best_terraform", g->players, ev->best_terraform);
        text_dump_prefix_del(tp);
    }
    OUTFLUSH();
    text_dump_prefix_del(tp);
    return save_out_close();
}

#undef OUTTBL
#undef OUTLINETBL
#undef OUTLINETBLNON0
#undef OUTPRE
#undef OUTADD
#undef OUTLINE

/* -------------------------------------------------------------------------- */

static int savetype_de_1oom0(struct game_s *g, const char *fname)
{
    char *sname = (*savename == '\0') ? savename : NULL;
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname));
    return game_save_do_load_fname(fname, sname, g);
}

static int savetype_en_1oom0(struct game_s *g, const char *fname)
{
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname ? fname : "(null)"));
    return game_save_do_save_fname(fname, savename, g);
}

/* -------------------------------------------------------------------------- */

static void savegame_usage(void)
{
    log_message_direct("Usage:\n    1oom_saveconv [OPTIONS] INPUT [OUTPUT]\n");
}

void (*main_usage)(void) = savegame_usage;

int main_handle_option(const char *argv)
{
    if (main_fname_num < 2) {
        fnames[main_fname_num++] = argv;
        return 0;
    } else {
        log_error("too many parameters!\n");
        return -1;
    }
}

static int saveconv_opt_typeo(char **argv, void *var)
{
    int i = 0;
    while (savetype_match[i].str) {
        if (strcmp(savetype_match[i].str, argv[1]) == 0) {
            savetypeo = savetype_match[i].type;
            LOG_DEBUG((1, "%s: set output type to '%s' -> '%s'\n", __func__, argv[1], savetype[savetypeo].name));
            return 0;
        }
        ++i;
    }
    log_error("unknown type '%s'\n", argv[1]);
    return -1;
}

static int saveconv_opt_typei(char **argv, void *var)
{
    int i = 0;
    while (savetype_match[i].str) {
        if (strcmp(savetype_match[i].str, argv[1]) == 0) {
            if (savetype[savetypei].decode) {
                savetypei = savetype_match[i].type;
                LOG_DEBUG((1, "%s: set input type to '%s' -> '%s'\n", __func__, argv[1], savetype[savetypei].name));
                return 0;
            } else {
                log_error("unknown type '%s' is not a valid input type\n", savetype[savetypei].name);
                return -1;
            }
        }
        ++i;
    }
    log_error("unknown type '%s'\n", argv[1]);
    return -1;
}

static int saveconv_opt_cmoo_en(char **argv, void *var)
{
    opt_use_configmoo = true;
    LOG_DEBUG((1, "%s\n", __func__));
    return 0;
}

static int saveconv_opt_sname(char **argv, void *var)
{
    strncpy(savename, argv[1], SAVE_NAME_LEN);
    savename[SAVE_NAME_LEN - 1] = '\0';
    LOG_DEBUG((1, "%s: set save name '%s'\n", __func__, savename));
    return 0;
}

const struct cmdline_options_s main_cmdline_options_early[] = {
    { "-i", 1,
      saveconv_opt_typei, 0,
      "INTYPE", "Input type:\n"
                 "  m   - MOO1 v1.3\n"
                 "  1   - 1oom save version 0\n"
                 "  s   - smart: autodetect (default)"
    },
    { "-o", 1,
      saveconv_opt_typeo, 0,
      "OUTTYPE", "Output type:\n"
                 "  m   - MOO1 v1.3\n"
                 "  1   - 1oom save version 0\n"
                 "  s   - smart: in old/new -> out new/old (default)\n"
                 "  t   - text (output only)\n"
                 "  d   - dummy (no output)"
    },
    { "-cmoo", 0,
      saveconv_opt_cmoo_en, 0,
      NULL, "Enable CONFIG.MOO use"
    },
    { "-n", 1,
      saveconv_opt_sname, 0,
      "NAME", "Set save name"
    },
    { 0, 0, 0, 0, 0, 0 }
};

const struct cmdline_options_s main_cmdline_options[] = {
    { 0, 0, 0, 0, 0, 0 }
};

/* -------------------------------------------------------------------------- */

static int main_early_init(void)
{
    struct game_s *g;
    struct game_aux_s *gaux;
    if (os_early_init()) {
        return 1;
    }
    gameptr = g = lib_malloc(sizeof(struct game_s));
    g->gaux = gaux = lib_malloc(sizeof(struct game_aux_s));
    gaux->savenamebuflen = FSDEV_PATH_MAX;
    gaux->savenamebuf = lib_malloc(gaux->savenamebuflen);
    gaux->savebuflen = sizeof(struct game_s) + 64;
    gaux->savebuf = lib_malloc(gaux->savebuflen);
    save2buf = lib_malloc(0x10000);
    return 0;
}

static int main_init(void)
{
    if (os_init()) {
        return 1;
    }
    return 0;
}

static void main_shutdown(void)
{
    lib_free(gameptr->gaux->savenamebuf);
    lib_free(gameptr->gaux->savebuf);
    lib_free(gameptr->gaux);
    lib_free(gameptr);
    lib_free(save2buf);
    os_shutdown();
}

int main_do(void)
{
    int res;
    uint32_t v;
    const char *fname;
    if (main_fname_num == 0) {
        options_show_usage();
        return 0;
    }
    fname = fnames[0];
    if ((util_parse_number(fname, &v)) && (v >= 1) && (v <= NUM_ALL_SAVES)) {
        game_save_get_slot_fname(gameptr->gaux->savenamebuf, gameptr->gaux->savenamebuflen, v - 1);
        fname = gameptr->gaux->savenamebuf;
    }
    LOG_DEBUG((1, "%s: decode type '%s' file '%s'\n", __func__, savetype[savetypei].name, fname));
    res = savetype[savetypei].decode(gameptr, fname);
    if (res < 0) {
        log_error("decoding file '%s' failed\n", fname);
        return 1;
    }
    if (!savetype[savetypeo].encode) {
        LOG_DEBUG((1, "%s: encode type '%s' no callback\n", __func__, savetype[savetypeo].name));
        if (main_fname_num == 2) {
            log_error("output filename given for type '%s' which has no output\n", savetype[savetypeo].name);
            return 1;
        }
        return 0;
    }
    fname = fnames[1];
    if (fname == 0) {
        if (!(savetype[savetypeo].flags & SAVETYPE_F_OPTOUT)) {
            log_error("output filename missing\n");
            return 1;
        }
    } else if ((util_parse_number(fname, &v)) && (v >= 1) && (v <= NUM_ALL_SAVES)) {
        game_save_get_slot_fname(gameptr->gaux->savenamebuf, gameptr->gaux->savenamebuflen, v - 1);
        fname = gameptr->gaux->savenamebuf;
    }
    LOG_DEBUG((1, "%s: encode type '%s' file '%s'\n", savetype[savetypeo].name, fname ? fname : "(null)"));
    if (savename[0] == '\0') {
        strcpy(savename, "saveconv");
    }
    res = savetype[savetypeo].encode(gameptr, fname);
    if (res < 0) {
        log_error("encoding file '%s' failed\n", fname ? fname : "(null)");
        return 1;
    }
    return 0;
}

/* -------------------------------------------------------------------------- */

int main_1oom(int argc, char **argv)
{
    if (main_early_init()) {
        return 1;
    }
    if (options_parse_early(argc, argv)) {
        return 1;
    }
    atexit(main_shutdown);
    if (main_init()) {
        return 2;
    }
    if (options_parse(argc, argv)) {
        return 3;
    }
    return main_do();
}
