#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pbx.h"
#include "bits.h"
#include "gameapi.h"
#include "lbx.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static int pbx_cb_name(void *ctx, const char *filename, int pbxi, char *str, uint32_t len)
{
    log_message("PBX: name '%s'\n", str);
    return 0;
}

static int pbx_cb_desc(void *ctx, const char *filename, int pbxi, char *str, uint32_t len)
{
    /* ignore */
    return 0;
}

static int pbx_cb_lbxp(void *ctx, const char *filename, int pbxi, const char *id, uint16_t itemi, uint8_t *data, uint32_t len)
{
    lbxfile_e lbxf = lbxfile_id(id);
    if (lbxf < LBXFILE_NUM) {
        lbxfile_add_patch(lbxf, itemi, data, len, filename);
        return 1;
    } else {
        log_error("patch file '%s' item %i base filename '%s' not recognized!\n", filename, pbxi, id);
        return -1;
    }
}

static int pbx_cb_lbxo(void *ctx, const char *filename, int pbxi, const char *id, uint16_t itemi, uint8_t *data, uint32_t len, uint32_t itemoffs)
{
    lbxfile_e lbxf = lbxfile_id(id);
    if (lbxf < LBXFILE_NUM) {
        lbxfile_add_overwrite(lbxf, itemi, itemoffs, data, len, filename);
        return 1;
    } else {
        log_error("patch file '%s' item %i base filename '%s' not recognized!\n", filename, pbxi, id);
        return -1;
    }
}

static bool pbx_cb_strp(void *ctx, const char *filename, int pbxi, const char *id, const char *patchstr, int itemi, uint32_t len)
{
    if (!game_str_patch(id, patchstr, itemi)) {
        log_error("patch file '%s' item %i strid '%s' (%i) invalid!\n", filename, pbxi, id, itemi);
        return false;
    }
    return true;
}

static bool pbx_cb_nump(void *ctx, const char *filename, int pbxi, const char *id, const int32_t *patchnums, int first, int num)
{
    if (!game_num_patch(id, patchnums, first, num)) {
        log_error("patch file '%s' item %i numid '%s' (%i) invalid!\n", filename, pbxi, id, first);
        return false;
    }
    return true;
}

/* -------------------------------------------------------------------------- */

