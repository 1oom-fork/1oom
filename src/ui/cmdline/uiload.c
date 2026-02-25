#include "config.h"

#include <stdio.h>

#include "ui.h"
#include "save.h"
#include "types.h"
#include "uiinput.h"

/* -------------------------------------------------------------------------- */

static struct input_list_s lg_in[] = {
    { 0, "1", NULL, NULL },
    { 1, "2", NULL, NULL },
    { 2, "3", NULL, NULL },
    { 3, "4", NULL, NULL },
    { 4, "5", NULL, NULL },
    { 5, "6", NULL, NULL },
    { -1, "Q", "q", "(quit)" },
    { 0, NULL, NULL, NULL },
};

/* -------------------------------------------------------------------------- */

int ui_load_game(void)
{
    int savenum = 0;

    for (int i = 0; i < NUM_SAVES; ++i) {
        if (game_save_tbl_have_save[i]) {
            lg_in[savenum].value = i;
            lg_in[savenum].display = game_save_tbl_name[i];
            ++savenum;
        }
    }

    if (savenum == 0) {
        return -1;
    }

    return ui_input_list("Load game", "> ", lg_in);
}
