#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "main.h"
#include "options.h"
#include "os.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s hw_cmdline_options[] = {
    { NULL, 0, NULL, NULL, NULL, NULL }
};

const struct cmdline_options_s hw_cmdline_options_extra[] = {
    { NULL, 0, NULL, NULL, NULL, NULL }
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

uint32_t hw_get_time_us(void)
{
    return os_get_time_us();
}
