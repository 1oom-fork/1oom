#include "config.h"

#include <stdio.h>

#include "ui.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s ui_cmdline_options[] = {
    { NULL, 0, NULL, NULL, NULL, NULL }
};

/* -------------------------------------------------------------------------- */

const char *idstr_ui = "nop";

/* bool ui_use_audio intentionally left undeclared */

/* -------------------------------------------------------------------------- */

int ui_early_init(void)
{
    return 0;
}

int ui_init(void)
{
    return 0;
}

void ui_shutdown(void)
{
}
