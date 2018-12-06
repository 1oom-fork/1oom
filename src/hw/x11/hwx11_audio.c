#include "config.h"

#include "hw_internal.h"

#include "log.h"

void hw_audio_shutdown(void)
{

}

int hw_audio_init(void)
{
    return 0;
}

int hw_audio_music_init(int ix, const uint8_t *data, uint32_t len)
{
    return 0;
}

void hw_audio_music_fadeout(void)
{

}

void hw_audio_music_play(int ix)
{

}

void hw_audio_music_release(int ix)
{

}

void hw_audio_music_stop(void)
{

}

void hw_audio_music_volume(int volume)
{

}

int hw_audio_sfx_batch_end(void)
{
    return 0;
}

int hw_audio_sfx_batch_start(int sfx_index_max)
{
    return 0;
}

int hw_audio_sfx_init(int ix, const uint8_t *data, uint32_t len)
{
    return 0;
}

void hw_audio_sfx_play(int ix)
{

}

void hw_audio_sfx_release(int ix)
{

}

void hw_audio_sfx_stop(void)
{

}

void hw_audio_sfx_volume(int volume)
{

}

int hw_audio_set_soundfont(const char *path)
{
    log_message("Audio: hw_audio_set_soundfont not implemented\n");
    return 0;
}
