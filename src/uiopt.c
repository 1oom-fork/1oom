#include "config.h"

#include "uiopt.h"
#include "hw.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

static bool uiopt_audio_sfx_toggle(void)
{
    opt_sfx_enabled = !opt_sfx_enabled;
    return true;
}

static bool uiopt_audio_music_toggle(void)
{
    opt_music_enabled = !opt_music_enabled;
    return true;
}

/* -------------------------------------------------------------------------- */

const struct uiopt_s uiopts_audio[] = {
    UIOPT_ITEM_BOOL("SFX", opt_sfx_enabled, uiopt_audio_sfx_toggle),
    UIOPT_ITEM_SLIDER_CALL(opt_sfx_volume, hw_audio_sfx_volume, 0, 128),
    UIOPT_ITEM_BOOL("Music", opt_music_enabled, uiopt_audio_music_toggle),
    UIOPT_ITEM_SLIDER_CALL(opt_music_volume, hw_audio_music_volume, 0, 128),
    UIOPT_ITEM_END
};
