#ifndef INC_1OOM_XMIDI_H
#define INC_1OOM_XMIDI_H

#include "types.h"

typedef bool (*xmidi_mt32_timbre_lookup_t)(void *ctx, uint8_t bank, uint8_t patch,
                                           const uint8_t **data, uint32_t *len);

extern void xmidi_mt32_set_timbre_lookup(xmidi_mt32_timbre_lookup_t lookup, void *ctx);
extern bool xmidi_convert_to_smf(const uint8_t *data_in, uint32_t len_in, uint8_t **data_out, uint32_t *len_out, bool *tune_loops);
extern bool xmidi_render_mt32_wav(const uint8_t *data_in, uint32_t len_in, uint32_t sample_rate,
                                  const char *rom_control, const char *rom_pcm,
                                  uint8_t **wav_out, uint32_t *wav_len, bool *tune_loops);

#endif