int pbx_add_file(const char *filename, struct pbx_add_cbs *cbs_in, void *ctx)
{
    FILE *fd;
    uint8_t buf[PBX_ITEM_HEADER_LEN + 4];
    uint32_t version;
    uint32_t items;
    uint8_t *data = NULL;
    int res = -1;
    struct pbx_add_cbs cbs;

    cbs.name = (cbs_in && cbs_in->name) ? cbs_in->name : pbx_cb_name;
    cbs.desc = (cbs_in && cbs_in->desc) ? cbs_in->desc : pbx_cb_desc;
    cbs.lbxp = (cbs_in && cbs_in->lbxp) ? cbs_in->lbxp : pbx_cb_lbxp;
    cbs.strp = (cbs_in && cbs_in->strp) ? cbs_in->strp : pbx_cb_strp;
    cbs.nump = (cbs_in && cbs_in->nump) ? cbs_in->nump : pbx_cb_nump;
    cbs.lbxo = (cbs_in && cbs_in->lbxo) ? cbs_in->lbxo : pbx_cb_lbxo;

    log_message("PBX: applying patch file '%s'\n", filename);

    fd = fopen(filename, "rb");
    if (!fd) {
        log_error("opening patch file '%s'!\n", filename);
        goto fail;
    }
    if (fread(buf, PBX_HEADER_LEN, 1, fd) < 1) {
        log_error("reading header of file '%s'!\n", filename);
        goto fail;
    }
    if (memcmp(buf, (const uint8_t *)PBX_MAGIC, 8) != 0) {
        log_error("invalid PBX header on file '%s'!\n", filename);
        goto fail;
    }
    version = GET_LE_32(&buf[PBX_OFFS_VERSION]);
    items = GET_LE_32(&buf[PBX_OFFS_ITEMS]);
    if (version != PBX_VERSION) {
        log_error("invalid PBX version %i on file '%s'! (expected %i)\n", version, filename, PBX_VERSION);
        goto fail;
    }
    for (int i = 0; i < items; ++i) {
        uint32_t offs, len, itemoffs;
        uint16_t type, itemi;
        const char *id;
        int ri;
        if (fseek(fd, PBX_HEADER_LEN + i * 4, SEEK_SET)) {
            log_error("problem seeking to item %i offs %i\n", i, PBX_HEADER_LEN + i * 4);
            goto fail;
        }
        if (fread(buf, 4, 1, fd) < 1) {
            log_error("problem reading file\n");
            goto fail;
        }
        offs = GET_LE_32(buf);
        if (fseek(fd, offs, SEEK_SET)) {
            log_error("problem seeking to item %i header at %i\n", i, offs);
            goto fail;
        }
        if (fread(buf, PBX_ITEM_HEADER_LEN, 1, fd) < 1) {
            log_error("reading item %i header of file '%s'!\n", i, filename);
            goto fail;
        }
        len = GET_LE_32(&(buf[PBX_OFFS_ITEM_LEN]));
        type = GET_LE_16(&(buf[PBX_OFFS_ITEM_TYPE]));
        itemi = GET_LE_16(&(buf[PBX_OFFS_ITEM_INDEX]));
        id = (const char *)&(buf[PBX_OFFS_ITEM_ID]);
        switch (type) {
            case PBX_ITEM_TYPE_LBXO:
                if (fread(&(buf[PBX_OFFS_ITEM_OFFS]), 4, 1, fd) < 1) {
                    log_error("reading item %i offset (len %i) of file '%s'!\n", i, len, filename);
                    goto fail;
                }
                itemoffs = GET_LE_32(&(buf[PBX_OFFS_ITEM_OFFS]));
                len -= 4;
                break;
            default:
                itemoffs = 0;
                break;
        }
        data = lib_malloc(len);
        if (fread(data, len, 1, fd) < 1) {
            log_error("reading item %i data (len %i) of file '%s'!\n", i, len, filename);
            goto fail;
        }
        switch (type) {
            case PBX_ITEM_TYPE_NAME:
                ri = cbs.name(ctx, filename, i, (char *)data, len);
                if (ri > 0) {
                    data = NULL;
                } else if (ri < 0) {
                    goto fail;
                }
                break;
            case PBX_ITEM_TYPE_DESC:
                ri = cbs.desc(ctx, filename, i, (char *)data, len);
                if (ri > 0) {
                    data = NULL;
                } else if (ri < 0) {
                    goto fail;
                }
                break;
            case PBX_ITEM_TYPE_LBXP:
                ri = cbs.lbxp(ctx, filename, i, id, itemi, data, len);
                if (ri > 0) {
                    data = NULL;
                } else if (ri < 0) {
                    goto fail;
                }
                break;
            case PBX_ITEM_TYPE_LBXO:
                ri = cbs.lbxo(ctx, filename, i, id, itemi, data, len, itemoffs);
                if (ri > 0) {
                    data = NULL;
                } else if (ri < 0) {
                    goto fail;
                }
                break;
            case PBX_ITEM_TYPE_STRP:
                if (!cbs.strp(ctx, filename, i, id, (const char *)data, itemi, len)) {
                    goto fail;
                }
                break;
            case PBX_ITEM_TYPE_NUMP:
                {
                    uint8_t *p;
                    int32_t *nums;
                    int numnum;
                    numnum = len / 4;
                    nums = lib_malloc(numnum * sizeof(int32_t));
                    p = data;
                    for (int j = 0; j < numnum; ++j) {
                        nums[j] = (int32_t)GET_LE_32(p);
                        p += 4;
                    }
                    if (!cbs.nump(ctx, filename, i, id, nums, itemi, numnum)) {
                        lib_free(nums);
                        goto fail;
                    }
                    lib_free(nums);
                }
                break;
            default:
                log_error("patch file '%s' item %i type %i not recognized!\n", filename, i, type);
                goto fail;
        }
        lib_free(data);
        data = NULL;
    }
    res = 0;
fail:
    if (data) {
        lib_free(data);
        data = NULL;
    }
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    return res;
}
