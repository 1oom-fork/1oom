#include "config.h"

#include <stdio.h>

#include "hw.h"
#include "hwsdl_audio.h"
#include "hwsdl_opt.h"
#include "log.h"
#include "options.h"
#include "types.h"

#ifdef HAVE_SDLMIXER
#include "fmt_mus.h"
#include "fmt_sfx.h"
#include "lib.h"
#endif /* HAVE_SDLMIXER */

/* -------------------------------------------------------------------------- */

#ifdef HAVE_SDLMIXER

static bool audio_initialized = false;
static int audio_rate = 0;

struct sfx_s {
    Mix_Chunk *chunk;
};
static int sfx_num = 0;
static struct sfx_s *sfxtbl = NULL;
static int sfx_playing;

struct mus_s {
    mus_type_t type;
    Mix_Music *music;
    Mix_MusicType sdlmtype;
    uint8_t *buf;   /* WAV music files need the data to be kept */
    bool loops;
};
static int mus_num = 0;
static struct mus_s *mustbl = NULL;
static int mus_playing;

/* -------------------------------------------------------------------------- */

static int get_slice_size(void)
{
    int limit;
    int n;
    limit = (opt_audiorate * opt_audioslice_ms) / 1000;
    /* Try all powers of two, not exceeding the limit. */
    for (n = 0; ; ++n) {
        /* 2^n <= limit < 2^n+1 ? */
        if ((1 << (n + 1)) > limit) {
            return (1 << n);
        }
    }
    /* Should never happen? */
    return 1024;
}

static Mix_MusicType mus_type_to_sdlm(mus_type_t type)
{
    switch (type) {
        case MUS_TYPE_LBXXMID:
        case MUS_TYPE_MIDI:
            return MUS_MID;
        case MUS_TYPE_WAV:
            return MUS_WAV;
        case MUS_TYPE_OGG:
            return MUS_OGG;
        case MUS_TYPE_FLAC:
            return MUS_FLAC;
        default:
            return MUS_NONE;
    }
}

/* -------------------------------------------------------------------------- */

int hw_audio_init(void)
{
    if (opt_audio_enabled) {
        int mixer_channels;
        uint16_t mixer_format;
        int slice = get_slice_size();
        if (Mix_OpenAudio(opt_audiorate, AUDIO_S16SYS, 2, slice) < 0) {
            log_error("initialising SDL_mixer (%i Hz, slice %i): %s\n", opt_audiorate, slice, Mix_GetError());
            return -1;
        }
        Mix_QuerySpec(&audio_rate, &mixer_format, &mixer_channels);
        if (mixer_channels != 2) {
            log_error("SDL_mixer gave %i channels instead of 2\n", mixer_channels);
            Mix_CloseAudio();
            return -1;
        }
        if (audio_rate != opt_audiorate) {
            log_warning("SDL_mixer gave %i Hz instead of %i Hz\n", audio_rate, opt_audiorate);
        }
        Mix_AllocateChannels(1);
        SDL_PauseAudio(0);
        sfx_playing = -1;
        mus_playing = -1;
        log_message("SDLA: init %i Hz slice %i\n", audio_rate, slice);
        log_message("SDLA: soundfonts '%s'\n", Mix_GetSoundFonts());
        if (hw_opt_sdlmixer_sf) {
            if (hw_audio_set_sdlmixer_sf(hw_opt_sdlmixer_sf) < 0) {
                Mix_CloseAudio();
                return -1;
            }
        }
        audio_initialized = true;
        {
            int volume;
            volume = opt_sfx_volume;
            opt_sfx_volume = -1;
            hw_audio_sfx_volume(volume);
            volume = opt_music_volume;
            opt_music_volume = -1;
            hw_audio_music_volume(volume);
        }
    }
    return 0;
}

