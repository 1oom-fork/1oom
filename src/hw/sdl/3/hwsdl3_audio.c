#include "config.h"

#ifdef HAVE_SDL3MIXER
#include "SDL3/SDL.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "SDL3/SDL_iostream.h"

#include "fmt_mus.h"
#include "fmt_sfx.h"
#include "lib.h"
#endif /* HAVE_SDL3MIXER */

#include <stdio.h>

#include "hw.h"
#include "hwsdl_audio.h"
#include "hwsdl_opt.h"
#include "log.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#ifdef HAVE_SDL3MIXER

MIX_Mixer *mixer;
SDL_PropertiesID props;

static bool audio_initialized = false;
static int audio_rate = 0;

struct sfx_s {
    MIX_Audio *chunk;
};
static int sfx_num = 0;
static struct sfx_s *sfxtbl = NULL;
static int sfx_playing;

struct mus_s {
    mus_type_t type;
    MIX_Audio *music;
    uint8_t *buf;   /* WAV music files need the data to be kept */
    bool loops;
};
static int mus_num = 0;
static struct mus_s *mustbl = NULL;
static int mus_playing;

/* -------------------------------------------------------------------------- */

int hw_audio_init(void)
{
    if (opt_audio_enabled) {
        SDL_AudioSpec spec;
        spec.format = SDL_AUDIO_S16;
        spec.channels = 2;
        spec.freq = opt_audiorate;
        if (!MIX_Init()) {
            log_error("MIX_Init() failed: %s\n", SDL_GetError());
            return -1;
        }
        mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
        if (!mixer) {
            log_error("initialising SDL_mixer (%i Hz): %s\n", opt_audiorate, SDL_GetError());
            return -1;
        }
        props = MIX_GetMixerProperties(mixer);
        if (!MIX_GetMixerFormat(mixer, &spec)) {
            log_error("Failed to read MIX_Mixer properties\n");
            MIX_DestroyMixer(mixer);
            return -1;
        }
        audio_rate = spec.freq;
        if (spec.channels != 2) {
            log_warning("SDL_mixer gave %i output channels instead of 2\n", spec.channels);
        }
        if (audio_rate != opt_audiorate) {
            log_warning("SDL_mixer gave %i Hz instead of %i Hz\n", spec.freq, opt_audiorate);
        }
        SDL_ResumeAudioDevice(0);
        sfx_playing = -1;
        mus_playing = -1;
        log_message("SDLA: init %i Hz\n", audio_rate);
        log_message("SDLA: soundfonts '%s'\n", SDL_GetStringProperty(props, "SDL_MIXER_SOUNDFONTS_STRING", NULL));
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
        MIX_DestroyMixer(mixer);
        MIX_Quit();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        for (int i = 0; i < sfx_num; ++i) {
            hw_audio_sfx_release(i);
        }
        lib_free(sfxtbl);
        sfxtbl = NULL;
        audio_initialized = false;
    }
}

int hw_audio_set_sdlmixer_sf(const char *path)
{
    log_message("SDLA: setting soundfont to '%s'\n", path);
    if (!SDL_SetStringProperty(props, "SDL_mixer.decoder.fluidsynth.soundfont_path", path)) {
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

    if (m->type == MUS_TYPE_UNKNOWN) {
        log_error("SDLA: failed to init music %i\n", mus_index);
        return -1;
    }

    {
        SDL_IOStream *rw = SDL_IOFromConstMem(data, len);
        m->music = MIX_LoadAudio_IO(mixer, rw, 1, 0);
        SDL_CloseIO(rw);
    }
    lib_free(buf);
    if (!m->music) {
        log_error("SDLA: MIX_LoadAudio_IO failed on music %i: %s\n", mus_index, SDL_GetError());
        m->type = MUS_TYPE_UNKNOWN;
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
            MIX_DestroyAudio(mustbl[mus_index].music);
            mustbl[mus_index].music = NULL;
        }
        if (mustbl[mus_index].buf) {
            lib_free(mustbl[mus_index].buf);
            mustbl[mus_index].buf = NULL;
        }
        mustbl[mus_index].type = MUS_TYPE_UNKNOWN;
    }
}

void hw_audio_music_play(int mus_index)
{
    if (audio_initialized && opt_music_enabled && (mus_index < mus_num)) {
        /*if (Mix_PlayingMusic()) {
            Mix_HaltMusic();
        }TODO Tracks*/
        MIX_PlayAudio(mixer, mustbl[mus_index].music/*, mustbl[mus_index].loops ? -1 : 0*/);
        /*Mix_VolumeMusic(opt_music_volume);*/
        mus_playing = mus_index;
    }
}

void hw_audio_music_fadeout(void)
{
    /*if (audio_initialized && opt_music_enabled && Mix_PlayingMusic()) {
        Mix_FadeOutMusic(1000);
    }TODO tracks*/
}

void hw_audio_music_stop(void)
{
    /*if (audio_initialized && opt_music_enabled) {
        Mix_HaltMusic();
        mus_playing = -1;
    }TODO tracks*/
}

void hw_audio_music_volume(int volume)
{
    /*if (volume < 0) {
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
    }TODO tracks*/
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
        sfxtbl[sfx_index].chunk = MIX_LoadAudio_IO(mixer, SDL_IOFromMem(data, len), 1, 1);
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
            MIX_DestroyAudio(sfxtbl[sfx_index].chunk);
            sfxtbl[sfx_index].chunk = NULL;
        }
    }
}

void hw_audio_sfx_play(int sfx_index)
{
    if (audio_initialized && opt_sfx_enabled && (sfx_index < sfx_num)) {
        MIX_PlayAudio(mixer, sfxtbl[sfx_index].chunk);
        sfx_playing = sfx_index;
    }
}

void hw_audio_sfx_stop(void)
{
    /*if (audio_initialized && Mix_Playing(0)) {
        Mix_HaltChannel(0);
        sfx_playing = -1;
    }TODO tracks*/
}

void hw_audio_sfx_volume(int volume)
{
    /*if (volume < 0) {
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
    }TODO tracks */
}

#else /* !HAVE_SDL3MIXER */

int hw_audio_init(void)
{
    if (opt_audio_enabled) {
        log_warning("SDLA: no audio due to missing SDL_mixer\n");
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
#endif /* HAVE_SDL3MIXER */
