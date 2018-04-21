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

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

int pbx_add_file(const char *filename)
{
    FILE *fd;
    uint8_t buf[PBX_ITEM_HEADER_LEN];
    uint32_t version;
    uint32_t items;
    uint8_t *data = NULL;
    int res = -1;

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
        uint32_t offs, len;
        uint16_t type, itemi;
        const char *id;
        lbxfile_e lbxf;
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
        data = lib_malloc(len);
        if (fread(data, len, 1, fd) < 1) {
            log_error("reading item %i data (len %i) of file '%s'!\n", i, len, filename);
            goto fail;
        }
        switch (type) {
            case PBX_ITEM_TYPE_NAME:
                log_message("PBX: name '%s'\n", data);
                break;
            case PBX_ITEM_TYPE_DESC:
                /* ignore */
                break;
            case PBX_ITEM_TYPE_LBXP:
                lbxf = lbxfile_id(id);
                if (lbxf < LBXFILE_NUM) {
                    lbxfile_add_patch(lbxf, itemi, data, len, filename);
                    data = NULL;
                } else {
                    log_error("patch file '%s' item %i base filename '%s' not recognized!\n", filename, i, id);
                    goto fail;
                }
                break;
            case PBX_ITEM_TYPE_STRP:
                if (!game_str_patch(id, (const char *)data, itemi)) {
                    log_error("patch file '%s' item %i strid '%s' (%i) invalid!\n", filename, i, id, itemi);
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
                    if (!game_num_patch(id, nums, itemi, numnum)) {
                        log_error("patch file '%s' item %i numid '%s' (%i) invalid!\n", filename, i, id, itemi);
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
