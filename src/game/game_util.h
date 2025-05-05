#ifndef INC_1OOM_GAME_UTIL_H
#define INC_1OOM_GAME_UTIL_H

#include "config.h"
#include "types.h"

extern void util_table_remove_item_keep_order(int itemi, void *tbl, int itemsz, int itemnum);
extern void util_table_remove_item_keep_order_zero(int itemi, void *tbl, int itemsz, int itemnum);
extern void util_table_remove_item_any_order(int itemi, void *tbl, int itemsz, int itemnum);

#endif
