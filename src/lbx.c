#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lbx.h"
#include "bits.h"
#include "lbxgfx.h"
#include "lib.h"
#include "log.h"
#include "md5.h"
#include "os.h"
#include "types.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

#define LBX_HEADER_LEN  0x200

struct lbx_s {
    enum {
        LBX_MODE_NONE = 0,
        LBX_MODE_FILE_OPEN,
        LBX_MODE_MEMORY
    } mode;
    FILE *fd;
    uint32_t len;
    uint16_t entries;
    uint16_t type;
    uint8_t *header;
    uint8_t *data;
    int num_in_use;
};

static const struct {
    const char *filename;
    int64_t size;
    bool in_memory;     /* load whole file to memory */
    const char *md5;
} lbxinfo[LBXFILE_NUM] = {
    { "backgrnd.lbx", 326083, true, "a46e6ad9e8cc23893da78467d8a29e28" },
    { "colonies.lbx", 175728, true, "fd8612f703c57d680385abdca17105d1" },
    { "council.lbx", 278398, true, "ad4603e8a311e347a65e4d361573046a" },
    { "design.lbx", 48830, true, "64984d5a57c7cf0f977233c06e5047ee" },
    { "diplomat.lbx", 243746, true, "e5fc3f6f1c54765df64635922d3c136a" },
    { "embassy.lbx", 440746, true, "95d07952c2e00f5b54e195e213fd10ab" },
    { "eventmsg.lbx", 31348, true, "1acbba8b9b9fdd613332007e8243d341" },
    { "firing.lbx", 4664, true, "aa75c8712b36826b12456fe677c08b54" },
    { "fonts.lbx", 73636, true, "90185afbfee99bd0880975607e540906" },
    { "help.lbx", 61668, true, "7b6bdb4d3473bf4c92728c322cf6d990" },
    { "intro.lbx", 1859531, false, "bb67559b324a39c08b8499f6b644c72c" },
    { "intro2.lbx", 1162795, false, "9d2939f54fd0afd598540ee6cff38702" },
    { "introsnd.lbx", 115133, false, "231249611c30549de45832c3ea3b7950" },
    { "landing.lbx", 1273514, true, "da2821237a1a6fc5e1c815165b4798ab" },
    { "missile.lbx", 12894, true, "4783bf2a2a80d2271a8b765860c6d42d" },
    { "music.lbx", 104646, true, "895a69c82771681d7f03a464387503f5" },
    { "names.lbx", 1748, true, "c5a389047b06046edeb3b88a52cf0ccf" },
    { "nebula.lbx", 17668, true, "cf760ca4317576017b2e639f90723966" },
    { "newscast.lbx", 136230, true, "24c92d46fa767d44ce547e7b04bfc1c2" },
    { "planets.lbx", 37532, true, "468dece5e73d06e1b6b548f86fcde51d" },
    { "research.lbx", 64820, true, "690ab296de5d0bce8436fa5174bb7048" },
    { "screens.lbx", 15923, true, "0756b968b1f10179571360aac2f6b302" },
    { "ships.lbx", 48014, true, "7af514d3126245a4dc8a15712eed7fa5" },
    { "ships2.lbx", 48483, true, "6eba05956be3ed6f5139bd6638920a26" },
    { "snddrv.lbx", 183011, true, "f3533498a2a5052ec6431f74a987a649" },
    { "soundfx.lbx", 355385, true, "7bb71fb62fba0fef557d37ee054bbcc9" },
    { "space.lbx", 77134, true, "db4deda97ad23b8107762cb169f7eae0" },
    { "spies.lbx", 834214, true, "d2e2fa83b239ca1218f205bbe965ebea" },
    { "starmap.lbx", 220383, true, "59200e5ea204447bf680e681ad405b9a" },
    { "starview.lbx", 870055, true, "6f41a8fd1f736a2eba6a1ee4c1536378" },
    { "techno.lbx", 266857, true, "ee9ab3ed71a6d70d614fa3a8cb857da6" },
    { "v11.lbx", 8887, true, "d2bb2f627bde3ca9b7260f9961d98c6e" },
    { "vortex.lbx", 490093, true, "581424657fc5427ad654405c6a1d03e7" },
    { "winlose.lbx", 1045576, false, "c0399f0c67473874d944aebf1a5dfc88" }
};

