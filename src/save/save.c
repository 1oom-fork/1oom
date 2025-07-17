#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "save.h"
#include "save_1oom.h"
#include "game.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "types.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

bool game_save_tbl_have_save[NUM_ALL_SAVES];
char game_save_tbl_name[NUM_ALL_SAVES][SAVE_NAME_LEN];

static int savenamebuflen = 0;
static char *savenamebuf = NULL;

void libsave_init(void)
{
    savenamebuflen = FSDEV_PATH_MAX;
    savenamebuf = lib_malloc(savenamebuflen);
}

void libsave_shutdown(void)
{
    lib_free(savenamebuf);
    savenamebuf = NULL;
    savenamebuflen = 0;
}

/* -------------------------------------------------------------------------- */

const char *game_save_get_slot_fname(int i)
{
    const char *path = os_get_path_user();
    char namebuf[16];
    int res;
    if (!os_get_fname_save(namebuf, i + 1)) {
        sprintf(namebuf, "1oom_save%i.bin", i + 1);
    }
    res = util_concat_buf(savenamebuf, savenamebuflen, path, FSDEV_DIR_SEP_STR, namebuf, NULL);
    if (res < 0) {
        log_error("Save: BUG: save name buffer too small by %i bytes\n", -res);
        return NULL;
    }
    return savenamebuf;
}

int game_save_check_saves(void)
{
    for (int i = 0; i < NUM_ALL_SAVES; ++i) {
        const char *fname = game_save_get_slot_fname(i);
        game_save_check_header(fname, i);
    }
    return 0;
}

bool game_save_is_1oom(const char *filename)
{
    return game_save_check_header(filename, -1);
}

int game_save_do_load_fname(const char *filename, char *savename, struct game_s *g)
{
    return game_save_do_load_do(filename, g, -1);
}

int game_save_do_save_fname(const char *filename, const char *savename, const struct game_s *g)
{
    return game_save_do_save_do(filename, savename, g, -1);
}

int game_save_do_load_i(int savei, struct game_s *g)
{
    int res;
    const char *filename = game_save_get_slot_fname(savei);
    res = game_save_do_load_do(filename, g, savei);
    return res;
}

int game_save_do_save_i(int savei, const char *savename, const struct game_s *g)
{
    int res;
    const char *filename;
    if (os_make_path_user()) {
        log_error("Save: failed to create user path '%s'\n", os_get_path_user());
    }
    filename = game_save_get_slot_fname(savei);
    res = game_save_do_save_do(filename, savename, g, savei);
    return res;
}
