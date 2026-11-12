#include "config.h"

#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <mmsystem.h>

#include "hw_internal.h"

#include "fmt_mus.h"
#include "fmt_sfx.h"
#include "lib.h"
#include "options.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static bool audio_initialized = false;
static int audio_rate = 0;
static HWAVEOUT waveout = NULL;
static WAVEHDR wavehdr;
static uint8_t *wavebuf = NULL;
static uint32_t wavebuf_len = 0;
static bool wave_playing = false;
static bool midi_playing = false;
static bool midi_loops = false;
static bool midi_needs_restart = false;

struct sfx_s {
    uint8_t *data;
    uint32_t len;
};
static int sfx_num = 0;
static struct sfx_s *sfxtbl = NULL;
static int sfx_playing;

struct mus_s {
    mus_type_t type;
    char *path;
    bool loops;
};
static int mus_num = 0;
static struct mus_s *mustbl = NULL;
static int mus_playing;

#define HWWIN32_MUS_TEMPFILE "1tempmus.mid"

/* -------------------------------------------------------------------------- */

int hw_audio_init(void)
{
    if (opt_audio_enabled) {
        WAVEFORMATEX wf;
        memset(&wf, 0, sizeof(wf));
        wf.wFormatTag = WAVE_FORMAT_PCM;
        wf.nChannels = 2;
        wf.wBitsPerSample = 16;
        wf.nSamplesPerSec = (DWORD)opt_audiorate;
        wf.nBlockAlign = (WORD)(wf.nChannels * wf.wBitsPerSample / 8);
        wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
        if (waveOutOpen(&waveout, WAVE_MAPPER, &wf, (DWORD_PTR)NULL, (DWORD_PTR)0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
            log_error("initialising waveOut (%i Hz) failed!\n", opt_audiorate);
            return -1;
        }
        audio_rate = opt_audiorate;
        log_message("Audio: waveOut %i Hz, 16 bit, 2 ch\n", audio_rate);
        sfx_playing = -1;
        mus_playing = -1;
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

void hw_audio_update(void)
{
    if (audio_initialized && wave_playing) {
        /* recycle the buffer once playback has passed it */
        if (wavehdr.dwFlags & WHDR_DONE) {
            waveOutUnprepareHeader(waveout, &wavehdr, sizeof(wavehdr));
            wave_playing = false;
            sfx_playing = -1;
        }
    }

    /* manual MIDI looping: the MCI sequencer driver does not support "repeat" */
    if (audio_initialized && midi_playing) {
        MCI_STATUS_PARMS msp;
        memset(&msp, 0, sizeof(msp));
        msp.dwItem = MCI_STATUS_MODE;
        if (mciSendCommand(mciGetDeviceID("moo_midi"), MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR)&msp) == 0) {
            if (msp.dwReturn == MCI_MODE_NOT_READY) {
                midi_playing = false;
                mus_playing = -1;
            } else if ((msp.dwReturn == MCI_MODE_STOP) && !midi_needs_restart) {
                if (midi_loops) {
                    midi_needs_restart = true;
                } else {
                    midi_playing = false;
                    mus_playing = -1;
                }
            }
        }
        if (midi_needs_restart) {
            mciSendString("seek moo_midi to start", NULL, 0, NULL);
            if (mciSendString("play moo_midi", NULL, 0, NULL) == 0) {
                midi_needs_restart = false;
            }
        }
    }
}

void hw_audio_shutdown(void)
{
    if (audio_initialized) {
        log_message("Audio: shutdown\n");
        hw_audio_sfx_stop();
        waveOutClose(waveout);
        waveout = NULL;
        for (int i = 0; i < sfx_num; ++i) {
            hw_audio_sfx_release(i);
        }
        lib_free(sfxtbl);
        sfxtbl = NULL;
        for (int i = 0; i < mus_num; ++i) {
            hw_audio_music_release(i);
        }
        lib_free(mustbl);
        mustbl = NULL;
        audio_initialized = false;
    }
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
            mustbl[i].path = NULL;
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
            } else {
                m->type = MUS_TYPE_UNKNOWN;
            }
            break;
        case MUS_TYPE_MIDI:
            break;
        case MUS_TYPE_UNKNOWN:
            break;
        default:
            log_error("Audio: unsupported music type %i\n", m->type);
            m->type = MUS_TYPE_UNKNOWN;
            break;
    }

    if (m->type == MUS_TYPE_UNKNOWN) {
        log_error("Audio: failed to init music %i\n", mus_index);
        return -1;
    }

    /* MCI cannot read a MIDI file in memory. Use a temp file. */
    if (m->type == MUS_TYPE_LBXXMID) {
        if (util_file_save(HWWIN32_MUS_TEMPFILE, data, len)) {
            log_error("Audio: failed to create temp file\n");
            lib_free(buf);
            m->type = MUS_TYPE_UNKNOWN;
            return -1;
        }
        lib_free(buf);
        m->path = lib_stralloc(HWWIN32_MUS_TEMPFILE);
    } else {
        /* MUS_TYPE_MIDI data cannot be referenced in memory either; save it too */
        if (util_file_save(HWWIN32_MUS_TEMPFILE, data_in, len_in)) {
            log_error("Audio: failed to create temp file\n");
            m->type = MUS_TYPE_UNKNOWN;
            return -1;
        }
        m->path = lib_stralloc(HWWIN32_MUS_TEMPFILE);
    }
    return 0;
}

