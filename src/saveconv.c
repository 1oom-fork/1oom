#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "bits.h"
#include "comp.h"
#include "hw.h"
#include "game/game.h"
#include "game/game_cfg.h"
#include "game/game_save.h"
#include "game/game_str.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "save/save.h"
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
static int savetype_en_moo13(const struct game_s *g, const char *fname);
static int savetype_de_1oom0(struct game_s *g, const char *fname);
static int savetype_en_1oom0(const struct game_s *g, const char *fname);
static bool savetype_is_text(const struct game_s *g, const char *fname);
static int savetype_de_text(struct game_s *g, const char *fname);
static int savetype_en_text(const struct game_s *g, const char *fname);

static const struct {
    const char *name;
    int (*decode)(struct game_s *g, const char *fname); /* to native */
    int (*encode)(const struct game_s *g, const char *fname);
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
        0, SAVETYPE_NATIVE
    },
    { /* SAVETYPE_1OOM0 */
        "1oom save version 0",
        savetype_de_1oom0,
        savetype_en_1oom0,
        0, SAVETYPE_MOO13
    },
    { /* SAVETYPE_TEXT  */
        "text",
        savetype_de_text,
        savetype_en_text,
        SAVETYPE_F_OPTOUT, SAVETYPE_NATIVE
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
    if (libsave_is_1oom(fname)) {
        savetypei = SAVETYPE_NATIVE;
        res = savetype[SAVETYPE_NATIVE].decode(g, fname);
    } else if (libsave_is_moo13(fname)) {
        savetypei = SAVETYPE_MOO13;
        res = savetype_de_moo13(g, fname);
    } else if (savetype_is_text(g, fname)) {
        savetypei = SAVETYPE_TEXT;
        res = savetype_de_text(g, fname);
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

static int savetype_de_moo13(struct game_s *g, const char *fname)
{
    if (libsave_moo13_load_do(fname, g) != 0) {
        return -1;
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
            if (util_file_try_load_len(tryname, cmoobuf, CMOO_LEN) <= 0) {
                tryname = "CONFIG.MOO";
                if (dir) {
                    lib_free(fullname);
                    tryname = fullname = util_concat(dir, FSDEV_DIR_SEP_STR, tryname, NULL);
                }
                if (util_file_try_load_len(tryname, cmoobuf, CMOO_LEN) <= 0) {
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

static int savetype_en_moo13(const struct game_s *g, const char *fname)
{
    if (libsave_moo13_save_do(fname, g) != 0) {
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
            if (util_file_try_load_len(tryname, cmoobuf, CMOO_LEN) <= 0) {
                tryname = "CONFIG.MOO";
                if (dir) {
                    lib_free(fullname);
                    tryname = fullname = util_concat(dir, FSDEV_DIR_SEP_STR, tryname, NULL);
                }
                if (util_file_try_load_len(tryname, cmoobuf, CMOO_LEN) <= 0) {
                    found = false;
                }
            }
            if (found) {
                strncpy((char *)&cmoobuf[0x22 + 20 * (savei - 1)], savename, 20);
                cmoobuf[0x22 + 20 * (savei - 1) + 20 - 1] = '\0';
                cmoobuf[0x16 + 2 * (savei - 1)] = 1;    /* set have save flag */
                LOG_DEBUG((1, "set '%s' slot %i '%s'\n", tryname, savei, savename));
                if (util_file_save(tryname, cmoobuf, CMOO_LEN)) {
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

typedef enum {
    GAME_INTROS_T_VALUE,
    GAME_INTROS_T_STR,
    GAME_INTROS_T_BV,
    GAME_INTROS_T_SUB
} game_intros_value_t;

struct game_intros_s {
    const char *str;
    game_intros_value_t type;
    int offs;
    int size;
    int len;
    const struct game_intros_s *sub;
};

#define GAME_INTROS_OFF(_r_, _s_) (((const uint8_t *)&(((const struct _r_ *)1024)->_s_)) - ((const uint8_t *)1024))
#define GAME_INTROS_VSIZE(_r_, _s_)  sizeof((&((struct _r_ *)1024)->_s_)[0])
#define GAME_INTROS_VLEN(_r_, _s_)   (sizeof(((struct _r_ *)1024)->_s_) / GAME_INTROS_VSIZE(_r_, _s_))
#define GAME_INTROS_TSIZE(_r_, _s_)  sizeof((((struct _r_ *)1024)->_s_)[0])
#define GAME_INTROS_TLEN(_r_, _s_)   (sizeof(((struct _r_ *)1024)->_s_) / GAME_INTROS_TSIZE(_r_, _s_))
#define GAME_INTROS_VAL(_r_, _s_)   { #_s_, GAME_INTROS_T_VALUE, GAME_INTROS_OFF(_r_, _s_), GAME_INTROS_VSIZE(_r_, _s_), GAME_INTROS_VLEN(_r_, _s_), 0 }
#define GAME_INTROS_TBL(_r_, _s_)   { #_s_, GAME_INTROS_T_VALUE, GAME_INTROS_OFF(_r_, _s_), GAME_INTROS_TSIZE(_r_, _s_), GAME_INTROS_TLEN(_r_, _s_), 0 }
#define GAME_INTROS_LTBL(_r_, _s_)  { "", GAME_INTROS_T_VALUE, 0, GAME_INTROS_TSIZE(_r_, _s_), GAME_INTROS_TLEN(_r_, _s_), 0 }
#define GAME_INTROS_BV(_r_, _s_, _l_)    { #_s_, GAME_INTROS_T_BV, GAME_INTROS_OFF(_r_, _s_), _l_, 1, 0 }
#define GAME_INTROS_BVT(_r_, _s_, _l_, _n_)    { #_s_, GAME_INTROS_T_BV, GAME_INTROS_OFF(_r_, _s_), _l_, _n_, 0 }
#define GAME_INTROS_STR(_r_, _s_, _z_, _l_)    { #_s_, GAME_INTROS_T_STR, GAME_INTROS_OFF(_r_, _s_), _z_, _l_, 0 }
#define GAME_INTROS_SUB(_r_, _s_, _p_)  { #_s_, GAME_INTROS_T_SUB, GAME_INTROS_OFF(_r_, _s_), GAME_INTROS_TSIZE(_r_, _s_), GAME_INTROS_TLEN(_r_, _s_), _p_ }
#define GAME_INTROS_SUBV(_r_, _s_, _p_) { #_s_, GAME_INTROS_T_SUB, GAME_INTROS_OFF(_r_, _s_), GAME_INTROS_VSIZE(_r_, _s_), GAME_INTROS_VLEN(_r_, _s_), _p_ }
#define GAME_INTROS_END     { 0, 0, 0, 0, 0, 0 }

static const struct game_intros_s game_intros_planet[] = {
    GAME_INTROS_STR(planet_s, name, PLANET_NAME_LEN, 1),
    GAME_INTROS_VAL(planet_s, x),
    GAME_INTROS_VAL(planet_s, y),
    GAME_INTROS_VAL(planet_s, star_type),
    GAME_INTROS_VAL(planet_s, look),
    GAME_INTROS_VAL(planet_s, frame),
    GAME_INTROS_VAL(planet_s, rocks),
    GAME_INTROS_VAL(planet_s, max_pop1),
    GAME_INTROS_VAL(planet_s, max_pop2),
    GAME_INTROS_VAL(planet_s, max_pop3),
    GAME_INTROS_VAL(planet_s, type),
    GAME_INTROS_VAL(planet_s, battlebg),
    GAME_INTROS_VAL(planet_s, infogfx),
    GAME_INTROS_VAL(planet_s, growth),
    GAME_INTROS_VAL(planet_s, special),
    GAME_INTROS_VAL(planet_s, owner),
    GAME_INTROS_VAL(planet_s, prev_owner),
    GAME_INTROS_VAL(planet_s, claim),
    GAME_INTROS_VAL(planet_s, waste),
    GAME_INTROS_BV(planet_s, explored, PLAYER_NUM),
    GAME_INTROS_VAL(planet_s, bc_to_ecoproj),
    GAME_INTROS_VAL(planet_s, bc_to_ship),
    GAME_INTROS_VAL(planet_s, bc_to_factory),
    GAME_INTROS_VAL(planet_s, reserve),
    GAME_INTROS_VAL(planet_s, pop),
    GAME_INTROS_VAL(planet_s, pop_prev),
    GAME_INTROS_VAL(planet_s, factories),
    GAME_INTROS_TBL(planet_s, slider),
    GAME_INTROS_TBL(planet_s, slider_lock),
    GAME_INTROS_VAL(planet_s, buildship),
    GAME_INTROS_VAL(planet_s, reloc),
    GAME_INTROS_VAL(planet_s, missile_bases),
    GAME_INTROS_VAL(planet_s, bc_to_base),
    GAME_INTROS_VAL(planet_s, bc_upgrade_base),
    GAME_INTROS_VAL(planet_s, have_stargate),
    GAME_INTROS_VAL(planet_s, shield),
    GAME_INTROS_VAL(planet_s, bc_to_shield),
    GAME_INTROS_VAL(planet_s, trans_num),
    GAME_INTROS_VAL(planet_s, trans_dest),
    GAME_INTROS_VAL(planet_s, pop_tenths),
    GAME_INTROS_VAL(planet_s, pop_oper_fact),
    GAME_INTROS_VAL(planet_s, bc_to_refit),
    GAME_INTROS_VAL(planet_s, rebels),
    GAME_INTROS_VAL(planet_s, unrest),
    GAME_INTROS_VAL(planet_s, unrest_reported),
    GAME_INTROS_BV(planet_s, finished, FINISHED_NUM),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_seen[] = {
    GAME_INTROS_VAL(seen_s, owner),
    GAME_INTROS_VAL(seen_s, pop),
    GAME_INTROS_VAL(seen_s, bases),
    GAME_INTROS_VAL(seen_s, factories),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_seen0[] = {
    { "", GAME_INTROS_T_SUB, 0, sizeof(struct seen_s), PLANETS_MAX, game_intros_seen },
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_enroute[] = {
    GAME_INTROS_VAL(fleet_enroute_s, owner),
    GAME_INTROS_VAL(fleet_enroute_s, x),
    GAME_INTROS_VAL(fleet_enroute_s, y),
    GAME_INTROS_VAL(fleet_enroute_s, dest),
    GAME_INTROS_VAL(fleet_enroute_s, speed),
    GAME_INTROS_TBL(fleet_enroute_s, ships),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_transport[] = {
    GAME_INTROS_VAL(transport_s, owner),
    GAME_INTROS_VAL(transport_s, x),
    GAME_INTROS_VAL(transport_s, y),
    GAME_INTROS_VAL(transport_s, dest),
    GAME_INTROS_VAL(transport_s, speed),
    GAME_INTROS_VAL(transport_s, pop),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_tech[] = {
    GAME_INTROS_TBL(techdata_s, percent),
    GAME_INTROS_TBL(techdata_s, slider),
    GAME_INTROS_TBL(techdata_s, slider_lock),
    GAME_INTROS_TBL(techdata_s, investment),
    GAME_INTROS_TBL(techdata_s, project),
    GAME_INTROS_TBL(techdata_s, cost),
    GAME_INTROS_TBL(techdata_s, completed),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_orbit[] = {
    GAME_INTROS_TBL(fleet_orbit_s, ships),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_spyreportfield[] = {
    GAME_INTROS_LTBL(empiretechorbit_s, spyreportfield[0]),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_eto[] = {
    GAME_INTROS_VAL(empiretechorbit_s, race),
    GAME_INTROS_VAL(empiretechorbit_s, banner),
    GAME_INTROS_VAL(empiretechorbit_s, trait1),
    GAME_INTROS_VAL(empiretechorbit_s, trait2),
    GAME_INTROS_VAL(empiretechorbit_s, ai_p3_countdown),
    GAME_INTROS_VAL(empiretechorbit_s, ai_p2_countdown),
    GAME_INTROS_BV(empiretechorbit_s, within_frange, PLAYER_NUM),
    GAME_INTROS_TBL(empiretechorbit_s, relation1),
    GAME_INTROS_TBL(empiretechorbit_s, relation2),
    GAME_INTROS_TBL(empiretechorbit_s, diplo_type),
    GAME_INTROS_TBL(empiretechorbit_s, diplo_val),
    GAME_INTROS_TBL(empiretechorbit_s, diplo_p1),
    GAME_INTROS_TBL(empiretechorbit_s, diplo_p2),
    GAME_INTROS_TBL(empiretechorbit_s, trust),
    GAME_INTROS_TBL(empiretechorbit_s, broken_treaty),
    GAME_INTROS_TBL(empiretechorbit_s, blunder),
    GAME_INTROS_TBL(empiretechorbit_s, tribute_field),
    GAME_INTROS_TBL(empiretechorbit_s, tribute_tech),
    GAME_INTROS_TBL(empiretechorbit_s, mood_treaty),
    GAME_INTROS_TBL(empiretechorbit_s, mood_trade),
    GAME_INTROS_TBL(empiretechorbit_s, mood_tech),
    GAME_INTROS_TBL(empiretechorbit_s, mood_peace),
    GAME_INTROS_TBL(empiretechorbit_s, treaty),
    GAME_INTROS_TBL(empiretechorbit_s, trade_bc),
    GAME_INTROS_TBL(empiretechorbit_s, trade_percent),
    GAME_INTROS_TBL(empiretechorbit_s, spymode_next),
    GAME_INTROS_TBL(empiretechorbit_s, offer_field),
    GAME_INTROS_TBL(empiretechorbit_s, offer_tech),
    GAME_INTROS_TBL(empiretechorbit_s, offer_bc),
    GAME_INTROS_TBL(empiretechorbit_s, hated),
    GAME_INTROS_TBL(empiretechorbit_s, mutual_enemy),
    GAME_INTROS_TBL(empiretechorbit_s, hatred),
    GAME_INTROS_TBL(empiretechorbit_s, have_met),
    GAME_INTROS_TBL(empiretechorbit_s, trade_established_bc),
    GAME_INTROS_TBL(empiretechorbit_s, spying),
    GAME_INTROS_TBL(empiretechorbit_s, spyfund),
    GAME_INTROS_TBL(empiretechorbit_s, spymode),
    GAME_INTROS_VAL(empiretechorbit_s, security),
    GAME_INTROS_TBL(empiretechorbit_s, spies),
    GAME_INTROS_VAL(empiretechorbit_s, reserve_bc),
    GAME_INTROS_VAL(empiretechorbit_s, tax),
    GAME_INTROS_VAL(empiretechorbit_s, base_shield),
    GAME_INTROS_VAL(empiretechorbit_s, base_comp),
    GAME_INTROS_VAL(empiretechorbit_s, base_weapon),
    GAME_INTROS_VAL(empiretechorbit_s, colonist_oper_factories),
    GAME_INTROS_SUBV(empiretechorbit_s, tech, game_intros_tech),
    GAME_INTROS_VAL(empiretechorbit_s, shipdesigns_num),
    GAME_INTROS_SUB(empiretechorbit_s, orbit, game_intros_orbit),
    GAME_INTROS_SUB(empiretechorbit_s, spyreportfield, game_intros_spyreportfield),
    GAME_INTROS_TBL(empiretechorbit_s, spyreportyear),
    GAME_INTROS_VAL(empiretechorbit_s, shipi_colony),
    GAME_INTROS_VAL(empiretechorbit_s, shipi_bomber),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_design[] = {
    GAME_INTROS_STR(shipdesign_s, name, SHIP_NAME_LEN, 1),
    GAME_INTROS_VAL(shipdesign_s, cost),
    GAME_INTROS_VAL(shipdesign_s, space),
    GAME_INTROS_VAL(shipdesign_s, hull),
    GAME_INTROS_VAL(shipdesign_s, look),
    GAME_INTROS_TBL(shipdesign_s, wpnt),
    GAME_INTROS_TBL(shipdesign_s, wpnn),
    GAME_INTROS_VAL(shipdesign_s, engine),
    GAME_INTROS_VAL(shipdesign_s, engines),
    GAME_INTROS_TBL(shipdesign_s, special),
    GAME_INTROS_VAL(shipdesign_s, shield),
    GAME_INTROS_VAL(shipdesign_s, jammer),
    GAME_INTROS_VAL(shipdesign_s, comp),
    GAME_INTROS_VAL(shipdesign_s, armor),
    GAME_INTROS_VAL(shipdesign_s, man),
    GAME_INTROS_VAL(shipdesign_s, hp),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_researchlist[] = {
    GAME_INTROS_LTBL(shipresearch_s, researchlist[0][0]),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_researchlist0[] = {
    { "", GAME_INTROS_T_SUB, 0, GAME_INTROS_TSIZE(shipresearch_s, researchlist[0]), TECH_TIER_NUM, game_intros_researchlist },
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_researchcompleted[] = {
    GAME_INTROS_LTBL(shipresearch_s, researchcompleted[0]),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_srd[] = {
    GAME_INTROS_SUB(shipresearch_s, design, game_intros_design),
    GAME_INTROS_SUB(shipresearch_s, researchlist, game_intros_researchlist0),
    GAME_INTROS_SUB(shipresearch_s, researchcompleted, game_intros_researchcompleted),
    GAME_INTROS_TBL(shipresearch_s, have_reserve_fuel),
    GAME_INTROS_TBL(shipresearch_s, year),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_monster[] = {
    GAME_INTROS_VAL(monster_s, exists),
    GAME_INTROS_VAL(monster_s, x),
    GAME_INTROS_VAL(monster_s, y),
    GAME_INTROS_VAL(monster_s, killer),
    GAME_INTROS_VAL(monster_s, dest),
    GAME_INTROS_VAL(monster_s, counter),
    GAME_INTROS_VAL(monster_s, nuked),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_new_ships[] = {
    GAME_INTROS_LTBL(gameevents_s, new_ships[0]),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_spies_caught[] = {
    GAME_INTROS_LTBL(gameevents_s, spies_caught[0]),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_ceasefire[] = {
    GAME_INTROS_LTBL(gameevents_s, ceasefire[0]),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_help_shown[] = {
    { "", GAME_INTROS_T_BV, 0, HELP_SHOWN_NUM, 1, 0 },
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_evn[] = {
    GAME_INTROS_VAL(gameevents_s, year),
    GAME_INTROS_BV(gameevents_s, done, GAME_EVENT_TBL_NUM),
    GAME_INTROS_VAL(gameevents_s, have_plague),
    GAME_INTROS_VAL(gameevents_s, plague_player),
    GAME_INTROS_VAL(gameevents_s, plague_planet_i),
    GAME_INTROS_VAL(gameevents_s, plague_val),
    GAME_INTROS_VAL(gameevents_s, have_nova),
    GAME_INTROS_VAL(gameevents_s, nova_player),
    GAME_INTROS_VAL(gameevents_s, nova_planet_i),
    GAME_INTROS_VAL(gameevents_s, nova_years),
    GAME_INTROS_VAL(gameevents_s, nova_val),
    GAME_INTROS_VAL(gameevents_s, have_accident),
    GAME_INTROS_VAL(gameevents_s, accident_planet_i),
    GAME_INTROS_VAL(gameevents_s, have_comet),
    GAME_INTROS_VAL(gameevents_s, comet_player),
    GAME_INTROS_VAL(gameevents_s, comet_planet_i),
    GAME_INTROS_VAL(gameevents_s, comet_years),
    GAME_INTROS_VAL(gameevents_s, comet_hp),
    GAME_INTROS_VAL(gameevents_s, comet_dmg),
    GAME_INTROS_VAL(gameevents_s, have_pirates),
    GAME_INTROS_VAL(gameevents_s, pirates_planet_i),
    GAME_INTROS_VAL(gameevents_s, pirates_hp),
    GAME_INTROS_SUBV(gameevents_s, crystal, game_intros_monster),
    GAME_INTROS_SUBV(gameevents_s, amoeba, game_intros_monster),
    GAME_INTROS_VAL(gameevents_s, planet_orion_i),
    GAME_INTROS_VAL(gameevents_s, have_guardian),
    GAME_INTROS_TBL(gameevents_s, home),
    GAME_INTROS_VAL(gameevents_s, report_stars),
    GAME_INTROS_SUB(gameevents_s, new_ships, game_intros_new_ships),
    GAME_INTROS_SUB(gameevents_s, spies_caught, game_intros_spies_caught),
    GAME_INTROS_SUB(gameevents_s, ceasefire, game_intros_ceasefire),
    GAME_INTROS_SUB(gameevents_s, help_shown, game_intros_help_shown),
    GAME_INTROS_TBL(gameevents_s, build_finished_num),
    GAME_INTROS_TBL(gameevents_s, voted),
    GAME_INTROS_TBL(gameevents_s, best_ecorestore),
    GAME_INTROS_TBL(gameevents_s, best_wastereduce),
    GAME_INTROS_TBL(gameevents_s, best_roboctrl),
    GAME_INTROS_TBL(gameevents_s, best_terraform),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_nebula_coord[] = {
    GAME_INTROS_LTBL(game_s, nebula_x0[0]),
    GAME_INTROS_END
};

static const struct game_intros_s game_intros_root[] = {
    GAME_INTROS_VAL(game_s, players),
    GAME_INTROS_BV(game_s, is_ai, PLAYER_NUM),
    GAME_INTROS_VAL(game_s, active_player),
    GAME_INTROS_VAL(game_s, difficulty),
    GAME_INTROS_VAL(game_s, galaxy_size),
    GAME_INTROS_VAL(game_s, galaxy_w),
    GAME_INTROS_VAL(game_s, galaxy_h),
    GAME_INTROS_VAL(game_s, galaxy_stars),
    GAME_INTROS_VAL(game_s, galaxy_maxx),
    GAME_INTROS_VAL(game_s, galaxy_maxy),
    GAME_INTROS_VAL(game_s, galaxy_seed),
    GAME_INTROS_VAL(game_s, seed),
    GAME_INTROS_VAL(game_s, year),
    GAME_INTROS_VAL(game_s, enroute_num),
    GAME_INTROS_VAL(game_s, transport_num),
    GAME_INTROS_VAL(game_s, end),
    GAME_INTROS_VAL(game_s, winner),
    GAME_INTROS_VAL(game_s, guardian_killer),
    GAME_INTROS_VAL(game_s, election_held),
    GAME_INTROS_VAL(game_s, nebula_num),
    GAME_INTROS_TBL(game_s, nebula_type),
    GAME_INTROS_TBL(game_s, nebula_x),
    GAME_INTROS_TBL(game_s, nebula_y),
    GAME_INTROS_SUB(game_s, nebula_x0, game_intros_nebula_coord),
    GAME_INTROS_SUB(game_s, nebula_x1, game_intros_nebula_coord),
    GAME_INTROS_SUB(game_s, nebula_y0, game_intros_nebula_coord),
    GAME_INTROS_SUB(game_s, nebula_y1, game_intros_nebula_coord),
    GAME_INTROS_STR(game_s, emperor_names, EMPEROR_NAME_LEN, PLAYER_NUM),
    GAME_INTROS_TBL(game_s, planet_focus_i),
    GAME_INTROS_SUB(game_s, planet, game_intros_planet),
    GAME_INTROS_SUB(game_s, seen, game_intros_seen0),
    GAME_INTROS_SUB(game_s, enroute, game_intros_enroute),
    GAME_INTROS_SUB(game_s, transport, game_intros_transport),
    GAME_INTROS_SUB(game_s, eto, game_intros_eto),
    GAME_INTROS_SUB(game_s, srd, game_intros_srd),
    GAME_INTROS_SUB(game_s, current_design, game_intros_design),
    GAME_INTROS_SUBV(game_s, evn, game_intros_evn),
    GAME_INTROS_END
};

static char *savetype_de_text_tok(char *p)
{
    char c;
    while (((c = *p) != '\0') && (c != ',') && (c != ' ')) {
        ++p;
    }
    *p = 0;
    if (c == '\0') {
        return p;
    }
    while (p[1] == ' ') {
        ++p;
    }
    return p + 1;
}

static char *savetype_de_text_parse_right(struct game_s *g, const char *fname, char *buf, int lnum, int offs, const struct game_intros_s *gitbl, int braces)
{
    const struct game_intros_s *gi = gitbl;
    char *p = buf;
    char c;
    int offs_base = offs;
again:
    while ((c = *p) == ' ') {
        ++p;
    }
    buf = p;
    if (c == '\0') {
        return p;
    } else if ((c == '/') && (p[1] == '/')) {
handle_comment:
        *p = '\0';
        return p;
    } else if (c == '"') {
        char *q;
        if (gi->type != GAME_INTROS_T_STR) {
            log_error("'%s' type %i does not accept strings (line %i)\n", fname, gi->type, lnum);
            return 0;
        }
        q = ((char *)(((uint8_t *)g) + offs));
        ++p;
        for (int i = 0; (i < (gi->size - 1)) && (*p != '"'); ++i) {
            *q++ = *p++;
        }
        *q++ = '\0';
        if (*p != '"') {
            log_error("'%s' too long string on line %i\n", fname, lnum);
            return 0;
        }
        return ++p;
    } else if (c == '{') {
        return savetype_de_text_parse_right(g, fname, p + 1, lnum, offs, gi, braces + 1);
    } else if (c == '}') {
handle_brace_close:
        if (braces) {
            return p + 1;
        } else {
            log_error("'%s' unexpected } on line %i\n", fname, lnum);
            return 0;
        }
    } else if (c == '.') {
handle_dot:
        /*
        if (gi->type != GAME_INTROS_T_SUB) {
            log_error("'%s' type %i does not accept .vars (line %i)\n", fname, gi->type, lnum);
            return 0;
        }
        */
        buf = ++p;
        while ((((c = *p) >= 'a') && (c <= 'z')) || (c == '_') || ((c >= '0') && (c <= '9'))) {
            ++p;
        }
        c = *p;
        *p++ = 0;
        for (gi = gitbl; gi->str != NULL; ++gi) {
            if (strcmp(gi->str, buf) == 0) {
                break;
            }
        }
        if (gi->str == NULL) {
            log_error("'%s' could not find token '%s' on line %i\n", fname, buf, lnum);
            return 0;
        }
        offs = offs_base + gi->offs;
        if (c == '[') {
            if (((c = *p++) != ']') || (*p++ != ' ')) {
                log_error("'%s' invalid '[] ' on line %i\n", fname, lnum);
                return 0;
            }
        }
        if (((c = *p++) != '=') || ((c = *p++) != ' ')) {
            log_error("'%s' invalid = on line %i\n", fname, lnum);
            return 0;
        }
        buf = p;
        goto again;
    } else if (((c >= '0') && (c <= '9')) || (c == '-')) {
        if ((c == '0') && (p[1] == 'b')) {
            int n = 0;
            if (gi->type != GAME_INTROS_T_BV) {
                log_error("'%s' type %i does not accept bool vectors (line %i)\n", fname, gi->type, lnum);
                return 0;
            }
            p += 2;
            buf = p;
            while ((p[0] == '0') || (p[0] == '1')) {
                ++n;
                ++p;
            }
            if ((p[0] == '\0') || (p[0] == ' ')) {
                p[0] = '\0';
            } else {
                log_error("'%s' unexpected char 0x%02x on line %i\n", fname, p[0], lnum);
                return 0;
            }
            if (n > gi->size) {
                log_error("'%s' bool vector size %i > max %i\n", fname, n, gi->size);
                return 0;
            } else {
                uint8_t *bv = ((uint8_t *)(((uint8_t *)g) + offs));
                buf = p;
                --p;
                for (int i = 0; i < n; ++i) {
                    if (*p-- == '1') {
                        BOOLVEC_SET1(bv, i);
                    }
                }
                return buf;
            }
        } else {
            int n;
            n = 0;
            if (gi->type != GAME_INTROS_T_VALUE) {
                log_error("'%s' type %i does not accept numbers (line %i)\n", fname, gi->type, lnum);
                return 0;
            }
            do {
                uint32_t v;
                if ((*p == '/') && (p[1] == '/')) {
                    goto handle_comment;
                }
                if (*p == '}') {
                    goto handle_brace_close;
                }
                if (*p == '.') {
                    goto handle_dot;
                }
                p = savetype_de_text_tok(p);
                if (!util_parse_number(buf, &v)) {
                    log_error("'%s' invalid number on line %i\n", fname, lnum);
                    return 0;
                }
                if (n >= gi->len) {
                    log_error("'%s' too many values (%i) on line %i for '%s'\n", fname, n, lnum, gi->str);
                    return 0;
                }
                switch (gi->size) {
                    case 1: *((uint8_t *)(((uint8_t *)g) + offs)) = v; break;
                    case 2: *((uint16_t *)(((uint8_t *)g) + offs)) = v; break;
                    case 4: *((uint32_t *)(((uint8_t *)g) + offs)) = v; break;
                    case 8: *((uint64_t *)(((uint8_t *)g) + offs)) = v; break;
                    default:
                        log_error("'%s' invalid var size %i on line %i\n", fname, gi->size, lnum);
                        return 0;
                }
                offs += gi->size;
                ++n;
                buf = p;
            } while (*p != 0);
        }
    } else {
        log_error("'%s' unexpected char 0x%02x on line %i\n", fname, c, lnum);
        return 0;
    }
    return p;
}

static int savetype_de_text_parse_line(struct game_s *g, const char *fname, char *buf, int lnum)
{
    int offs = -1;
    char *p = buf;
    const struct game_intros_s *gitbl = game_intros_root;
    while (1) {
        const struct game_intros_s *gi;
        char c, cend;
        bool have_sub;
        have_sub = false;
        p = buf;
        while ((((c = *p) >= 'a') && (c <= 'z')) || (c == '_') || ((c >= '0') && (c <= '9'))) {
            ++p;
        }
        cend = c;
        *p++ = 0;
        if (c == '\0') {
            log_error("'%s' unexpected 0 on line %i\n", fname, lnum);
            return -1;
        }
        gi = NULL;
        if ((buf[0] >= 'a') && (buf[0] <= 'z')) {
            for (gi = gitbl; gi->str != NULL; ++gi) {
                if (strcmp(gi->str, buf) == 0) {
                    break;
                }
            }
            if (gi->str == NULL) {
                log_error("'%s' could not find token '%s' on line %i\n", fname, buf, lnum);
                return -1;
            }
            if (offs < 0) {
                offs = gi->offs;
            } else {
                offs += gi->offs;
            }
        }
        while (c == '[') {
            uint32_t v;
            char *numstr;
            numstr = p;
            while (((c = *p) >= '0') && (c <= '9')) {
                ++p;
            }
            if (c != ']') {
                log_error("'%s' missing ']' on line %i\n", fname, lnum);
                return -1;
            }
            *p++ = 0;
            c = *p++;
            if (*numstr == 0) {
                break;
            } else {
                if (!util_parse_number(numstr, &v)) {
                    log_error("'%s' parsing index on line %i\n", fname, lnum);
                    return -1;
                }
                if ((offs < 0) || (gi == NULL)) {
                    log_error("'%s' indexing without variable on line %i\n", fname, lnum);
                    return -1;
                }
                if (v >= gi->len) {
                    log_error("'%s' index %i >= len %i on line %i\n", fname, v, gi->len, lnum);
                    return -1;
                }
                offs += gi->size * v;
                if (gi->sub) {
                    gitbl = gi = gi->sub;
                    have_sub = true;
                }
            }
        }
        if (c == '.') {
            if (gi == NULL) {
                log_error("'%s' . without name on line %i\n", fname, lnum);
                return -1;
            }
            if ((!have_sub) && gi->sub) {
                gitbl = gi->sub;
                have_sub = true;
            }
            if (!have_sub) {
                log_error("'%s' . without name on line %i\n", fname, lnum);
                return -1;
            }
            buf = p;
            continue;
        }
        if (c == ' ') {
            if (offs < 0) {
                log_error("'%s' variable name missing on line %i\n", fname, lnum);
                return -1;
            }
            if (((c = *p++) != '=') || ((c = *p++) != ' ')) {
                log_error("'%s' invalid = on line %i\n", fname, lnum);
                return -1;
            }
            p = savetype_de_text_parse_right(g, fname, p, lnum, offs, gi, 0);
            return (p != NULL) ? 0 : -1;
        } else {
            log_error("'%s' unexpected char 0x%02x on line %i\n", fname, cend, lnum);
            return -1;
        }
    }
    return 0;
}

static bool savetype_is_text(const struct game_s *g, const char *fname)
{
    FILE *fd = NULL;
    int len;
    bool is_text = true;
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname));
    fd = fopen(fname, "r");
    if (!fd) {
        log_error("failed to open file '%s'\n", fname);
        return false;
    }
    while ((len = util_get_line((char *)save2buf, 1024, fd)) >= 0) {
        if ((len == 0) || ((save2buf[0] == '/') && (save2buf[1] == '/'))) {
            continue;
        }
        if (memcmp(save2buf, "savename = \"", 12) == 0) {
            continue;
        } else if ((memcmp(save2buf, "g->", 3) != 0) || (len > 1020)) {
            is_text = false;
            break;
        }
    }
    if (fd) {
        fclose(fd);
    }
    return is_text;
}

static int savetype_de_text(struct game_s *g, const char *fname)
{
    FILE *fd = NULL;
    int len, lnum = 0;
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname));
    fd = fopen(fname, "r");
    if (!fd) {
        log_error("failed to open file '%s'\n", fname);
        return -1;
    }
    {
        void *t = g->gaux;
        memset(g, 0, sizeof(*g));
        g->gaux = t;
    }
    while ((len = util_get_line((char *)save2buf, 1024, fd)) >= 0) {
        ++lnum;
        if ((len == 0) || ((save2buf[0] == '/') && (save2buf[1] == '/'))) {
            continue;
        }
        if (memcmp(save2buf, "savename = \"", 12) == 0) {
            if (savename[0] == 0) {
                char *p = savename;
                const char *q = (const char *)&save2buf[12];
                for (int i = 0; (i < (SAVE_NAME_LEN)) && (*q != '"'); ++i) {
                    *p++ = *q++;
                }
                *p++ = 0;
            }
            continue;
        } else if ((memcmp(save2buf, "g->", 3) != 0) || (len > 1020)) {
            log_error("invalid line %i in '%s'\n", lnum, fname);
            goto fail;
        }
        if (savetype_de_text_parse_line(g, fname, (char *)&save2buf[3], lnum) < 0) {
            goto fail;
        }
    }
    if (fd) {
        fclose(fd);
    }
    return 0;
fail:
    if (fd) {
        fclose(fd);
    }
    return -1;
}

#define OUTADD  save2len += sprintf((char *)&save2buf[save2len],
#define OUTPRE()    OUTADD "%s", tp->buf)
#define OUTLINE OUTPRE(); OUTADD
#define OUTLINEI(_name_, _var_) OUTPRE(); OUTADD "%s = %i\n", _name_, _var_)
#define OUTLINEX(_name_, _var_) OUTPRE(); OUTADD "%s = 0x%x\n", _name_, _var_)
#define OUTLINES(_name_, _var_) OUTPRE(); OUTADD "%s = \"%s\"\n", _name_, _var_)
#define OUTLINEBV(_name_, _var_, _num_) OUTPRE(); OUTADD "%s = %s\n", _name_, savetype_en_bv(_var_, _num_))
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
    OUTLINES("name", sd->name);
    OUTLINEI("cost", sd->cost);
    OUTLINEI("space", sd->space);
    OUTLINEI("hull", sd->hull);
    OUTLINEI("look", sd->look);
    OUTLINETBL("wpnt", WEAPON_SLOT_NUM, sd->wpnt);
    OUTLINETBL("wpnn", WEAPON_SLOT_NUM, sd->wpnn);
    OUTLINEI("engine", sd->engine);
    OUTLINEI("engines", sd->engines);
    OUTLINETBL("special", SPECIAL_SLOT_NUM, sd->special);
    OUTLINEI("shield", sd->shield);
    OUTLINEI("jammer", sd->jammer);
    OUTLINEI("comp", sd->comp);
    OUTLINEI("armor", sd->armor);
    OUTLINEI("man", sd->man);
    OUTLINEI("hp", sd->hp);
}

static void savetype_en_text_monster(const monster_t *m, struct text_dump_prefix_s *tp)
{
    OUTLINEI("exists", m->exists);
    OUTLINEI("x", m->x);
    OUTLINEI("y", m->y);
    OUTLINEI("killer", m->killer);
    OUTLINEI("dest", m->dest);
    OUTLINEI("counter", m->counter);
    OUTLINEI("nuked", m->nuked);
}

static int savetype_en_text(const struct game_s *g, const char *fname)
{
    struct text_dump_prefix_s tp[1];
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname));
    save2len = 0;
    if (save_out_open(fname)) {
        return -1;
    }
    text_dump_prefix_init(tp);
    OUTLINES("savename", savename);
    text_dump_prefix_add(tp, "g", "->");
    OUTLINEI("players", g->players);
    OUTLINEBV("is_ai", g->is_ai, g->players);
    OUTLINEI("active_player", g->active_player);
    OUTLINEI("difficulty", g->difficulty);
    OUTLINEI("galaxy_size", g->galaxy_size);
    OUTLINEI("galaxy_w", g->galaxy_w);
    OUTLINEI("galaxy_h", g->galaxy_h);
    OUTLINEI("galaxy_stars", g->galaxy_stars);
    OUTLINEI("galaxy_maxx", g->galaxy_maxx);
    OUTLINEI("galaxy_maxy", g->galaxy_maxy);
    OUTLINEX("galaxy_seed", g->galaxy_seed);
    OUTLINEX("seed", g->seed);
    OUTLINEI("year", g->year);
    OUTLINEI("enroute_num", g->enroute_num);
    OUTLINEI("transport_num", g->transport_num);
    OUTLINEI("end", g->end);
    OUTLINEI("winner", g->winner);
    OUTLINEI("guardian_killer", g->guardian_killer);
    OUTLINEI("election_held", g->election_held);
    OUTFLUSH();
    OUTLINEI("nebula_num", g->nebula_num);
    if (g->nebula_num) {
        OUTLINETBL("nebula_type", g->nebula_num, g->nebula_type);
        OUTLINETBL("nebula_x", g->nebula_num, g->nebula_x);
        OUTLINETBL("nebula_y", g->nebula_num, g->nebula_y);
        for (int i = 0; i < g->nebula_num; ++i) {
            text_dump_prefix_add_tbl(tp, "nebula_x0", "", i);
            OUTLINETBL("", 4, g->nebula_x0[i]);
            text_dump_prefix_del(tp);
            text_dump_prefix_add_tbl(tp, "nebula_x1", "", i);
            OUTLINETBL("", 4, g->nebula_x1[i]);
            text_dump_prefix_del(tp);
            text_dump_prefix_add_tbl(tp, "nebula_y0", "", i);
            OUTLINETBL("", 4, g->nebula_y0[i]);
            text_dump_prefix_del(tp);
            text_dump_prefix_add_tbl(tp, "nebula_y1", "", i);
            OUTLINETBL("", 4, g->nebula_y1[i]);
            text_dump_prefix_del(tp);
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        OUTLINE "emperor_names[%i] = \"%s\"\n", i, g->emperor_names[i]);
    }
    OUTLINETBL("planet_focus_i", g->players, g->planet_focus_i);
    OUTFLUSH();
    for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        text_dump_prefix_add_tbl(tp, "planet", ".", i);
        OUTLINES("name", p->name);
        OUTLINEI("x", p->x);
        OUTLINEI("y", p->y);
        OUTLINEI("star_type", p->star_type);
        OUTLINEI("look", p->look);
        OUTLINEI("frame", p->frame);
        OUTLINEI("rocks", p->rocks);
        OUTLINEI("max_pop1", p->max_pop1);
        OUTLINEI("max_pop2", p->max_pop2);
        OUTLINEI("max_pop3", p->max_pop3);
        OUTLINEI("type", p->type);
        OUTLINEI("battlebg", p->battlebg);
        OUTLINEI("infogfx", p->infogfx);
        OUTLINEI("growth", p->growth);
        OUTLINEI("special", p->special);
        OUTLINEI("owner", p->owner);
        OUTLINEI("prev_owner", p->prev_owner);
        OUTLINEI("claim", p->claim);
        OUTLINEI("waste", p->waste);
        OUTLINEBV("explored", p->explored, g->players);
        OUTLINEI("bc_to_ecoproj", p->bc_to_ecoproj);
        OUTLINEI("bc_to_ship", p->bc_to_ship);
        OUTLINEI("bc_to_factory", p->bc_to_factory);
        OUTLINEI("reserve", p->reserve);
        OUTLINEI("pop", p->pop);
        OUTLINEI("pop_prev", p->pop_prev);
        OUTLINEI("factories", p->factories);
        OUTLINETBL("slider", PLANET_SLIDER_NUM, p->slider);
        OUTLINETBL("slider_lock", PLANET_SLIDER_NUM, p->slider_lock);
        OUTLINEI("buildship", p->buildship);
        OUTLINEI("reloc", p->reloc);
        OUTLINEI("missile_bases", p->missile_bases);
        OUTLINEI("bc_to_base", p->bc_to_base);
        OUTLINEI("bc_upgrade_base", p->bc_upgrade_base);
        OUTLINEI("have_stargate", p->have_stargate);
        OUTLINEI("shield", p->shield);
        OUTLINEI("bc_to_shield", p->bc_to_shield);
        OUTLINEI("trans_num", p->trans_num);
        OUTLINEI("trans_dest", p->trans_dest);
        OUTLINEI("pop_tenths", p->pop_tenths);
        OUTLINEI("pop_oper_fact", p->pop_oper_fact);
        OUTLINEI("bc_to_refit", p->bc_to_refit);
        OUTLINEI("rebels", p->rebels);
        OUTLINEI("unrest", p->unrest);
        OUTLINEI("unrest_reported", p->unrest_reported);
        OUTLINEBV("finished", p->finished, FINISHED_NUM);
        text_dump_prefix_del(tp);
        OUTFLUSH();
    }
    for (player_id_t j = PLAYER_0; j < g->players; ++j) {
        for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
            const seen_t *s = &(g->seen[j][i]);
            OUTLINE "seen[%i][%i] = { .owner = %i, .pop = %i, .bases = %i, .factories = %i }\n", j, i, s->owner, s->pop, s->bases, s->factories);
        }
        OUTFLUSH();
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        OUTLINE "enroute");
        OUTADD "[%i] = { .owner = %i, .x = %i, .y = %i, .dest = %i, .speed = %i, ", i, r->owner, r->x, r->y, r->dest, r->speed);
        OUTADD ".ships[] = { %i, %i, %i, %i, %i, %i } }\n", r->ships[0], r->ships[1], r->ships[2], r->ships[3], r->ships[4], r->ships[5]);
    }
    OUTFLUSH();
    for (int i = 0; i < g->transport_num; ++i) {
        const transport_t *r = &(g->transport[i]);
        OUTLINE "transport");
        OUTADD "[%i] = { .owner = %i, .x = %i, .y = %i, .dest = %i, .speed = %i, ", i, r->owner, r->x, r->y, r->dest, r->speed);
        OUTADD ".pop = %i }\n", r->pop);
    }
    OUTFLUSH();
    for (player_id_t pl = PLAYER_0; pl < g->players; ++pl) {
        const empiretechorbit_t *e = &(g->eto[pl]);
        text_dump_prefix_add_tbl(tp, "eto", ".", pl);
        OUTLINEI("race", e->race);
        OUTLINEI("banner", e->banner);
        OUTLINEI("trait1", e->trait1);
        OUTLINEI("trait2", e->trait2);
        OUTLINEI("ai_p3_countdown", e->ai_p3_countdown);
        OUTLINEI("ai_p2_countdown", e->ai_p2_countdown);
        OUTLINEBV("within_frange", e->within_frange, g->players);
        OUTLINETBL("relation1", g->players, e->relation1);
        OUTLINETBL("relation2", g->players, e->relation2);
        OUTLINETBL("diplo_type", g->players, e->diplo_type);
        OUTLINETBL("diplo_val", g->players, e->diplo_val);
        OUTLINETBL("diplo_p1", g->players, e->diplo_p1);
        OUTLINETBL("diplo_p2", g->players, e->diplo_p2);
        OUTLINETBL("trust", g->players, e->trust);
        OUTLINETBL("broken_treaty", g->players, e->broken_treaty);
        OUTLINETBL("blunder", g->players, e->blunder);
        OUTLINETBL("tribute_field", g->players, e->tribute_field);
        OUTLINETBL("tribute_tech", g->players, e->tribute_tech);
        OUTLINETBL("mood_treaty", g->players, e->mood_treaty);
        OUTLINETBL("mood_trade", g->players, e->mood_trade);
        OUTLINETBL("mood_tech", g->players, e->mood_tech);
        OUTLINETBL("mood_peace", g->players, e->mood_peace);
        OUTLINETBL("treaty", g->players, e->treaty);
        OUTLINETBL("trade_bc", g->players, e->trade_bc);
        OUTLINETBL("trade_percent", g->players, e->trade_percent);
        OUTLINETBL("spymode_next", g->players, e->spymode_next);
        OUTLINETBL("offer_field", g->players, e->offer_field);
        OUTLINETBL("offer_tech", g->players, e->offer_tech);
        OUTLINETBL("offer_bc", g->players, e->offer_bc);
        OUTLINETBL("hated", g->players, e->hated);
        OUTLINETBL("mutual_enemy", g->players, e->mutual_enemy);
        OUTLINETBL("hatred", g->players, e->hatred);
        OUTLINETBL("have_met", g->players, e->have_met);
        OUTLINETBL("trade_established_bc", g->players, e->trade_established_bc);
        OUTLINETBL("spying", g->players, e->spying);
        OUTLINETBL("spyfund", g->players, e->spyfund);
        OUTLINETBL("spymode", g->players, e->spymode);
        OUTLINEI("security", e->security);
        OUTLINETBL("spies", g->players, e->spies);
        OUTLINEI("reserve_bc", e->reserve_bc);
        OUTLINEI("tax", e->tax);
        OUTLINEI("base_shield", e->base_shield);
        OUTLINEI("base_comp", e->base_comp);
        OUTLINEI("base_weapon", e->base_weapon);
        OUTLINEI("colonist_oper_factories", e->colonist_oper_factories);
        OUTLINETBL("tech.percent", TECH_FIELD_NUM, e->tech.percent);
        OUTLINETBL("tech.slider", TECH_FIELD_NUM, e->tech.slider);
        OUTLINETBL("tech.slider_lock", TECH_FIELD_NUM, e->tech.slider_lock);
        OUTLINETBL("tech.investment", TECH_FIELD_NUM, e->tech.investment);
        OUTLINETBL("tech.project", TECH_FIELD_NUM, e->tech.project);
        OUTLINETBL("tech.cost", TECH_FIELD_NUM, e->tech.cost);
        OUTLINETBL("tech.completed", TECH_FIELD_NUM, e->tech.completed);
        OUTLINEI("shipdesigns_num", e->shipdesigns_num);
        OUTFLUSH();
        for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
            text_dump_prefix_add_tbl(tp, "orbit", ".", i);
            OUTLINETBLNON0("ships", e->shipdesigns_num, e->orbit[i].ships);
            text_dump_prefix_del(tp);
        }
        OUTFLUSH();
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            if (i != pl) {
                text_dump_prefix_add_tbl(tp, "spyreportfield", "", i);
                OUTLINETBL("", TECH_FIELD_NUM, e->spyreportfield[i]);
                text_dump_prefix_del(tp);
            }
        }
        OUTLINETBL("spyreportyear", g->players, e->spyreportyear);
        OUTLINEI("shipi_colony", e->shipi_colony);
        OUTLINEI("shipi_bomber", e->shipi_bomber);
        text_dump_prefix_del(tp);
        OUTFLUSH();
    }
    for (player_id_t pl = PLAYER_0; pl < g->players; ++pl) {
        const shipresearch_t *srd = &(g->srd[pl]);
        const empiretechorbit_t *e = &(g->eto[pl]);
        text_dump_prefix_add_tbl(tp, "srd", ".", pl);
        for (int i = 0; i < g->eto[pl].shipdesigns_num; ++i) {
            text_dump_prefix_add_tbl(tp, "design", ".", i);
            savetype_en_text_sd(&(srd->design[i]), tp);
            text_dump_prefix_del(tp);
        }
        OUTFLUSH();
        for (tech_field_t f = TECH_FIELD_COMPUTER; f < TECH_FIELD_NUM; ++f) {
            text_dump_prefix_add_tbl(tp, "researchlist", "", f);
            for (int t = 0; t < TECH_TIER_NUM; ++t) {
                text_dump_prefix_add_tbl(tp, "", "", t);
                OUTLINETBL("", 3, srd->researchlist[f][t]);
                text_dump_prefix_del(tp);
            }
            text_dump_prefix_del(tp);
            OUTFLUSH();
        }
        for (tech_field_t f = TECH_FIELD_COMPUTER; f < TECH_FIELD_NUM; ++f) {
            text_dump_prefix_add_tbl(tp, "researchcompleted", "", f);
            OUTLINETBL("", e->tech.completed[f], srd->researchcompleted[f]);
            text_dump_prefix_del(tp);
        }
        OUTFLUSH();
        OUTLINETBL("have_reserve_fuel", e->shipdesigns_num, srd->have_reserve_fuel);
        OUTLINETBL("year", e->shipdesigns_num, srd->year);
        text_dump_prefix_del(tp);
    }
    OUTFLUSH();
    for (player_id_t pl = PLAYER_0; pl < g->players; ++pl) {
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
        OUTLINEI("year", ev->year);
        OUTLINEBV("done", ev->done, GAME_EVENT_TBL_NUM);
        OUTLINEI("have_plague", ev->have_plague);
        OUTLINEI("plague_player", ev->plague_player);
        OUTLINEI("plague_planet_i", ev->plague_planet_i);
        OUTLINEI("plague_val", ev->plague_val);
        OUTLINEI("have_nova", ev->have_nova);
        OUTLINEI("nova_player", ev->nova_player);
        OUTLINEI("nova_planet_i", ev->nova_planet_i);
        OUTLINEI("nova_years", ev->nova_years);
        OUTLINEI("nova_val", ev->nova_val);
        OUTLINEI("have_accident", ev->have_accident);
        OUTLINEI("accident_planet_i", ev->accident_planet_i);
        OUTLINEI("have_comet", ev->have_comet);
        OUTLINEI("comet_player", ev->comet_player);
        OUTLINEI("comet_planet_i", ev->comet_planet_i);
        OUTLINEI("comet_years", ev->comet_years);
        OUTLINEI("comet_hp", ev->comet_hp);
        OUTLINEI("comet_dmg", ev->comet_dmg);
        OUTLINEI("have_pirates", ev->have_pirates);
        OUTLINEI("pirates_planet_i", ev->pirates_planet_i);
        OUTLINEI("pirates_hp", ev->pirates_hp);
        text_dump_prefix_add(tp, "crystal", ".");
        savetype_en_text_monster(&(ev->crystal), tp);
        text_dump_prefix_del(tp);
        text_dump_prefix_add(tp, "amoeba", ".");
        savetype_en_text_monster(&(ev->amoeba), tp);
        text_dump_prefix_del(tp);
        OUTLINEI("planet_orion_i", ev->planet_orion_i);
        OUTLINEI("have_guardian", ev->have_guardian);
        OUTLINETBL("home", g->players, ev->home);
        OUTLINEI("report_stars", ev->report_stars);
        OUTFLUSH();
        for (player_id_t pl = PLAYER_0; pl < g->players; ++pl) {
            text_dump_prefix_add_tbl(tp, "new_ships", "", pl);
            OUTLINETBL("", NUM_SHIPDESIGNS, ev->new_ships[pl]);
            text_dump_prefix_del(tp);
        }
        for (player_id_t pl = PLAYER_0; pl < g->players; ++pl) {
            text_dump_prefix_add_tbl(tp, "spies_caught", "", pl);
            OUTLINETBL("", g->players, ev->spies_caught[pl]);
            text_dump_prefix_del(tp);
        }
        for (player_id_t pl = PLAYER_0; pl < g->players; ++pl) {
            text_dump_prefix_add_tbl(tp, "ceasefire", "", pl);
            OUTLINETBL("", g->players, ev->ceasefire[pl]);
            text_dump_prefix_del(tp);
        }
        for (player_id_t pl = PLAYER_0; pl < g->players; ++pl) {
            if (IS_HUMAN(g, pl)) {
                text_dump_prefix_add_tbl(tp, "help_shown", "", pl);
                OUTLINEBV("", ev->help_shown[pl], HELP_SHOWN_NUM);
                text_dump_prefix_del(tp);
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
    FILE *fd = NULL;
    char *sname = (*savename == '\0') ? savename : NULL;
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname));
    if (sname) {
        fd = libsave_1oom_open_check_header(fname, sname);
        if (fd) {
            fclose(fd);
            fd = NULL;
        }
    }
    return libsave_1oom_load_do(fname, g);
}

static int savetype_en_1oom0(const struct game_s *g, const char *fname)
{
    LOG_DEBUG((2, "%s: '%s'\n", __func__, fname ? fname : "(null)"));
    return libsave_1oom_save_do(fname, savename, g);
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
                 "  s   - smart: autodetect (default)\n"
                 "  m   - MOO1 v1.3\n"
                 "  1   - 1oom save version 0\n"
                 "  t   - text"
    },
    { "-o", 1,
      saveconv_opt_typeo, 0,
      "OUTTYPE", "Output type:\n"
                 "  s   - smart: in old/new -> out new/old (default)\n"
                 "  m   - MOO1 v1.3\n"
                 "  1   - 1oom save version 0\n"
                 "  t   - text\n"
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
    if (os_early_init()) {
        return 1;
    }
    gameptr = lib_malloc(sizeof(struct game_s));
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
    lib_free(gameptr);
    lib_free(save2buf);
    os_shutdown();
}

int main_do(void)
{
    int res;
    const char *fname;
    if (main_fname_num == 0) {
        options_show_usage();
        return 0;
    }
    fname = fnames[0];
    res = savetype[savetypei].decode(gameptr, fname);
    if (res < 0) {
        log_error("decoding file '%s' failed\n", fname);
        return 1;
    }
    log_message("saveconv: decode type '%s' file '%s'\n", savetype[savetypei].name, fname);
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
    }
    log_message("saveconv: encode type '%s' file '%s'\n", savetype[savetypeo].name, fname ? fname : "(null)");
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
