#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "cfg.h"
#include "main.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s hw_cmdline_options[] = {
    { NULL, 0, NULL, NULL, NULL, NULL }
};

const struct cmdline_options_s hw_cmdline_options_extra[] = {
    { NULL, 0, NULL, NULL, NULL, NULL }
};

const struct cfg_items_s hw_cfg_items[] = {
    CFG_ITEM_END
};

const struct cfg_items_s hw_cfg_items_extra[] = {
    CFG_ITEM_END
};

/* -------------------------------------------------------------------------- */

const char *idstr_hw = "nop";

int main(int argc, char **argv)
{
    return main_1oom(argc, argv);
}

int hw_early_init(void)
{
    return 0;
}

int hw_init(void)
{
    return 0;
}

void hw_shutdown(void)
{
}

void hw_log_message(const char *msg)
{
    fputs(msg, stdout);
}

void hw_log_warning(const char *msg)
{
    fputs(msg, stderr);
}

void hw_log_error(const char *msg)
{
    fputs(msg, stderr);
}

uint8_t *hw_video_get_buf(void)
{
    return 0;
}
