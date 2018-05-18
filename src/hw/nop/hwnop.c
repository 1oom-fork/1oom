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

uint32_t hw_get_time_us(void)
{
    return 0;
}

int hw_event_handle(void)
{
    return 0;
}

int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h)
{
    return 0;
}

int hw_video_init(int w, int h)
{
    return 0;
}

void hw_video_set_palette(uint8_t *palette, int first, int num)
{
}

uint8_t *hw_video_get_buf(void)
{
    return 0;
}

uint8_t *hw_video_draw_buf(void)
{
    return 0;
}

int hw_audio_music_init(int mus_index, const uint8_t *data, uint32_t len)
{
    return 0;
}
void hw_audio_music_release(int mus_index)
{
}
void hw_audio_music_play(int mus_index)
{
}
void hw_audio_music_stop(void)
{
}
int hw_audio_sfx_init(int sfx_index, const uint8_t *data, uint32_t len)
{
    return 0;
}
void hw_audio_sfx_release(int sfx_index)
{
}
void hw_audio_sfx_play(int sfx_index)
{
}
void hw_audio_sfx_stop(void)
{
}
