#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "hw.h"
#include "lbx.h"
#include "log.h"
#include "os.h"
#include "ui.h"

/* -------------------------------------------------------------------------- */

static int main_early_init(void)
{
    if (0
      || os_early_init()
      || hw_early_init()
      || ui_early_init()
      || lbxfile_init()
    ) {
        return 1;
    }
    return 0;
}

static int main_init(void)
{
    if (0
      || os_init()
      || hw_init()
      || ui_init()
    ) {
        return 1;
    }
    return 0;
}

static void main_shutdown(void)
{
    main_do_shutdown();
    lbxfile_shutdown();
    ui_shutdown();
    hw_shutdown();
    os_shutdown();
}

/* -------------------------------------------------------------------------- */

int main_1oom(int argc, char **argv)
{
    if (main_early_init()) {
        return 1;
    }
    log_message("1oom: main:%s ui:%s hw:%s os:%s\n", idstr_main, idstr_ui, idstr_hw, idstr_os);
    if (options_parse_early(argc, argv)) {
        return 1;
    }
    atexit(main_shutdown);
    if (main_init()) {
        return 2;
    }
    if (options_parse(argc, argv)) {
        return 3;
    }
    if (lbxfile_find_dir()) {
        return 4;
    }
    log_message("USER: '%s'\n", os_get_path_user());
    return main_do();
}