static struct lbx_s lbxtbl[LBXFILE_NUM] = { 0 };

static char entrynamebuf[32 + 1]; /* HACK */

/* -------------------------------------------------------------------------- */

static bool lbxfile_check_md5(const char *path)
{
    bool result = true;
    uint8_t *buf;
    const struct lbxinfo *info;
    MD5_CTX ctx;
    log_message("Checking md5: ");
    buf = lib_malloc(lbxinfo[LBXFILE_INTRO].size);
    for (int i = 0; i < LBXFILE_NUM; ++i) {
        const char *fname;
        fname = lbxinfo[i].filename;
        if (path) {
            fname = util_concat(path, FSDEV_DIR_SEP_STR, lbxinfo[i].filename, NULL);
        }
        if (util_file_try_load_len(fname, buf, lbxinfo[i].size) == 0) {
            if (path) {
                lib_free(fname);
                fname = NULL;
            }
            char lbx_fname_ucase[16];
            const char *p = lbxinfo[i].filename;
            for (int i = 0; i < 16; ++i) {
                lbx_fname_ucase[i] = toupper(*p);
                if (!*p) {
                    break;
                }
                ++p;
            }
            if (path) {
                fname = util_concat(path, FSDEV_DIR_SEP_STR, lbx_fname_ucase, NULL);
                if (util_file_try_load_len(fname, buf, lbxinfo[i].size) == 0) {
                    if (path) {
                        lib_free(fname);
                        fname = NULL;
                        break;
                    }
                }
            }
        }
        if (path) {
            lib_free(fname);
            fname = NULL;
        }
        {
            unsigned char md5_result[16];
            char str[33];
            log_message("%s ", lbxinfo[i].filename);
            MD5_Init(&ctx);
            MD5_Update(&ctx, buf, lbxinfo[i].size);
            MD5_Final(md5_result, &ctx);
            for (int i = 0; i < 16; ++i) {
                sprintf(&str[i * 2], "%02x", md5_result[i]);
            }
            str[32] = '\0';
            if (strcmp(str, lbxinfo[i].md5)) {
                result = false;
                break;
            }
        }
    }
    log_message("Done!\n");
    return result;
}

static FILE *lbxfile_try_fopen(const char *path, const char *filename)
{
    FILE *fd;
    char *fname;
    if (path) {
        fname = util_concat(path, FSDEV_DIR_SEP_STR, filename, NULL);
    }
    fd = fopen(path ? fname : filename, "rb");
    if (path) {
        lib_free(fname);
        fname = NULL;
    }
    if (!fd) {
        char buf[32];
        const char *p = filename;
        char *q = buf;
        while (*p) {
            *q++ = toupper(*p++);
        }
        *q = 0;
        if (path) {
            fname = util_concat(path, FSDEV_DIR_SEP_STR, buf, NULL);
        }
        fd = fopen(path ? fname : buf, "rb");
        if (path) {
            lib_free(fname);
            fname = NULL;
        }
    }
    return fd;
}

static int lbxfile_open(struct lbx_s *p, const char *filename)
{
    uint16_t v;

    p->fd = lbxfile_try_fopen(os_get_path_data(), filename);

    if (!p->fd) {
        log_error("opening file '%s'!\n", filename);
        goto fail;
    }

    p->header = lib_malloc(LBX_HEADER_LEN);

    if (fread(p->header, LBX_HEADER_LEN, 1, p->fd) < 1) {
        log_error("reading header of file '%s'!\n", filename);
        goto fail;
    }

    v = GET_LE_16(&p->header[0]);
    p->entries = v;
    v = GET_LE_16(&p->header[2]);
    if (v != 0xfead) {
        log_error("file '%s' has wrong signature 0x%04x!\n", filename, v);
        goto fail;
    }

    v = GET_LE_16(&p->header[4]);
    if (v != 0) {
        log_error("file '%s' has nonzero at offs 4: 0x%04x!\n", filename, v);
    }

    p->type = GET_LE_16(&p->header[6]);

    if (fseek(p->fd, 0, SEEK_END)) {
        log_error("error seeking file '%s'\n", filename);
        goto fail;
    }

    p->len = ftell(p->fd);
    rewind(p->fd);
    p->mode = LBX_MODE_FILE_OPEN;
    p->num_in_use = 0;
    return 0;
fail:
    if (p->fd) {
        fclose(p->fd);
        p->fd = NULL;
    }
    lib_free(p->header);
    p->header = NULL;
    return -1;
}