void hw_audio_shutdown(void)
{
    if (audio_initialized) {
        log_message("SDLA: shutdown\n");
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        for (int i = 0; i < sfx_num; ++i) {
            hw_audio_sfx_release(i);
        }
        lib_free(sfxtbl);
        sfxtbl = NULL;
        lib_free(hw_opt_sdlmixer_sf);
        hw_opt_sdlmixer_sf = NULL;
        audio_initialized = false;
    }
}

int hw_audio_set_sdlmixer_sf(const char *path)
{
    log_message("SDLA: setting soundfont to '%s'\n", path);
    if (Mix_SetSoundFonts(path) < 0) {
        log_error("SDLA: failed to set soundfonts to '%s'\n", path);
        return -1;
    }
    return 0;
}

int hw_audio_music_init(int mus_index, const uint8_t *data_in, uint32_t len_in)
{
    const uint8_t *data = NULL;
    uint8_t *buf = NULL;
    uint32_t len = 0;
    struct mus_s *m;

    if (!audio_initialized) {
        return 0;
    }

    if (mus_index >= mus_num) {
        int old_mus_num = mus_num;
        mus_num = (mus_index + 1);
        mustbl = lib_realloc(mustbl, mus_num * sizeof(struct mus_s));
        for (int i = old_mus_num; i < mus_num; ++i) {
            mustbl[i].type = MUS_TYPE_UNKNOWN;
            mustbl[i].music = NULL;
            mustbl[i].buf = NULL;
        }
    }

    m = &mustbl[mus_index];

    if (m->type != MUS_TYPE_UNKNOWN) {
        hw_audio_music_release(mus_index);
    }

    m->type = fmt_mus_detect(data_in, len_in);
    switch (m->type) {
        case MUS_TYPE_LBXXMID:
            if (fmt_mus_convert_xmid(data_in, len_in, &buf, &len, &m->loops)) {
                data = buf;
            } else {
                m->type = MUS_TYPE_UNKNOWN;
            }
            break;
        case MUS_TYPE_WAV:
            if (fmt_sfx_convert(data_in, len_in, &buf, &len, NULL, audio_rate, true)) {
                data = buf;
                m->buf = buf;
                buf = NULL;
            } else {
                m->type = MUS_TYPE_UNKNOWN;
            }
            m->loops = false;   /* FIXME */
            break;
        case MUS_TYPE_UNKNOWN:
            break;
        default:
            data = data_in;
            len = len_in;
            m->loops = false;   /* FIXME */
            break;
    }

    m->sdlmtype = mus_type_to_sdlm(m->type);

    if (m->type == MUS_TYPE_UNKNOWN) {
        log_error("SDLA: failed to init music %i\n", mus_index);
        return -1;
    }
    {
        SDL_RWops *rw = SDL_RWFromConstMem(data, len);
        m->music = Mix_LoadMUSType_RW(rw, m->sdlmtype, 0);
        SDL_RWclose(rw);
    }
    lib_free(buf);
    if (!m->music) {
        log_error("SDLA: Mix_LoadMUSType_RW failed on music %i (type %i)\n", mus_index, m->type);
        m->type = MUS_TYPE_UNKNOWN;
        m->sdlmtype = MUS_NONE;
        return -1;
    }

    return 0;
}

void hw_audio_music_release(int mus_index)
{
    if (mus_index < mus_num) {
        if (mus_playing == mus_index) {
            hw_audio_music_stop();
        }
        if (mustbl[mus_index].music) {
            Mix_FreeMusic(mustbl[mus_index].music);
            mustbl[mus_index].music = NULL;
        }
        if (mustbl[mus_index].buf) {
            lib_free(mustbl[mus_index].buf);
            mustbl[mus_index].buf = NULL;
        }
        mustbl[mus_index].type = MUS_TYPE_UNKNOWN;
        mustbl[mus_index].sdlmtype = MUS_NONE;
    }
}

