#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "game_save.h"
#include "game.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "save.h"
#include "types.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

bool game_save_tbl_have_save[NUM_ALL_SAVES];
char game_save_tbl_name[NUM_ALL_SAVES][SAVE_NAME_LEN];

bool game_opt_use_moo13 = true;

/* -------------------------------------------------------------------------- */

static int game_save_get_slot_fname(char *buf, int buflen, int i)
{
    const char *path = os_get_path_user();
    char namebuf[16];
    int res;
    if (game_opt_use_moo13) {
        sprintf(namebuf, "SAVE%i.GAM", i + 1);
    } else {
        if (!os_get_fname_save(namebuf, i + 1)) {
            sprintf(namebuf, "1oom_save%i.bin", i + 1);
        }
    }
    res = util_concat_buf(buf, buflen, path, FSDEV_DIR_SEP_STR, namebuf, NULL);
    if (res < 0) {
        log_error("Save: BUG: save name buffer too small by %i bytes\n", -res);
        return -1;
    }
    return 0;
}

static void game_save_check_save(const char *fname, int savei)
{
    FILE *fd = NULL;
    char savename[SAVE_NAME_LEN];
    bool update_table = false;
    if ((savei >= 0) && (savei < NUM_ALL_SAVES)) {
        update_table = true;
    }
    if (update_table) {
        game_save_tbl_have_save[savei] = false;
        game_save_tbl_name[savei][0] = '\0';
    }
    if (game_opt_use_moo13) {
        update_table = false; /* FIXME */
        if (libsave_is_moo13(fname)) {
            game_save_tbl_have_save[savei] = true;
        }
    } else {
        fd = libsave_1oom_open_check_header(fname, savename);
        if (!fd) {
            update_table = false;
        }
    }
    if (update_table) {
        game_save_tbl_have_save[savei] = true;
        memcpy(game_save_tbl_name[savei], &(savename[0]), SAVE_NAME_LEN);
        game_save_tbl_name[savei][SAVE_NAME_LEN - 1] = '\0';
    }
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
}

int game_save_check_saves(void)
{
    char *fnamebuf = NULL;

    fnamebuf = lib_malloc(FSDEV_PATH_MAX);
    for (int i = 0; i < NUM_ALL_SAVES; ++i) {
        game_save_get_slot_fname(fnamebuf, FSDEV_PATH_MAX, i);
        game_save_check_save(fnamebuf, i);
    }
    lib_free(fnamebuf);
    fnamebuf = NULL;
    return 0;
}

int game_save_do_load_fname(const char *filename, struct game_s *g)
{
    if (libsave_is_moo13(filename) && !libsave_moo13_load_do(filename, g)) {
        return 0;
    }
    return libsave_1oom_load_do(filename, g);
}

int game_save_do_load_i(int savei, struct game_s *g)
{
    int res;
    char *filename = lib_malloc(FSDEV_PATH_MAX);
    game_save_get_slot_fname(filename, FSDEV_PATH_MAX, savei);
    game_save_check_save(filename, savei);
    if (game_opt_use_moo13) {
        res = libsave_moo13_load_do(filename, g);
    } else {
        res = libsave_1oom_load_do(filename, g);
    }
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
    if (game_opt_use_moo13) {
        res = libsave_moo13_save_do(filename, g);
    } else {
        res = libsave_1oom_save_do(filename, savename, g);
    }
    game_save_check_save(filename, savei);
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
