#include "config.h"

#include <string.h>

#include "fmt_mus.h"
#include "bits.h"
#include "fmt_id.h"
#include "lib.h"
#include "log.h"
#include "options.h"
#include "types.h"
#include "xmidi.h"

/* -------------------------------------------------------------------------- */

mus_type_t fmt_mus_detect(const uint8_t *data, uint32_t len)
{
    uint32_t hdrid;
    if (len < 32) {
        return MUS_TYPE_UNKNOWN;
    }
    hdrid = GET_BE_32(data);
    if (hdrid == HDRID_LBXXMID) {
        return MUS_TYPE_LBXXMID;
    } else if (hdrid == HDRID_MIDI) {
        return MUS_TYPE_MIDI;
    } else if (hdrid == HDRID_WAV) {
        return MUS_TYPE_WAV;
    } else if (hdrid == HDRID_OGG) {
        return MUS_TYPE_OGG;
    } else if (hdrid == HDRID_FLAC) {
        return MUS_TYPE_FLAC;
    }
    return MUS_TYPE_UNKNOWN;
}

bool fmt_mus_convert_xmid(const uint8_t *data_in, uint32_t len_in, uint8_t **data_out_ptr, uint32_t *len_out_ptr, bool *tune_loops)
{
    return xmidi_convert_to_smf(data_in, len_in, data_out_ptr, len_out_ptr, tune_loops);
}

bool fmt_mus_render_xmid_mt32(const uint8_t *data_in, uint32_t len_in, uint32_t sample_rate, uint8_t **wav_out, uint32_t *wav_len, bool *tune_loops)
{
    return xmidi_render_mt32_wav(data_in, len_in, sample_rate, opt_mt32_rom1, opt_mt32_rom2, wav_out, wav_len, tune_loops);
}