void hw_audio_music_play(int mus_index)
{
    if (audio_initialized && opt_music_enabled && (mus_index < mus_num)) {
        if (Mix_PlayingMusic()) {
            Mix_HaltMusic();
        }
        Mix_VolumeMusic(opt_music_volume);
        Mix_PlayMusic(mustbl[mus_index].music, mustbl[mus_index].loops ? -1 : 0);
        mus_playing = mus_index;
    }
}

void hw_audio_music_fadeout(void)
{
    if (audio_initialized && opt_music_enabled && Mix_PlayingMusic()) {
        Mix_FadeOutMusic(1000);
    }
}

void hw_audio_music_stop(void)
{
    if (audio_initialized && opt_music_enabled) {
        Mix_HaltMusic();
        mus_playing = -1;
    }
}

void hw_audio_music_volume(int volume)
{
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 128) {
        volume = 128;
    }
    if (audio_initialized && opt_music_enabled) {
        Mix_VolumeMusic(volume);
    }
    if (opt_music_volume != volume) {
        log_message("SDLA: music volume %i\n", volume);
        opt_music_volume = volume;
    }
}

int hw_audio_sfx_init(int sfx_index, const uint8_t *data_in, uint32_t len_in)
{
    uint8_t *data = NULL;
    uint32_t len = 0;

    if (!audio_initialized) {
        return 0;
    }

    if (sfx_index >= sfx_num) {
        int old_sfx_num = sfx_num;
        sfx_num = (sfx_index + 1);
        sfxtbl = lib_realloc(sfxtbl, sfx_num * sizeof(struct sfx_s));
        for (int i = old_sfx_num; i < sfx_num; ++i) {
            sfxtbl[i].chunk = NULL;
        }
    }

    if (sfxtbl[sfx_index].chunk) {
        hw_audio_sfx_release(sfx_index);
    }

    if (fmt_sfx_convert(data_in, len_in, &data, &len, NULL, audio_rate, true)) {
        sfxtbl[sfx_index].chunk = Mix_LoadWAV_RW(SDL_RWFromMem(data, len), 0);
        lib_free(data);
    } else {
        log_error("SDLA: failed to init sound %i\n", sfx_index);
        return -1;
    }
    return 0;
}

void hw_audio_sfx_release(int sfx_index)
{
    if (sfx_index < sfx_num) {
        if (sfxtbl[sfx_index].chunk) {
            if (sfx_playing == sfx_index) {
                hw_audio_sfx_stop();
            }
            Mix_FreeChunk(sfxtbl[sfx_index].chunk);
            sfxtbl[sfx_index].chunk = NULL;
        }
    }
}

void hw_audio_sfx_play(int sfx_index)
{
    if (audio_initialized && opt_sfx_enabled && (sfx_index < sfx_num)) {
        Mix_PlayChannel(0, sfxtbl[sfx_index].chunk, 0);
        sfx_playing = sfx_index;
    }
}

void hw_audio_sfx_stop(void)
{
    if (audio_initialized && Mix_Playing(0)) {
        Mix_HaltChannel(0);
        sfx_playing = -1;
    }
}

void hw_audio_sfx_volume(int volume)
{
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 128) {
        volume = 128;
    }
    if (audio_initialized && opt_sfx_enabled) {
        Mix_Volume(0, volume);
    }
    if (opt_sfx_volume != volume) {
        log_message("SDLA: sfx volume %i\n", volume);
        opt_sfx_volume = volume;
    }
}

#else /* !HAVE_SDLMIXER */

int hw_audio_init(void)
{
    if (opt_audio_enabled) {
        log_warning("SDLA: no audio due to missing SDL_mixer!\n");
    }
    return 0;
}

void hw_audio_shutdown(void)
{
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
void hw_audio_music_fadeout(void)
{
}
void hw_audio_music_stop(void)
{
}
void hw_audio_music_volume(int volume/*0..128*/)
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
void hw_audio_sfx_volume(int volume/*0..128*/)
{
}
#endif /* HAVE_SDLMIXER1 */
