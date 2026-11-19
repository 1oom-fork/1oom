#include "config.h"

#ifdef HAVE_SDL3MIXER
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "fmt_mus.h"
#include "fmt_sfx.h"
#include "lib.h"
#endif

#include "hw_internal.h"

#include "hwsdl3_opt.h"
#include "options.h"

#ifdef HAVE_SDL3MIXER

#define SOUNDFONT_PROP "SDL_mixer.decoder.fluidsynth.soundfont_path"

static bool audio_initialized = false;
static int audio_rate = 0;
static MIX_Mixer *mixer = NULL;
static MIX_Track *music_track = NULL;
static MIX_Track *sfx_track = NULL;
static char *soundfont_path = NULL;

struct sfx_s {
    MIX_Audio *audio;
};
static int sfx_num = 0;
static struct sfx_s *sfxtbl = NULL;
static int sfx_playing;

struct mus_s {
    mus_type_t type;
    MIX_Audio *audio;
    bool loops;
};
static int mus_num = 0;
static struct mus_s *mustbl = NULL;
static int mus_playing;

static float volume_to_gain(int volume)
{
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 128) {
        volume = 128;
    }
    return (float)volume / 128.0f;
}

static MIX_Audio *audio_load(const uint8_t *data, uint32_t len, bool is_midi)
{
    SDL_IOStream *io;
    MIX_Audio *audio;

    io = SDL_IOFromConstMem(data, (size_t)len);
    if (!io) {
        log_error("SDLA: SDL_IOFromConstMem failed: %s\n", SDL_GetError());
        return NULL;
    }
    if (is_midi && soundfont_path) {
        SDL_PropertiesID props = SDL_CreateProperties();
        if (props == 0) {
            log_error("SDLA: SDL_CreateProperties failed: %s\n", SDL_GetError());
            SDL_CloseIO(io);
            return NULL;
        }
        if (!SDL_SetPointerProperty(props, MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER, io)
          || !SDL_SetPointerProperty(props, MIX_PROP_AUDIO_LOAD_PREFERRED_MIXER_POINTER, mixer)
          || !SDL_SetBooleanProperty(props, MIX_PROP_AUDIO_LOAD_PREDECODE_BOOLEAN, false)
          || !SDL_SetStringProperty(props, SOUNDFONT_PROP, soundfont_path)) {
            log_error("SDLA: failed to set audio load properties: %s\n", SDL_GetError());
            SDL_CloseIO(io);
            SDL_DestroyProperties(props);
            return NULL;
        }
        if (!SDL_SetBooleanProperty(props, MIX_PROP_AUDIO_LOAD_CLOSEIO_BOOLEAN, true)) {
            log_error("SDLA: failed to set audio load properties: %s\n", SDL_GetError());
            SDL_CloseIO(io);
            SDL_DestroyProperties(props);
            return NULL;
        }
        audio = MIX_LoadAudioWithProperties(props);
        SDL_DestroyProperties(props);
    } else {
        audio = MIX_LoadAudio_IO(mixer, io, false, true);
    }
    if (!audio) {
        log_error("SDLA: MIX_LoadAudio failed: %s\n", SDL_GetError());
        return NULL;
    }
    return audio;
}

