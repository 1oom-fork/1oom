/* already included by includer */
#if 0
#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "hwalleg_opt.h"
#include "options.h"
#include "types.h"
#endif

/* -------------------------------------------------------------------------- */

bool hw_opt_fullscreen = false;

/* -------------------------------------------------------------------------- */

const struct cfg_items_s hw_cfg_items[] = {
    CFG_ITEM_END
};

/* -------------------------------------------------------------------------- */

const struct uiopt_s hw_uiopts[] = {
    UIOPT_ITEM_END
};

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s hw_cmdline_options[] = {
    { NULL, 0, NULL, NULL, NULL, NULL }
};
