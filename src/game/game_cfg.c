#include "config.h"

#include "game_cfg.h"
#include "game_save.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

uint8_t *cmoo_buf = NULL;

/* -------------------------------------------------------------------------- */

static int game_cfg_get_fname(char *buf, int buflen)
{
    const char *path = os_get_path_user();
    int res;
    res = util_concat_buf(buf, buflen, path, FSDEV_DIR_SEP_STR, "CONFIG.MOO", NULL);
    if (res < 0) {
        log_error("Game: BUG: cfg name buffer too small by %i bytes\n", -res);
        return -1;
    }
    return 0;
}

int game_cfg_init(void)
{
    cmoo_buf = lib_malloc(CMOO_LEN);
    if (game_cfg_load() != 0) {
        lib_free(cmoo_buf);
        cmoo_buf = NULL;
        return -1;
    }
    return 0;
}

int game_cfg_load(void)
{
    int res = -1;
    char *filename;
    if (!cmoo_buf) {
        return -1;
    }
    filename = lib_malloc(FSDEV_PATH_MAX);
    game_cfg_get_fname(filename, FSDEV_PATH_MAX);
    if (util_file_try_load_len(filename, cmoo_buf, CMOO_LEN) > 0) {
        switch (cmoo_buf[CMOO_OFFS_SOUND_MODE]) {
            case 0:
                opt_music_enabled = false;
                opt_sfx_enabled = false;
                break;
            case 1:
                opt_music_enabled = false;
                opt_sfx_enabled = true;
                break;
            case 2:
            default:
                opt_music_enabled = true;
                opt_sfx_enabled = true;
                break;
        }
        res = 0;
    } else {
        log_error("failed to load '%s'\n", filename);
    }
    lib_free(filename);
    filename = NULL;
    return res;
}

int game_cfg_write(void)
{
    int res = 0;
    char *filename;
    if (!cmoo_buf) {
        return -1;
    }
    filename = lib_malloc(FSDEV_PATH_MAX);
    game_cfg_get_fname(filename, FSDEV_PATH_MAX);
    if (util_file_save(filename, cmoo_buf, CMOO_LEN)) {
        log_error("Game: failed to save '%s'\n", filename);
        res = -1;
    }
    lib_free(filename);
    filename = NULL;
    return res;
}

void game_cfg_shutdown(void)
{
    if (cmoo_buf) {
        lib_free(cmoo_buf);
        cmoo_buf = NULL;
    }
}