int hw_audio_init(void)
{
    SDL_AudioSpec spec;
    int volume;

    if (!opt_audio_enabled) {
        return 0;
    }
    if (!MIX_Init()) {
        log_error("SDLA: MIX_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    spec.freq = opt_audiorate;
    mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!mixer) {
        log_error("SDLA: MIX_CreateMixerDevice failed: %s\n", SDL_GetError());
        MIX_Quit();
        return -1;
    }
    if (!MIX_GetMixerFormat(mixer, &spec)) {
        log_error("SDLA: MIX_GetMixerFormat failed: %s\n", SDL_GetError());
        MIX_DestroyMixer(mixer);
        mixer = NULL;
        MIX_Quit();
        return -1;
    }
    audio_rate = spec.freq;
    if (spec.channels != 2) {
        log_warning("SDL_mixer gave %i output channels instead of 2\n", spec.channels);
    }
    if (audio_rate != opt_audiorate) {
        log_warning("SDL_mixer gave %i Hz instead of %i Hz\n", audio_rate, opt_audiorate);
    }
    music_track = MIX_CreateTrack(mixer);
    sfx_track = MIX_CreateTrack(mixer);
    if (!music_track || !sfx_track) {
        log_error("SDLA: MIX_CreateTrack failed: %s\n", SDL_GetError());
        MIX_DestroyMixer(mixer);
        mixer = NULL;
        MIX_Quit();
        return -1;
    }
    sfx_playing = -1;
    mus_playing = -1;
    log_message("SDLA: init %i Hz format %s\n", audio_rate, SDL_GetAudioFormatName(spec.format));
    audio_initialized = true;
    volume = opt_sfx_volume;
    opt_sfx_volume = -1;
    hw_audio_sfx_volume(volume);
    volume = opt_music_volume;
    opt_music_volume = -1;
    hw_audio_music_volume(volume);
    return 0;
}

void hw_audio_shutdown(void)
{
    if (!audio_initialized) {
        return;
    }
    log_message("SDLA: shutdown\n");
    for (int i = 0; i < mus_num; ++i) {
        hw_audio_music_release(i);
    }
    for (int i = 0; i < sfx_num; ++i) {
        hw_audio_sfx_release(i);
    }
    lib_free(mustbl);
    lib_free(sfxtbl);
    mustbl = NULL;
    sfxtbl = NULL;
    mus_num = 0;
    sfx_num = 0;
    lib_free(soundfont_path);
    soundfont_path = NULL;
    MIX_DestroyMixer(mixer);
    mixer = NULL;
    music_track = NULL;
    sfx_track = NULL;
    MIX_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audio_initialized = false;
}

int hw_audio_set_soundfont(const char *path)
{
    log_message("SDLA: setting soundfont to '%s'\n", path);
    lib_free(soundfont_path);
    soundfont_path = path ? lib_stralloc(path) : NULL;
    return 0;
}

int hw_audio_music_init(int mus_index, const uint8_t *data_in, uint32_t len_in)
{
    const uint8_t *data = NULL;
    uint8_t *buf = NULL;
    uint32_t len = 0;
    bool is_midi = false;
    struct mus_s *m;

    if (!audio_initialized) {
        return 0;
    }
    if (mus_index >= mus_num) {
        int old_mus_num = mus_num;
        mus_num = mus_index + 1;
        mustbl = lib_realloc(mustbl, (size_t)mus_num * sizeof(struct mus_s));
        for (int i = old_mus_num; i < mus_num; ++i) {
            mustbl[i].type = MUS_TYPE_UNKNOWN;
            mustbl[i].audio = NULL;
            mustbl[i].loops = false;
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
            is_midi = true;
        } else {
            m->type = MUS_TYPE_UNKNOWN;
        }
        break;
    case MUS_TYPE_WAV:
        if (fmt_sfx_convert(data_in, len_in, &buf, &len, NULL, (uint32_t)audio_rate, true)) {
            data = buf;
        } else {
            m->type = MUS_TYPE_UNKNOWN;
        }
        m->loops = false;
        break;
    case MUS_TYPE_UNKNOWN:
        break;
    default:
        data = data_in;
        len = len_in;
        m->loops = false;
        is_midi = (m->type == MUS_TYPE_MIDI);
        break;
    }
    if (m->type == MUS_TYPE_UNKNOWN) {
        log_error("SDLA: failed to init music %i\n", mus_index);
        return -1;
    }
    m->audio = audio_load(data, len, is_midi);
    lib_free(buf);
    if (!m->audio) {
        m->type = MUS_TYPE_UNKNOWN;
        return -1;
    }
    return 0;
}

void hw_audio_music_release(int mus_index)
{
    if (mus_index >= mus_num) {
        return;
    }
    if (mus_playing == mus_index) {
        hw_audio_music_stop();
    }
    if (mustbl[mus_index].audio) {
        MIX_DestroyAudio(mustbl[mus_index].audio);
        mustbl[mus_index].audio = NULL;
    }
    mustbl[mus_index].type = MUS_TYPE_UNKNOWN;
}

void hw_audio_music_play(int mus_index)
{
    SDL_PropertiesID props;

    if (!audio_initialized || !opt_music_enabled || (mus_index >= mus_num)
      || !mustbl[mus_index].audio) {
        return;
    }
    if (MIX_TrackPlaying(music_track)) {
        MIX_StopTrack(music_track, 0);
    }
    if (!MIX_SetTrackAudio(music_track, mustbl[mus_index].audio)) {
        log_error("SDLA: MIX_SetTrackAudio failed: %s\n", SDL_GetError());
        return;
    }
    props = SDL_CreateProperties();
    if (props == 0) {
        log_warning("SDLA: SDL_CreateProperties failed: %s\n", SDL_GetError());
    } else if (!SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER,
                                      mustbl[mus_index].loops ? -1 : 0)) {
        log_warning("SDLA: SDL_SetNumberProperty failed: %s\n", SDL_GetError());
        SDL_DestroyProperties(props);
        props = 0;
    }
    if (!MIX_PlayTrack(music_track, props)) {
        log_error("SDLA: MIX_PlayTrack failed: %s\n", SDL_GetError());
    } else {
        mus_playing = mus_index;
    }
    if (props) {
        SDL_DestroyProperties(props);
    }
    MIX_SetTrackGain(music_track, volume_to_gain(opt_music_volume));
}

