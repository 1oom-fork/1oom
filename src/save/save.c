#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "save.h"
#include "game.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "types.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

bool game_save_tbl_have_save[NUM_ALL_SAVES];
char game_save_tbl_name[NUM_ALL_SAVES][SAVE_NAME_LEN];

/* -------------------------------------------------------------------------- */

int game_save_get_slot_fname(char *buf, int buflen, int i)
{
    const char *path = os_get_path_user();
    char namebuf[16];
    int res;
    if (!os_get_fname_save(namebuf, i + 1)) {
        sprintf(namebuf, "1oom_save%i.bin", i + 1);
    }
    res = util_concat_buf(buf, buflen, path, FSDEV_DIR_SEP_STR, namebuf, NULL);
    if (res < 0) {
        log_error("Save: BUG: save name buffer too small by %i bytes\n", -res);
        return -1;
    }
    return 0;
}

int game_save_check_saves(void)
{
    FILE *fd;
    char *fnamebuf = NULL;

    fnamebuf = lib_malloc(FSDEV_PATH_MAX);
    for (int i = 0; i < NUM_ALL_SAVES; ++i) {
        game_save_get_slot_fname(fnamebuf, FSDEV_PATH_MAX, i);
        fd = libsave_1oom_open_check_header(fnamebuf, i, true, 0);
        if (fd) {
            fclose(fd);
        }
    }
    lib_free(fnamebuf);
    fnamebuf = NULL;
    return 0;
}

int game_save_do_load_fname(const char *filename, struct game_s *g)
{
    return libsave_1oom_load_do(filename, g, -1, NULL);
}

int game_save_do_load_i(int savei, struct game_s *g)
{
    int res;
    char *filename = lib_malloc(FSDEV_PATH_MAX);
    game_save_get_slot_fname(filename, FSDEV_PATH_MAX, savei);
    res = libsave_1oom_load_do(filename, g, savei, 0);
    lib_free(filename);
    filename = NULL;
    return res;
}

int game_save_do_save_i(int savei, const char *savename, const struct game_s *g)
{
    int res;
    char *filename = lib_malloc(FSDEV_PATH_MAX);
    if (os_make_path_user()) {
        log_error("Save: failed to create user path '%s'\n", os_get_path_user());
    }
    game_save_get_slot_fname(filename, FSDEV_PATH_MAX, savei);
    res = libsave_1oom_save_do(filename, savename, g, savei);
    lib_free(filename);
    filename = NULL;
    return res;
}

void game_save_do_delete_i(int savei, const struct game_s *g)
{
    char *filename = lib_malloc(FSDEV_PATH_MAX);
    game_save_get_slot_fname(filename, FSDEV_PATH_MAX, savei);
    unlink(filename);
    lib_free(filename);
    filename = NULL;
}
