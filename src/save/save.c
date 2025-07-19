#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "save.h"
#include "save_1oom.h"
#include "bits.h"
#include "game.h"
#include "game_aux.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "types.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

bool game_save_tbl_have_save[NUM_ALL_SAVES];
char game_save_tbl_name[NUM_ALL_SAVES][SAVE_NAME_LEN];

static int savenamebuflen = 0;
static int savebuflen = 0;
static char *savenamebuf = NULL;
static uint8_t *savebuf = NULL;

void libsave_init(void)
{
    savenamebuflen = FSDEV_PATH_MAX;
    savenamebuf = lib_malloc(savenamebuflen);
    savebuflen = sizeof(struct game_s) + 64;
    savebuf = lib_malloc(savebuflen);
}

void libsave_shutdown(void)
{
    lib_free(savenamebuf);
    savenamebuf = NULL;
    savenamebuflen = 0;
    lib_free(savebuf);
    savebuf = NULL;
    savebuflen = 0;
}

/* -------------------------------------------------------------------------- */

static int game_save_do_save_do(const char *filename, const char *savename, const struct game_s *g, int savei)
{
    FILE *fd;
    uint8_t hdr[SAVE_1OOM_HDR_SIZE];
    int res = -1, len;
    if ((len = libsave_1oom_encode(savebuf, savebuflen, g)) <= 0) {
        return -1;
    }
    if (os_make_path_for(filename)) {
        log_error("Save: failed to create path for '%s'\n", filename);
    }
    libsave_1oom_make_header(hdr, savename);
    fd = game_save_open_check_header(filename, -1, false, 0);
    if (fd) {
        /* file exists */
        fclose(fd);
        fd = NULL;
    }
    if ((savei >= 0) && (savei < (NUM_SAVES + 1))) {
        game_save_tbl_have_save[savei] = false;
        game_save_tbl_name[savei][0] = '\0';
    }
    fd = fopen(filename, "wb+");
    if (0
      || (!fd)
      || (fwrite(hdr, SAVE_1OOM_HDR_SIZE, 1, fd) != 1)
      || (fwrite(savebuf, len, 1, fd) != 1)
    ) {
        log_error("Save: failed to save '%s'\n", filename);
        unlink(filename);
        goto done;
    }
    if ((savei >= 0) && (savei < (NUM_SAVES + 1))) {
        game_save_tbl_have_save[savei] = true;
        memcpy(game_save_tbl_name[savei], &hdr[SAVE_1OOM_OFFS_NAME], SAVE_NAME_LEN);
    }
    log_message("Save: save '%s' '%s'\n", filename, savename);
    res = 0;
done:
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    return res;
}

static int game_save_do_load_do(const char *filename, struct game_s *g, int savei, char *savename)
{
    FILE *fd = NULL;
    int res = -1, len = 0;

    fd = game_save_open_check_header(filename, savei, true, savename);
    if ((!fd) || ((len = fread(savebuf, 1, savebuflen, fd)) == 0) || (!feof(fd))) {
        log_error("Save: failed to load '%s'\n", filename);
    } else if (libsave_1oom_decode(savebuf, len, g) != 0) {
        log_error("Save: invalid data on load '%s'\n", filename);
    } else {
        log_message("Save: load '%s'\n", filename);
        res = 0;
    }
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    return res;
}

/* -------------------------------------------------------------------------- */

void *game_save_open_check_header(const char *filename, int i, bool update_table, char *savename)
{
    uint8_t hdr[SAVE_1OOM_HDR_SIZE];
    FILE *fd;
    if ((i < 0) || (i >= NUM_ALL_SAVES)) {
        update_table = false;
    }
    if (update_table) {
        game_save_tbl_have_save[i] = false;
        game_save_tbl_name[i][0] = '\0';
    }
    if (savename) {
        savename[0] = '\0';
    }
    fd = fopen(filename, "rb");
    if (fd) {
        if (1
          && (fread(hdr, SAVE_1OOM_HDR_SIZE, 1, fd) == 1)
          && (memcmp(hdr, (const uint8_t *)SAVE_1OOM_MAGIC, 8) == 0)
          && (GET_LE_32(&hdr[SAVE_1OOM_OFFS_VERSION]) == SAVE_1OOM_VERSION)
        ) {
            if (update_table) {
                game_save_tbl_have_save[i] = true;
                memcpy(game_save_tbl_name[i], &hdr[SAVE_1OOM_OFFS_NAME], SAVE_NAME_LEN);
                game_save_tbl_name[i][SAVE_NAME_LEN - 1] = '\0';
            }
            if (savename) {
                memcpy(savename, &hdr[SAVE_1OOM_OFFS_NAME], SAVE_NAME_LEN);
                savename[SAVE_NAME_LEN - 1] = '\0';
            }
        } else {
            fclose(fd);
            fd = NULL;
        }
    }
    return fd;
}

const char *libsave_select_slot_fname(int i)
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

int libsave_check_saves(void)
{
    FILE *fd;

    for (int i = 0; i < NUM_ALL_SAVES; ++i) {
        const char *fname = libsave_select_slot_fname(i);
        fd = game_save_open_check_header(fname, i, true, 0);
        if (fd) {
            fclose(fd);
        }
    }
    return 0;
}

int game_save_do_load_fname(const char *filename, char *savename, struct game_s *g)
{
    return game_save_do_load_do(filename, g, -1, savename);
}

int game_save_do_save_fname(const char *filename, const char *savename, const struct game_s *g)
{
    return game_save_do_save_do(filename, savename, g, -1);
}

int game_save_do_load_i(int savei, struct game_s *g)
{
    int res;
    const char *filename = libsave_select_slot_fname(savei);
    res = game_save_do_load_do(filename, g, savei, 0);
    return res;
}

int game_save_do_save_i(int savei, const char *savename, const struct game_s *g)
{
    int res;
    const char *filename;
    if (os_make_path_user()) {
        log_error("Save: failed to create user path '%s'\n", os_get_path_user());
    }
    filename = libsave_select_slot_fname(savei);
    res = game_save_do_save_do(filename, savename, g, savei);
    return res;
}
