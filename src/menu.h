#ifndef INC_1OOM_MENU_H
#define INC_1OOM_MENU_H

#include "kbd.h"

typedef enum {
    MAIN_MENU_ITEM_TYPE_NONE,
    MAIN_MENU_ITEM_TYPE_RETURN,
    MAIN_MENU_ITEM_TYPE_PAGE,
    MAIN_MENU_ITEM_TYPE_PAGE_BACK,
    MAIN_MENU_ITEM_TYPE_FUNCTION,
    MAIN_MENU_ITEM_TYPE_BOOL,
    MAIN_MENU_ITEM_TYPE_INT,
    MAIN_MENU_ITEM_TYPE_ENUM,
    MAIN_MENU_ITEM_TYPE_STR,
} main_menu_item_type_t;

struct main_menu_item_data_s {
    main_menu_item_type_t type;
    void (*func) (void *);
    bool (*is_active) (void *);
    const char *text;
    const char *(*get_text_value) (int);
    void *value_ptr;
    int action_i;
    int value_min;
    int value_max;
    mookey_t key;
};

#endif
