#include "config.h"

#include <stdlib.h>

#include "main.h"
#include "os.h"
#include "saveconv_main.h"

/* -------------------------------------------------------------------------- */

const char *idstr_main = "saveconv";

bool main_use_lbx = false;
bool ui_use_audio = false;

/* -------------------------------------------------------------------------- */

int main_1oom(int argc, char **argv)
{
    if (saveconv_main_init()) {
        return 1;
    }
    if (options_parse_early(argc, argv)) {
        return 1;
    }
    atexit(saveconv_main_shutdown);
    if (os_init()) {
        return 2;
    }
    if (options_parse(argc, argv)) {
        return 3;
    }
    return main_do();
}