static void lbxfile_close(lbxfile_e i)
{
    switch (lbxtbl[i].mode) {
        case LBX_MODE_FILE_OPEN:
            fclose(lbxtbl[i].fd);
            lbxtbl[i].fd = NULL;
            lib_free(lbxtbl[i].header);
            lbxtbl[i].header = NULL;
            break;
        case LBX_MODE_MEMORY:
            lib_free(lbxtbl[i].data);
            lbxtbl[i].data = NULL;
            lbxtbl[i].header = NULL;
            break;
        case LBX_MODE_NONE:
            break;
    }
    lbxtbl[i].mode = LBX_MODE_NONE;
}

static int lbxfile_load(lbxfile_e i)
{
    if (lbxfile_open(&lbxtbl[i], lbxinfo[i].filename)) {
        goto fail;
    }
    if (lbxinfo[i].in_memory) {
        lbxtbl[i].data = lib_malloc(lbxtbl[i].len);
        if (fread(lbxtbl[i].data, lbxtbl[i].len, 1, lbxtbl[i].fd) < 1) {
            log_error("problem reading file %s\n", lbxinfo[i].filename);
            lib_free(lbxtbl[i].data);
            lbxtbl[i].data = NULL;
            goto fail;
        }
        fclose(lbxtbl[i].fd);
        lbxtbl[i].fd = NULL;
        lib_free(lbxtbl[i].header);
        lbxtbl[i].header = lbxtbl[i].data;
        lbxtbl[i].mode = LBX_MODE_MEMORY;
    }
    return 0;
    fail:
    exit(EXIT_FAILURE);
    return 1;
}

static uint8_t *lbx_extract(struct lbx_s *p, uint16_t i, uint32_t *len_ptr, const char *filename)
{
    if (!p) {
        return NULL;
    }
    if (i >= p->entries) {
        log_error("LBX: %s invalid id %i >= %i\n", filename, i, p->entries);
        return NULL;
    }
    uint32_t offs0, offs1, len;
    offs0 = GET_LE_32(&p->header[8 + i * 4]);
    offs1 = GET_LE_32(&p->header[8 + i * 4 + 4]);
    len = offs1 - offs0;
    if (len_ptr) {
        *len_ptr = len;
    }
    if (p->mode == LBX_MODE_FILE_OPEN) {
        if (fseek(p->fd, offs0, SEEK_SET)) {
            log_error("LBX: problem seeking %s to %i\n", filename, offs0);
            return NULL;
        }
        uint8_t *d = lib_malloc(len);
        if (fread(d, len, 1, p->fd) < 1) {
            log_error("LBX: problem reading file %s\n", filename);
            return NULL;
        }
        return d;
    } else if (p->mode == LBX_MODE_MEMORY) {
        return &p->data[offs0];
    } else {
        log_error("LBX: extract without open\n");
        return NULL;
    }
}

/* -------------------------------------------------------------------------- */

int lbxfile_init(void)
{
    memset(lbxtbl, 0, sizeof(lbxtbl));
    return 0;
}

void lbxfile_shutdown(void)
{
    for (lbxfile_e i = LBXFILE_BACKGRND; i < LBXFILE_NUM; ++i) {
        lbxfile_close(i);
    }
}

int lbxfile_find_dir(void)
{
    const char **paths = os_get_paths_data();
    const char **pp = paths;
    log_message_direct("LBX: seeking data directory\n");
    while (*pp) {
        FILE *fd;
        log_message("LBX: trying '%s'\n", *pp);
        if ((fd = lbxfile_try_fopen(*pp, "fonts.lbx"))) {
            fclose(fd);
            break;
        }
        ++pp;
    }
    if (!*pp) {
        log_error("could not find the LBX files! Try running with -data path_to_moo1.\n");
        return -1;
    } else {
        log_message("LBX: found data directory at '%s'\n", *pp);
        if (!lbxfile_check_md5(*pp)) {
            log_fatal_and_die("Incorrect LBX md5\n");
        }
        os_set_path_data(*pp);
    }
    return 0;
}

