#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pbx.h"
#include "lbx.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "types.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

#ifdef FEATURE_MODEBUG
int opt_modebug = 0;
#endif

bool game_num_patch(const char *numid, const int32_t *patchnums, int first, int num)
{
    return false;
}

bool game_str_patch(const char *strid, const char *patchstr, int i)
{
    return false;
}

lbxfile_e lbxfile_id(const char *filename)
{
    return LBXFILE_NUM;
}

void lbxfile_add_patch(lbxfile_e file_id, uint16_t i, uint8_t *data, uint32_t len, const char *patchfilename)
{
}

/* -------------------------------------------------------------------------- */

struct pbxdump_s {
    FILE *fd;
    const char *prefix;
    const char *prefix_file;
    bool flag_write;
};

/* -------------------------------------------------------------------------- */

static int pbxdump_write(const char *prefix, const char *suffix, bool flag_write, const uint8_t *data, uint32_t len)
{
    if (flag_write) {
        char *fname = util_concat(prefix, suffix, NULL);
        if (util_file_save(fname, data, len) < 0) {
            log_error("writing '%s'!\n", fname);
            lib_free(fname);
            return -1;
        }
        lib_free(fname);
    }
    return 0;
}

static int pbx_cb_name(void *ctx, const char *filename, int pbxi, char *str, uint32_t len)
{
    struct pbxdump_s *d = ctx;
    bool can_print = true;
    if (len > 100) {
        can_print = false;
    } else {
        for (uint32_t i = 0; i < len; ++i) {
            char c;
            c = str[i];
            if ((c < 0x20) || (c > 0x7e)) {
                can_print = false;
                break;
            }
        }
    }
    if (can_print) {
        if (fprintf(d->fd, "0,\"%s\"\n", str) < 0) {
            return -1;
        }
        return 0;
    } else {
        if (fprintf(d->fd, "0,%s_name.txt\n", d->prefix_file) < 0) {
            return -1;
        }
        return pbxdump_write(d->prefix, "_name.txt", d->flag_write, (const uint8_t *)str, len);
    }
}

static int pbx_cb_desc(void *ctx, const char *filename, int pbxi, char *str, uint32_t len)
{
    struct pbxdump_s *d = ctx;
    if (fprintf(d->fd, "1,%s_desc.txt\n", d->prefix_file) < 0) {
        return -1;
    }
    return pbxdump_write(d->prefix, "_desc.txt", d->flag_write, (const uint8_t *)str, len);
}

static int pbx_cb_lbxp(void *ctx, const char *filename, int pbxi, const char *id, uint16_t itemi, uint8_t *data, uint32_t len)
{
    struct pbxdump_s *d = ctx;
    char buf[32];
    char *p = buf;
    *p++ = '_';
    for (const char *q = id; *q != '.';) {
        *p++ = *q++;
    }
    sprintf(p, "_%03u.bin", itemi);
    if (fprintf(d->fd, "2,%s,%u,%s%s\n", id, itemi, d->prefix_file, buf) < 0) {
        return -1;
    }
    return pbxdump_write(d->prefix, buf, d->flag_write, data, len);
}

static bool pbx_cb_strp(void *ctx, const char *filename, int pbxi, const char *id, const char *patchstr, int itemi, uint32_t len)
{
    struct pbxdump_s *d = ctx;
    if (fprintf(d->fd, "3,%s,%i,\"%s\"\n", id, itemi, patchstr) < 0) {
        return false;
    }
    return true;
}

static bool pbx_cb_nump(void *ctx, const char *filename, int pbxi, const char *id, const int32_t *patchnums, int first, int num)
{
    struct pbxdump_s *d = ctx;
    if (fprintf(d->fd, "4,%s,%i", id, first) < 0) {
        return false;
    }
    for (int i = 0; i < num; ++i) {
        if (fprintf(d->fd, ",%i", patchnums[i]) < 0) {
            return false;
        }
    }
    if (fprintf(d->fd, "\n") < 0) {
        return -1;
    }
    return true;
}

static int dump_pbx(const char *filename, const char *prefix_in, bool flag_write)
{
    struct pbx_add_cbs cbs;
    struct pbxdump_s ctx;
    char *fname_pbxin = NULL, *dir = NULL, *fnam = NULL, *prefix = NULL;
    int res;
    cbs.name = pbx_cb_name;
    cbs.desc = pbx_cb_desc;
    cbs.lbxp = pbx_cb_lbxp;
    cbs.strp = pbx_cb_strp;
    cbs.nump = pbx_cb_nump;
    ctx.flag_write = flag_write;
    ctx.prefix = prefix = lib_stralloc(prefix_in ? prefix_in : filename);
    {
        char *p;
        p = strrchr(prefix, '.');
        if (p && (strcasecmp(p + 1, "pbx") == 0) && ((*(p + 4)) == '\0')) {
            *p = '\0';
        }
    }
    util_fname_split(ctx.prefix, &dir, &fnam);
    if (dir) {
        if (flag_write && os_make_path(dir)) {
            log_error("creating path '%s'\n", dir);
            return -1;
        }
        fname_pbxin = util_concat(dir, FSDEV_DIR_SEP_STR, fnam, ".pbxin", NULL);
        ctx.prefix_file = fnam;
    } else {
        ctx.prefix_file = ctx.prefix;
        fname_pbxin = util_concat(ctx.prefix, ".pbxin", NULL);
    }
    if (flag_write) {
        log_message("PBXDUMP: writing to '%s'\n", fname_pbxin);
        ctx.fd = fopen(fname_pbxin, "w+");
    } else {
        log_message("PBXDUMP: dry run, would use prefix '%s'\n", ctx.prefix);
        ctx.fd = stdout;
    }
    res = pbx_add_file(filename, &cbs, &ctx);
    if (flag_write) {
        fclose(ctx.fd);
        ctx.fd = NULL;
    }
    lib_free(fname_pbxin);
    lib_free(dir);
    lib_free(fnam);
    lib_free(prefix);
    return res;
}

/* -------------------------------------------------------------------------- */

static void show_usage(void)
{
    fprintf(stderr, "Usage:\n"
                    "  1oom_pbxdump [OPTIONS] IN.PBX [OUTPREFIX]\n"
                    "Options:\n"
                    "  -w       - write files\n"
           );
}

/* -------------------------------------------------------------------------- */

int main_1oom(int argc, char **argv)
{
    const char *filename_in;
    const char *prefix_out = NULL;
    bool flag_write = false;
    int i;
    i = 1;
    if (i >= argc) {
        show_usage();
        return 1;
    }
    while (argv[i][0] == '-') {
        if (argv[i][2] != '\0') {
            show_usage();
            return 1;
        }
        switch (argv[i][1]) {
            case 'w':
                flag_write = true;
                break;
            default:
                show_usage();
                return 1;
        }
        ++i;
    }
    if (i >= argc) {
        show_usage();
        return 1;
    }
    filename_in = argv[i++];
    if (i < argc) {
        prefix_out = argv[i++];
        if (i < argc) {
            show_usage();
            return 1;
        }
    }
    return dump_pbx(filename_in, prefix_out, flag_write);
}
