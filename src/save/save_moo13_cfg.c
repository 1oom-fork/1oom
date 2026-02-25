#include "config.h"

#include <string.h>

#include "lib.h"
#include "log.h"
#include "os.h"
#include "save.h"
#include "save_moo13_cfg.h"
#include "util.h"

int libsave_cmoo_get_fname(char *buf, int buflen)
{
    const char *path = os_get_path_user();
    int res;
    res = util_concat_buf(buf, buflen, path, FSDEV_DIR_SEP_STR, "CONFIG.MOO", NULL);
    if (res < 0) {
        log_error("Save: BUG: config name buffer too small by %i bytes\n", -res);
        return -1;
    }
    return 0;
}

int libsave_cmoo_load_do(const char *filename)
{
    uint8_t *buf = NULL;
    int res = 0;
    int len;
    buf = lib_malloc(SAVE_CMOO_LEN);
    if ((len = util_file_try_load_len(filename, buf, SAVE_CMOO_LEN)) <= 0) {
        log_error("loading MOO1 v1.3 '%s' (got %i != %i bytes)\n", filename, len, SAVE_MOO13_LEN);
        res = -1;
    }
    if (res == 0) {
        for (int i = 0; i < NUM_SAVES; ++i) {
            game_save_tbl_have_save[i] = buf[save_cmoo_havesave_offs(i)];
            strncpy(game_save_tbl_name[i], (char *)&buf[save_cmoo_savename_offs(i)], SAVE_NAME_LEN);
            game_save_tbl_name[i][SAVE_NAME_LEN - 1] = '\0';
        }
    }
    lib_free(buf);
    buf = NULL;
    return res;
}

int libsave_cmoo_save_do(const char *filename, const char *savename, int savei)
{
    uint8_t *buf = NULL;
    int res = 0;
    int len;
    buf = lib_malloc(SAVE_CMOO_LEN);
    if ((len = util_file_try_load_len(filename, buf, SAVE_CMOO_LEN)) <= 0) {
        log_error("loading MOO1 v1.3 '%s' (got %i != %i bytes)\n", filename, len, SAVE_MOO13_LEN);
        res = -1;
    }
    if (res == 0) {
        strncpy((char *)&buf[save_cmoo_savename_offs(savei)], savename, SAVE_NAME_LEN);
        buf[save_cmoo_savename_offs(savei) + SAVE_NAME_LEN - 1] = '\0';
        buf[save_cmoo_havesave_offs(savei)] = 1;
        if (util_file_save(filename, buf, SAVE_CMOO_LEN)) {
            log_warning("failed to save '%s'\n", filename);
        }
    }
    lib_free(buf);
    buf = NULL;
    return res;
}