void hw_audio_music_release(int mus_index)
{
    if (mus_index < mus_num) {
        if (mus_playing == mus_index) {
            hw_audio_music_stop();
        }
        if (mustbl[mus_index].path) {
            DeleteFile(mustbl[mus_index].path);
            lib_free(mustbl[mus_index].path);
            mustbl[mus_index].path = NULL;
        }
        mustbl[mus_index].type = MUS_TYPE_UNKNOWN;
    }
}

void hw_audio_music_play(int mus_index)
{
    char cmd[256];
    char alias[] = "moo_midi";

    if (audio_initialized && opt_music_enabled && (mus_index < mus_num) && mustbl[mus_index].path) {
        hw_audio_music_stop();
        sprintf(cmd, "open \"%s\" type sequencer alias %s", mustbl[mus_index].path, alias);
        if (mciSendString(cmd, NULL, 0, NULL) != 0) {
            log_error("Audio: MCI open failed on music %i\n", mus_index);
            return;
        }
        sprintf(cmd, "play %s", alias);
        if (mciSendString(cmd, NULL, 0, NULL) != 0) {
            log_error("Audio: MCI play failed on music %i\n", mus_index);
        } else {
            midi_playing = true;
            midi_loops = mustbl[mus_index].loops;
            midi_needs_restart = false;
        }
        mus_playing = mus_index;
    }
}

void hw_audio_music_fadeout(void)
{
    /* TODO */
    hw_audio_music_stop();
}

void hw_audio_music_stop(void)
{
    char cmd[256];
    char alias[] = "moo_midi";

    if (audio_initialized && (mus_playing >= 0)) {
        sprintf(cmd, "stop %s", alias);
        mciSendString(cmd, NULL, 0, NULL);
        sprintf(cmd, "close %s", alias);
        mciSendString(cmd, NULL, 0, NULL);
        midi_playing = false;
        midi_loops = false;
        midi_needs_restart = false;
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
    if (audio_initialized) {
        midiOutSetVolume(NULL, (DWORD)((volume * 65535 / 128) | ((volume * 65535 / 128) << 16)));
    }
    if (opt_music_volume != volume) {
        log_message("Audio: music volume %i\n", volume);
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
            sfxtbl[i].data = NULL;
            sfxtbl[i].len = 0;
        }
    }

    if (sfxtbl[sfx_index].data) {
        hw_audio_sfx_release(sfx_index);
    }

    if (fmt_sfx_convert(data_in, len_in, &data, &len, NULL, audio_rate, false)) {
        sfxtbl[sfx_index].data = data;
        sfxtbl[sfx_index].len = len;
    } else {
        log_error("Audio: failed to init sound %i\n", sfx_index);
        return -1;
    }
    return 0;
}

void hw_audio_sfx_release(int sfx_index)
{
    if (sfx_index < sfx_num) {
        if (sfxtbl[sfx_index].data) {
            if (sfx_playing == sfx_index) {
                hw_audio_sfx_stop();
            }
            lib_free(sfxtbl[sfx_index].data);
            sfxtbl[sfx_index].data = NULL;
            sfxtbl[sfx_index].len = 0;
        }
    }
}

void hw_audio_sfx_play(int sfx_index)
{
    if (audio_initialized && opt_sfx_enabled && (sfx_index < sfx_num) && sfxtbl[sfx_index].data) {
        hw_audio_sfx_stop();
        if (!wavebuf || (wavebuf_len < sfxtbl[sfx_index].len)) {
            lib_free(wavebuf);
            wavebuf_len = sfxtbl[sfx_index].len;
            wavebuf = lib_malloc(wavebuf_len);
        }
        if (!wavebuf) {
            wavebuf_len = 0;
            return;
        }
        memcpy(wavebuf, sfxtbl[sfx_index].data, sfxtbl[sfx_index].len);
        memset(&wavehdr, 0, sizeof(wavehdr));
        wavehdr.lpData = (LPSTR)wavebuf;
        wavehdr.dwBufferLength = sfxtbl[sfx_index].len;
        if (waveOutPrepareHeader(waveout, &wavehdr, sizeof(wavehdr)) != MMSYSERR_NOERROR) {
            log_error("Audio: waveOutPrepareHeader failed\n");
            return;
        }
        if (waveOutWrite(waveout, &wavehdr, sizeof(wavehdr)) != MMSYSERR_NOERROR) {
            log_error("Audio: waveOutWrite failed\n");
            waveOutUnprepareHeader(waveout, &wavehdr, sizeof(wavehdr));
            return;
        }
        wave_playing = true;
        sfx_playing = sfx_index;
    }
}

void hw_audio_sfx_stop(void)
{
    if (audio_initialized && wave_playing) {
        waveOutReset(waveout);
        waveOutUnprepareHeader(waveout, &wavehdr, sizeof(wavehdr));
        wave_playing = false;
        sfx_playing = -1;
    }
}

void hw_audio_sfx_volume(int volume)
{
    DWORD vol;

    if (volume < 0) {
        volume = 0;
    }
    if (volume > 128) {
        volume = 128;
    }
    if (audio_initialized) {
        vol = (DWORD)(volume * 65535 / 128);
        waveOutSetVolume(waveout, vol | (vol << 16));
    }
    if (opt_sfx_volume != volume) {
        log_message("Audio: sfx volume %i\n", volume);
        opt_sfx_volume = volume;
    }
}

int hw_audio_set_soundfont(const char *path)
{
    log_message("Audio: hw_audio_set_soundfont not implemented\n");
    return 0;
}
