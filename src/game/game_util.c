#include "config.h"

#include <string.h>

#include "types.h"

/* -------------------------------------------------------------------------- */

void util_table_remove_item_keep_order(int itemi, void *tbl, int itemsz, int itemnum)
{
    if ((itemi < 0) || (itemi >= (itemnum - 1))) {
        return;
    }
    memmove(((uint8_t *)tbl) + itemi * itemsz, ((uint8_t *)tbl) + (itemi + 1) * itemsz, (itemnum - 1 - itemi) * itemsz);
}

void util_table_remove_item_keep_order_zero(int itemi, void *tbl, int itemsz, int itemnum)
{
    if ((itemi < 0) || (itemi >= itemnum)) {
        return;
    }
    if (itemi < (itemnum - 1)) {
        memmove(((uint8_t *)tbl) + itemi * itemsz, ((uint8_t *)tbl) + (itemi + 1) * itemsz, (itemnum - 1 - itemi) * itemsz);
    }
    memset(((uint8_t *)tbl) + (itemnum - 1) * itemsz, 0, itemsz);
}

void util_table_remove_item_any_order(int itemi, void *tbl, int itemsz, int itemnum)
{
    if ((itemi < 0) || (itemi >= (itemnum - 1))) {
        return;
    }
    memcpy(((uint8_t *)tbl) + itemi * itemsz, ((uint8_t *)tbl) + (itemnum - 1) * itemsz, itemsz);
}