void hw_audio_music_fadeout(void)
{
    if (audio_initialized && opt_music_enabled && MIX_TrackPlaying(music_track)) {
        Sint64 frames = MIX_TrackMSToFrames(music_track, 1000);
        MIX_StopTrack(music_track, frames);
    }
}

void hw_audio_music_stop(void)
{
    if (audio_initialized && opt_music_enabled && MIX_TrackPlaying(music_track)) {
        MIX_StopTrack(music_track, 0);
    }
    mus_playing = -1;
}

void hw_audio_music_volume(int volume)
{
    volume = (volume < 0) ? 0 : ((volume > 128) ? 128 : volume);
    if (audio_initialized && opt_music_enabled) {
        MIX_SetTrackGain(music_track, volume_to_gain(volume));
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
        sfx_num = sfx_index + 1;
        sfxtbl = lib_realloc(sfxtbl, (size_t)sfx_num * sizeof(struct sfx_s));
        for (int i = old_sfx_num; i < sfx_num; ++i) {
            sfxtbl[i].audio = NULL;
        }
    }
    if (sfxtbl[sfx_index].audio) {
        hw_audio_sfx_release(sfx_index);
    }
    if (!fmt_sfx_convert(data_in, len_in, &data, &len, NULL, (uint32_t)audio_rate, true)) {
        log_error("SDLA: failed to init sound %i\n", sfx_index);
        return -1;
    }
    sfxtbl[sfx_index].audio = audio_load(data, len, false);
    lib_free(data);
    return sfxtbl[sfx_index].audio ? 0 : -1;
}

void hw_audio_sfx_release(int sfx_index)
{
    if (sfx_index >= sfx_num) {
        return;
    }
    if (sfx_playing == sfx_index) {
        hw_audio_sfx_stop();
    }
    if (sfxtbl[sfx_index].audio) {
        MIX_DestroyAudio(sfxtbl[sfx_index].audio);
        sfxtbl[sfx_index].audio = NULL;
    }
}

void hw_audio_sfx_play(int sfx_index)
{
    if (!audio_initialized || !opt_sfx_enabled || (sfx_index >= sfx_num)
      || !sfxtbl[sfx_index].audio) {
        return;
    }
    if (MIX_TrackPlaying(sfx_track)) {
        MIX_StopTrack(sfx_track, 0);
    }
    if (!MIX_SetTrackAudio(sfx_track, sfxtbl[sfx_index].audio)) {
        log_error("SDLA: MIX_SetTrackAudio failed: %s\n", SDL_GetError());
        return;
    }
    if (!MIX_PlayTrack(sfx_track, 0)) {
        log_error("SDLA: MIX_PlayTrack failed: %s\n", SDL_GetError());
        return;
    }
    MIX_SetTrackGain(sfx_track, volume_to_gain(opt_sfx_volume));
    sfx_playing = sfx_index;
}

void hw_audio_sfx_stop(void)
{
    if (audio_initialized && opt_sfx_enabled && MIX_TrackPlaying(sfx_track)) {
        MIX_StopTrack(sfx_track, 0);
    }
    sfx_playing = -1;
}

void hw_audio_sfx_volume(int volume)
{
    volume = (volume < 0) ? 0 : ((volume > 128) ? 128 : volume);
    if (audio_initialized && opt_sfx_enabled) {
        MIX_SetTrackGain(sfx_track, volume_to_gain(volume));
    }
    if (opt_sfx_volume != volume) {
        log_message("SDLA: sfx volume %i\n", volume);
        opt_sfx_volume = volume;
    }
}

#else

int hw_audio_init(void)
{
    if (opt_audio_enabled) {
        log_warning("SDLA: no audio due to missing SDL3_mixer\n");
    }
    return 0;
}

void hw_audio_shutdown(void)
{
}

int hw_audio_set_soundfont(const char *path)
{
    (void)path;
    return 0;
}

int hw_audio_music_init(int mus_index, const uint8_t *data, uint32_t len)
{
    (void)mus_index;
    (void)data;
    (void)len;
    return 0;
}

void hw_audio_music_release(int mus_index)
{
    (void)mus_index;
}

void hw_audio_music_play(int mus_index)
{
    (void)mus_index;
}

void hw_audio_music_fadeout(void)
{
}

void hw_audio_music_stop(void)
{
}

void hw_audio_music_volume(int volume)
{
    (void)volume;
}

int hw_audio_sfx_init(int sfx_index, const uint8_t *data, uint32_t len)
{
    (void)sfx_index;
    (void)data;
    (void)len;
    return 0;
}

void hw_audio_sfx_release(int sfx_index)
{
    (void)sfx_index;
}

void hw_audio_sfx_play(int sfx_index)
{
    (void)sfx_index;
}

void hw_audio_sfx_stop(void)
{
}

void hw_audio_sfx_volume(int volume)
{
    (void)volume;
}

#endif
