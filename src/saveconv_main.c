#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "game/game.h"
#include "game/game_aux.h"
#include "game/game_save.h"
#include "lib.h"
#include "log.h"
#include "main.h"
#include "os.h"
#include "saveconv.h"
#include "saveconv_main.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

const char *idstr_main = "saveconv";

bool main_use_lbx = false;
bool ui_use_audio = false;

/* -------------------------------------------------------------------------- */

bool main_use_cfg = false;

char *cfg_cfgname(void)
{
    return 0;
}

int cfg_load(const char *filename)
{
    return -1;  /* should never be called */
}

int cfg_save(const char *filename)
{
    return -1;  /* should never be called */
}

/* -------------------------------------------------------------------------- */

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

struct savetype_s {
    const char *name;
    bool (*detect)(struct game_s *g, const char *fname);
    int (*decode)(struct game_s *g, const char *fname); /* to native */
    int (*encode)(struct game_s *g, const char *fname);
    uint8_t flags;
    savetype_t othertype;
};

const struct savetype_s savetype[SAVETYPE_NUM] = {
    { /* SAVETYPE_SMART */
        "smart",
        0,
        savetype_de_smart,
        0,
        0, SAVETYPE_NUM
    },
    { /* SAVETYPE_MOO13 */
        "MOO v1.3",
        saveconv_is_moo13,
        saveconv_de_moo13,
        saveconv_en_moo13,
        0, SAVETYPE_NATIVE
    },
    { /* SAVETYPE_1OOM0 */
        "1oom save version 0",
        0,
        saveconv_de_1oom0,
        saveconv_en_1oom0,
        0, SAVETYPE_MOO13
    },
    { /* SAVETYPE_TEXT  */
        "text",
        saveconv_is_text,
        saveconv_de_text,
        saveconv_en_text,
        SAVETYPE_F_OPTOUT, SAVETYPE_NATIVE
    },
    { /* SAVETYPE_DUMMY */
        "dummy",
        0,
        0,
        0,
        0, SAVETYPE_NUM
    }
};

savetype_t savetypei = SAVETYPE_SMART;
savetype_t savetypeo = SAVETYPE_SMART;

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

static int main_fname_num = 0;
static const char *fnames[2] = { 0/*in*/, 0/*out*/ };

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
    if ((fd = game_save_open_check_header(fname, -1, false, 0, 0)) != 0) {
        fclose(fd);
        fd = NULL;
        savetypei = SAVETYPE_NATIVE;
        res = savetype[SAVETYPE_NATIVE].decode(g, fname);
    } else if (saveconv_is_moo13(g, fname)) {
        savetypei = SAVETYPE_MOO13;
        res = saveconv_de_moo13(g, fname);
    } else if (saveconv_is_text(g, fname)) {
        savetypei = SAVETYPE_TEXT;
        res = saveconv_de_text(g, fname);
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

struct pbx_add_cbs;

int pbx_add_file(const char *filename, struct pbx_add_cbs *cbs)
{
    return -1;
}

/* -------------------------------------------------------------------------- */

static struct game_s game;
static struct game_aux_s gaux;

static int saveconv_main_init(void)
{
    if (os_early_init()) {
        return 1;
    }
    memset(&game, 0, sizeof(struct game_s));
    memset(&gaux, 0, sizeof(struct game_aux_s));
    game.gaux = &gaux;
    gaux.savenamebuflen = FSDEV_PATH_MAX;
    gaux.savenamebuf = lib_malloc(gaux.savenamebuflen);
    gaux.savebuflen = sizeof(struct game_s) + 64;
    gaux.savebuf = lib_malloc(gaux.savebuflen);
    saveconv_init();
    return 0;
}

static void saveconv_main_shutdown(void)
{
    lib_free(gaux.savenamebuf);
    lib_free(gaux.savebuf);
    saveconv_shutdown();
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
    if (util_parse_number(fname, &v)) {
        if ((v >= 1) && (v <= NUM_ALL_SAVES)) {
            game_save_get_slot_fname(gaux.savenamebuf, gaux.savenamebuflen, v - 1);
            fname = gaux.savenamebuf;
        } else if ((v >= 2300) && (v <= 9999)) {
            game_save_get_year_fname(gaux.savenamebuf, gaux.savenamebuflen, v);
            fname = gaux.savenamebuf;
        }
    }
    res = savetype[savetypei].decode(&game, fname);
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
    } else if (util_parse_number(fname, &v)) {
        if ((v >= 1) && (v <= NUM_ALL_SAVES)) {
            game_save_get_slot_fname(gaux.savenamebuf, gaux.savenamebuflen, v - 1);
            fname = gaux.savenamebuf;
        } else if ((v >= 2300) && (v <= 9999)) {
            game_save_get_year_fname(gaux.savenamebuf, gaux.savenamebuflen, v);
            fname = gaux.savenamebuf;
        }
    }
    log_message("saveconv: encode type '%s' file '%s'\n", savetype[savetypeo].name, fname ? fname : "(null)");
    if (savename[0] == '\0') {
        strcpy(savename, "saveconv");
    }
    res = savetype[savetypeo].encode(&game, fname);
    if (res < 0) {
        log_error("encoding file '%s' failed\n", fname ? fname : "(null)");
        return 1;
    }
    return 0;
}

int main_1oom(int argc, char **argv)
{
    if (saveconv_main_init()) {
        return 1;
    }
    if (options_parse_early(argc, argv)) {
        return 1;
    }
    atexit(saveconv_main_shutdown);
    if (os_init()) {
        return 2;
    }
    if (options_parse(argc, argv)) {
        return 3;
    }
    return main_do();
}