uint8_t *lbxfile_item_get(lbxfile_e file_id, uint16_t entry_id, uint32_t *len_ptr)
{
    uint8_t *p = NULL;
    {
        if (lbxtbl[file_id].mode == LBX_MODE_NONE) {
            if (lbxfile_load(file_id)) {
                goto fail;
            }
        }

        p = lbx_extract(&lbxtbl[file_id], entry_id, len_ptr, lbxinfo[file_id].filename);

        if (p == NULL) {
            goto fail;
        }
    }

    ++lbxtbl[file_id].num_in_use;
    return p;

fail:
    return NULL;
}

void lbxfile_item_release(lbxfile_e file_id, uint8_t *ptr)
{
    if (lbxtbl[file_id].num_in_use <= 0) {
        log_error("%s: release without use\n", lbxinfo[file_id].filename);
        return;
    }
    if (lbxtbl[file_id].mode == LBX_MODE_FILE_OPEN) {
        lib_free(ptr);
    } else if (lbxtbl[file_id].mode == LBX_MODE_MEMORY) {
        if (lbxtbl[file_id].type == LBX_TYPE_GFX) {
            /* restore gfx data to original state */
            lbxgfx_set_frame_0(ptr);
            lbxgfx_set_epage(ptr, 0);
        }
    }
    --lbxtbl[file_id].num_in_use;
    if (lbxtbl[file_id].num_in_use <= 0) {
        lbxfile_close(file_id);
    }
}

void lbxfile_item_release_file(lbxfile_e file_id)
{
    lbxfile_close(file_id);
}

const char *lbxfile_name(lbxfile_e file_id)
{
    return lbxinfo[file_id].filename;
}

lbxfile_e lbxfile_id(const char *filename)
{
    for (lbxfile_e j = LBXFILE_BACKGRND; j < LBXFILE_NUM; ++j) {
        if (strcmp(lbxfile_name(j), filename) == 0) {
            return j;
        }
    }
    return LBXFILE_NUM;
}

int lbxfile_type(lbxfile_e file_id)
{
    if ((lbxtbl[file_id].mode == LBX_MODE_NONE) && (lbxfile_load(file_id))) {
        goto fail;
    }
    return lbxtbl[file_id].type;
    fail:
    return -1;
}

int lbxfile_num_items(lbxfile_e file_id)
{
    if ((lbxtbl[file_id].mode == LBX_MODE_NONE) && (lbxfile_load(file_id))) {
        goto fail;
    }
    return lbxtbl[file_id].entries;
    fail:
    return -1;
}

const char *lbxfile_item_name(lbxfile_e file_id, uint16_t entry_id)
{
    if ((lbxtbl[file_id].mode == LBX_MODE_NONE) && (lbxfile_load(file_id))) {
        goto fail;
    }
    {
        struct lbx_s *p;
        p = &lbxtbl[file_id];
        if (p->mode == LBX_MODE_FILE_OPEN) {
            if (fseek(p->fd, 0x200 + entry_id * 32, SEEK_SET)) {
                log_error("problem seeking to %i (of %i)\n", 0x200 + entry_id * 32, p->len);
                goto fail;
            }
            if (fread(entrynamebuf, 32, 1, p->fd) < 1) {
                log_error("problem reading file\n");
                goto fail;
            }
            return entrynamebuf;
        } else {
            return (const char *)&p->data[0x200 + entry_id * 32];
        }
    }
    fail:
    return NULL;
}

int lbxfile_item_len(lbxfile_e file_id, uint16_t entry_id)
{
    if ((lbxtbl[file_id].mode == LBX_MODE_NONE) && (lbxfile_load(file_id))) {
        return -1;
    }
    {
        uint32_t offs0, offs1, len;
        struct lbx_s *p;
        p = &lbxtbl[file_id];
        offs0 = GET_LE_32(&p->header[8 + entry_id * 4]);
        offs1 = GET_LE_32(&p->header[8 + entry_id * 4 + 4]);
        len = offs1 - offs0;
        return len;
    }
}

int lbxfile_item_offs(lbxfile_e file_id, uint16_t entry_id)
{
    if ((lbxtbl[file_id].mode == LBX_MODE_NONE) && (lbxfile_load(file_id))) {
        return -1;
    }
    {
        uint32_t offs0;
        struct lbx_s *p;
        p = &lbxtbl[file_id];
        offs0 = GET_LE_32(&p->header[8 + entry_id * 4]);
        return offs0;
    }
}
