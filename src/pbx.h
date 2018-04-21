#ifndef INC_1OOM_PBX_H
#define INC_1OOM_PBX_H

#include "types.h"

#define PBX_HEADER_LEN 16
#define PBX_MAGIC  "1oomPBX "
#define PBX_OFFS_VERSION   8
#define PBX_OFFS_ITEMS     12
#define PBX_VERSION   1

#define PBX_ITEM_HEADER_LEN 32
#define PBX_OFFS_ITEM_LEN   0
#define PBX_OFFS_ITEM_TYPE  4
#define PBX_OFFS_ITEM_INDEX 6
#define PBX_OFFS_ITEM_ID    8
#define PBX_ITEM_ID_LEN (PBX_ITEM_HEADER_LEN - PBX_OFFS_ITEM_ID)

typedef enum {
    PBX_ITEM_TYPE_NAME = 0,
    PBX_ITEM_TYPE_DESC,
    PBX_ITEM_TYPE_LBXP,
    PBX_ITEM_TYPE_STRP,
    PBX_ITEM_TYPE_NUMP
} pbx_item_type_t;

extern int pbx_add_file(const char *filename);

#endif
