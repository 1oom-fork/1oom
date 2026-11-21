#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "bits.h"
#include "fmt_id.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "types.h"
#include "util.h"
#include "xmidi.h"

#if defined(HAVE_MT32EMU)
#if defined(HAVE_MT32EMU_C_INTERFACE_C_INTERFACE_H)
#include <mt32emu/c_interface/c_interface.h>
#elif defined(HAVE_MT32EMU_C_INTERFACE_H)
#include <mt32emu/c_interface.h>
#else
#include <c_interface.h>
#endif
#endif

#define XMIDI_PPQN 60
#define XMIDI_US_PER_TICK 8333
#define XMIDI_FOR_NEST 4
#define XMIDI_NOTE_SLOTS 64

typedef struct {
    uint8_t *data;
    uint32_t len;
    uint64_t tick;
} xmidi_mevent_t;

typedef struct {
    xmidi_mevent_t *v;
    uint32_t n;
    uint32_t cap;
} xmidi_mevents_t;

typedef struct {
    uint64_t tick;
    uint8_t status;
    uint8_t note;
    bool used;
} xmidi_noteoff_t;

typedef struct {
    const uint8_t *evnt;
    uint32_t evnt_len;
    uint16_t timb_num;
    const uint8_t *timb_tbl; /* patch,bank pairs */
} xmidi_form_t;

static bool mevents_push(xmidi_mevents_t *m, uint64_t tick, const uint8_t *data, uint32_t len)
{
    xmidi_mevent_t *e;
    if (m->n == m->cap) {
        uint32_t ncap = m->cap ? m->cap * 2 : 256;
        xmidi_mevent_t *nv = (xmidi_mevent_t *)realloc(m->v, ncap * sizeof(*nv));
        if (!nv) {
            return false;
        }
        m->v = nv;
        m->cap = ncap;
    }
    e = &m->v[m->n++];
    e->tick = tick;
    e->len = len;
    e->data = (uint8_t *)lib_malloc(len);
    if (!e->data) {
        --m->n;
        return false;
    }
    memcpy(e->data, data, len);
    return true;
}

static void mevents_free(xmidi_mevents_t *m)
{
    uint32_t i;
    for (i = 0; i < m->n; ++i) {
        lib_free(m->v[i].data);
    }
    free(m->v);
    m->v = NULL;
    m->n = 0;
    m->cap = 0;
}

static uint32_t xmidi_vlq_read(const uint8_t *p, uint32_t len, uint32_t *pos)
{
    uint32_t v = 0;
    while (*pos < len) {
        uint8_t b = p[(*pos)++];
        v = (v << 7) | (uint32_t)(b & 0x7f);
        if ((b & 0x80) == 0) {
            break;
        }
    }
    return v;
}

static int xmidi_find_chunk(const uint8_t *data, uint32_t len, const char id[4], const uint8_t **out, uint32_t *out_len)
{
    uint32_t pos = 0;
    /* FORM XMID inside: at 0x10 "FORM", 0x3a "XMID", chunks from 0x3e in typical MOO1 */
    if (len >= 0x3e && memcmp(data + 0x10, "FORM", 4) == 0 && memcmp(data + 0x3a, "XMID", 4) == 0) {
        pos = 0x3e;
    } else {
        /* fallback scan */
        for (pos = 0; pos + 8 <= len; ++pos) {
            if (memcmp(data + pos, "XMID", 4) == 0) {
                pos += 4;
                break;
            }
        }
    }
    while (pos + 8 <= len) {
        uint32_t clen = GET_BE_32(data + pos + 4);
        uint32_t body = pos + 8;
        if (body > len || clen > len - body) {
            break;
        }
        if (memcmp(data + pos, id, 4) == 0) {
            *out = data + body;
            *out_len = clen;
            return 1;
        }
        pos = body + clen + (clen & 1u);
    }
    return 0;
}

static bool xmidi_parse(const uint8_t *data, uint32_t len, xmidi_form_t *form)
{
    const uint8_t *timb = NULL, *evnt = NULL;
    uint32_t timb_len = 0, evnt_len = 0;
    memset(form, 0, sizeof(*form));
    if (len < 0x20 || GET_BE_32(data) != HDRID_LBXXMID) {
        return false;
    }
    if (!xmidi_find_chunk(data, len, "EVNT", &evnt, &evnt_len)) {
        return false;
    }
    form->evnt = evnt;
    form->evnt_len = evnt_len;
    if (xmidi_find_chunk(data, len, "TIMB", &timb, &timb_len) && timb_len >= 2) {
        form->timb_num = GET_LE_16(timb);
        form->timb_tbl = timb + 2;
        if ((uint32_t)form->timb_num * 2 > timb_len - 2) {
            form->timb_num = (uint16_t)((timb_len - 2) / 2);
        }
    }
    return true;
}

static bool timbre_bank(const xmidi_form_t *form, uint8_t patch, uint8_t *bank)
{
    uint16_t i;
    for (i = 0; i < form->timb_num; ++i) {
        if (form->timb_tbl[i * 2] == patch) {
            *bank = form->timb_tbl[i * 2 + 1];
            return true;
        }
    }
    return false;
}

static void noteoff_emit(xmidi_mevents_t *out, xmidi_noteoff_t *n)
{
    uint8_t msg[3];
    msg[0] = n->status;
    msg[1] = n->note;
    msg[2] = 0;
    mevents_push(out, n->tick, msg, 3);
    n->used = false;
}

static void flush_due_noteoffs(xmidi_mevents_t *out, xmidi_noteoff_t *tbl, uint64_t now_tick)
{
    int i;
    for (;;) {
        int best = -1;
        for (i = 0; i < XMIDI_NOTE_SLOTS; ++i) {
            if (tbl[i].used && tbl[i].tick <= now_tick) {
                if (best < 0 || tbl[i].tick < tbl[best].tick) {
                    best = i;
                }
            }
        }
        if (best < 0) {
            break;
        }
        noteoff_emit(out, &tbl[best]);
    }
}

static void flush_all_noteoffs(xmidi_mevents_t *out, xmidi_noteoff_t *tbl, uint64_t now_tick)
{
    int i;
    for (;;) {
        int best = -1;
        for (i = 0; i < XMIDI_NOTE_SLOTS; ++i) {
            if (tbl[i].used) {
                if (best < 0 || tbl[i].tick < tbl[best].tick) {
                    best = i;
                }
            }
        }
        if (best < 0) {
            break;
        }
        if (tbl[best].tick > now_tick) {
            tbl[best].tick = now_tick;
        }
        noteoff_emit(out, &tbl[best]);
    }
}

static void schedule_noteoff(xmidi_noteoff_t *tbl, uint8_t ch, uint8_t note, uint64_t tick)
{
    int i, slot = -1;
    for (i = 0; i < XMIDI_NOTE_SLOTS; ++i) {
        if (!tbl[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        slot = 0;
        for (i = 1; i < XMIDI_NOTE_SLOTS; ++i) {
            if (tbl[i].tick < tbl[slot].tick) slot = i;
        }
    }
    tbl[slot].used = true;
    tbl[slot].status = (uint8_t)(0x80 | (ch & 0x0f));
    tbl[slot].note = note;
    tbl[slot].tick = tick;
}

static bool xmidi_sequence(const uint8_t *data, uint32_t len, xmidi_mevents_t *out, bool *tune_loops)
{
    xmidi_form_t form;
    xmidi_noteoff_t notes[XMIDI_NOTE_SLOTS];
    uint32_t pos = 0;
    uint64_t tick = 0;
    uint32_t interval = 0;
    uint32_t loop_count[XMIDI_FOR_NEST];
    uint32_t loop_pos[XMIDI_FOR_NEST];
    int loop_sp = -1;
    uint8_t chan_bank[16];
    bool end = false;
    bool infinite = false;

    memset(notes, 0, sizeof(notes));
    memset(chan_bank, 0, sizeof(chan_bank));
    for (int i = 0; i < XMIDI_FOR_NEST; ++i) { loop_count[i] = 0; loop_pos[i] = 0; }

    if (!xmidi_parse(data, len, &form)) {
        log_error("XMID: invalid/corrupt data\n");
        return false;
    }

    while (pos < form.evnt_len && !end) {
        uint8_t st = form.evnt[pos++];
        if (st < 0x80) {
            interval += st;
            continue;
        }
        tick += interval;
        interval = 0;
        flush_due_noteoffs(out, notes, tick);

        if (st >= 0xf0) {
            if (st == 0xff) {
                uint8_t type;
                uint32_t mlen;
                if (pos >= form.evnt_len) break;
                type = form.evnt[pos++];
                mlen = xmidi_vlq_read(form.evnt, form.evnt_len, &pos);
                if (pos + mlen > form.evnt_len) break;
                if (type == 0x2f) {
                    uint8_t msg[3] = { 0xff, 0x2f, 0x00 };
                    flush_all_noteoffs(out, notes, tick);
                    mevents_push(out, tick, msg, 3);
                    end = true;
                } else if (type == 0x51 && mlen == 3) {
                    /* XMIDI timing uses fixed 120Hz quantums; tempo meta is ignored for playback speed. */
                } else if (type == 0x58 && mlen >= 2) {
                    uint8_t msg[7] = { 0xff, 0x58, 0x04, form.evnt[pos], form.evnt[pos + 1], (mlen > 2 ? form.evnt[pos + 2] : 24), (mlen > 3 ? form.evnt[pos + 3] : 8) };
                    mevents_push(out, tick, msg, 7);
                }
                pos += mlen;
            } else if (st == 0xf0 || st == 0xf7) {
                uint32_t slen = xmidi_vlq_read(form.evnt, form.evnt_len, &pos);
                if (pos + slen > form.evnt_len) break;
                /* ignore sysex in converted stream for now */
                pos += slen;
            } else {
                uint8_t msg[1] = { st };
                mevents_push(out, tick, msg, 1);
            }
            continue;
        }

        switch (st & 0xf0) {
        case 0x90: {
            uint8_t ch = st & 0x0f;
            uint8_t note, vel;
            uint32_t dur;
            if (pos + 1 >= form.evnt_len) { end = true; break; }
            note = form.evnt[pos++];
            vel = form.evnt[pos++];
            dur = xmidi_vlq_read(form.evnt, form.evnt_len, &pos);
            {
                uint8_t msg[3] = { st, note, vel };
                mevents_push(out, tick, msg, 3);
                if (vel != 0) {
                    schedule_noteoff(notes, ch, note, tick + dur);
                }
            }
            break;
        }
        case 0x80:
        case 0xa0:
        case 0xe0: {
            uint8_t d1, d2, msg[3];
            if (pos + 1 >= form.evnt_len) { end = true; break; }
            d1 = form.evnt[pos++]; d2 = form.evnt[pos++];
            msg[0] = st; msg[1] = d1; msg[2] = d2;
            mevents_push(out, tick, msg, 3);
            break;
        }
        case 0xb0: {
            uint8_t ch = st & 0x0f;
            uint8_t cc, val;
            if (pos + 1 >= form.evnt_len) { end = true; break; }
            cc = form.evnt[pos++]; val = form.evnt[pos++];
            if (cc == 114) { /* PATCH_BANK_SEL */
                chan_bank[ch] = val;
            } else if (cc == 116) { /* FOR_LOOP */
                if (loop_sp + 1 < XMIDI_FOR_NEST) {
                    ++loop_sp;
                    loop_count[loop_sp] = val;
                    loop_pos[loop_sp] = pos;
                    if (val == 0) infinite = true;
                }
            } else if (cc == 117) { /* NEXT_LOOP */
                if (loop_sp >= 0) {
                    if (val < 64) {
                        /* BREAK */
                        --loop_sp;
                    } else if (loop_count[loop_sp] == 0) {
                        /* infinite: terminate for offline output */
                        --loop_sp;
                        infinite = true;
                        {
                            uint8_t msg[3] = { 0xff, 0x2f, 0x00 };
                            flush_all_noteoffs(out, notes, tick);
                            mevents_push(out, tick, msg, 3);
                            end = true;
                        }
                    } else {
                        if (--loop_count[loop_sp] != 0) {
                            pos = loop_pos[loop_sp];
                        } else {
                            --loop_sp;
                        }
                    }
                }
            } else if (cc == 110 || cc == 111 || cc == 112 || cc == 113 || cc == 115 || cc == 118 || cc == 119 || cc == 120) {
                /* internal AIL controllers: ignore in SMF */
            } else {
                uint8_t msg[3] = { st, cc, val };
                mevents_push(out, tick, msg, 3);
            }
            break;
        }
        case 0xc0: {
            uint8_t ch = st & 0x0f;
            uint8_t patch, bank = 0;
            if (pos >= form.evnt_len) { end = true; break; }
            patch = form.evnt[pos++];
            if (chan_bank[ch] != 0) {
                bank = chan_bank[ch];
            } else {
                timbre_bank(&form, patch, &bank);
            }
            if (bank != 0) {
                uint8_t bsel[3] = { (uint8_t)(0xb0 | ch), 0x00, bank };
                mevents_push(out, tick, bsel, 3);
            }
            {
                uint8_t msg[2] = { st, patch };
                mevents_push(out, tick, msg, 2);
            }
            break;
        }
        case 0xd0: {
            uint8_t d1, msg[2];
            if (pos >= form.evnt_len) { end = true; break; }
            d1 = form.evnt[pos++];
            msg[0] = st; msg[1] = d1;
            mevents_push(out, tick, msg, 2);
            break;
        }
        default:
            end = true;
            break;
        }
    }

    if (!end) {
        uint8_t msg[3] = { 0xff, 0x2f, 0x00 };
        flush_all_noteoffs(out, notes, tick);
        mevents_push(out, tick, msg, 3);
    }

    if (tune_loops) {
        *tune_loops = infinite;
    }
    return true;
}

static int ev_cmp(const void *a, const void *b)
{
    const xmidi_mevent_t *ea = (const xmidi_mevent_t *)a;
    const xmidi_mevent_t *eb = (const xmidi_mevent_t *)b;
    if (ea->tick < eb->tick) return -1;
    if (ea->tick > eb->tick) return 1;
    /* note off before note on at same tick */
    {
        int aoff = (ea->len >= 1 && ((ea->data[0] & 0xf0) == 0x80 || (ea->data[0] == 0x90 && ea->len >= 3 && ea->data[2] == 0)));
        int boff = (eb->len >= 1 && ((eb->data[0] & 0xf0) == 0x80 || (eb->data[0] == 0x90 && eb->len >= 3 && eb->data[2] == 0)));
        if (aoff != boff) return aoff ? -1 : 1;
    }
    return 0;
}

static uint32_t vlq_write(uint8_t *buf, uint32_t value)
{
    uint8_t tmp[5];
    int n = 0;
    tmp[n++] = (uint8_t)(value & 0x7f);
    while ((value >>= 7) != 0) {
        tmp[n++] = (uint8_t)((value & 0x7f) | 0x80);
    }
    for (int i = 0; i < n; ++i) {
        buf[i] = tmp[n - 1 - i];
    }
    return (uint32_t)n;
}

bool xmidi_convert_to_smf(const uint8_t *data_in, uint32_t len_in, uint8_t **data_out, uint32_t *len_out, bool *tune_loops)
{
    xmidi_mevents_t evs = { 0 };
    uint8_t *smf = NULL;
    uint32_t cap, pos, i;
    uint64_t last = 0;

    if (data_out) *data_out = NULL;
    if (len_out) *len_out = 0;
    if (tune_loops) *tune_loops = false;

    if (!xmidi_sequence(data_in, len_in, &evs, tune_loops)) {
        mevents_free(&evs);
        return false;
    }
    qsort(evs.v, evs.n, sizeof(evs.v[0]), ev_cmp);

    cap = 22 + evs.n * 16 + 32;
    smf = (uint8_t *)lib_malloc(cap);
    if (!smf) {
        mevents_free(&evs);
        return false;
    }
    memcpy(smf, "MThd", 4);
    SET_BE_32(smf + 4, 6);
    SET_BE_16(smf + 8, 0);
    SET_BE_16(smf + 10, 1);
    SET_BE_16(smf + 12, XMIDI_PPQN);
    memcpy(smf + 14, "MTrk", 4);
    SET_BE_32(smf + 18, 0);
    pos = 22;

    /* tempo 120bpm for 60 PPQN => 500000 us/qn; XMIDI timing is in quantums, map 1 quantum = 1 tick */
    {
        uint8_t tempo[7] = { 0x00, 0xff, 0x51, 0x03, 0x07, 0xa1, 0x20 };
        memcpy(smf + pos, tempo, 7);
        pos += 7;
    }

    for (i = 0; i < evs.n; ++i) {
        uint64_t dt = evs.v[i].tick >= last ? evs.v[i].tick - last : 0;
        uint8_t vb[8];
        uint32_t vbl;
        if (pos + 16 + evs.v[i].len > cap) {
            while (pos + 16 + evs.v[i].len > cap) cap *= 2;
            smf = (uint8_t *)lib_realloc(smf, cap);
        }
        vbl = vlq_write(vb, (uint32_t)dt);
        memcpy(smf + pos, vb, vbl);
        pos += vbl;
        memcpy(smf + pos, evs.v[i].data, evs.v[i].len);
        pos += evs.v[i].len;
        last = evs.v[i].tick;
    }

    if (evs.n == 0 || !(evs.v[evs.n - 1].len == 3 && evs.v[evs.n - 1].data[0] == 0xff && evs.v[evs.n - 1].data[1] == 0x2f)) {
        uint8_t eot[4] = { 0x00, 0xff, 0x2f, 0x00 };
        if (pos + 4 > cap) { cap += 4; smf = (uint8_t *)lib_realloc(smf, cap); }
        memcpy(smf + pos, eot, 4);
        pos += 4;
    }

    SET_BE_32(smf + 18, pos - 22);
    smf = (uint8_t *)lib_realloc(smf, pos);
    if (data_out) *data_out = smf; else lib_free(smf);
    if (len_out) *len_out = pos;
    mevents_free(&evs);
    return true;
}

#if !defined(HAVE_MT32EMU)

bool xmidi_render_mt32_wav(const uint8_t *data_in, uint32_t len_in, uint32_t sample_rate,
                           const char *rom_control, const char *rom_pcm,
                           uint8_t **wav_out, uint32_t *wav_len, bool *tune_loops)
{
    (void)data_in; (void)len_in; (void)sample_rate; (void)rom_control; (void)rom_pcm;
    if (wav_out) *wav_out = NULL;
    if (wav_len) *wav_len = 0;
    if (tune_loops) *tune_loops = false;
    return false;
}

#else

static void wav_put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void wav_put_u32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }

static bool xmidi_mt32_have_roms(mt32emu_context ctx)
{
    mt32emu_rom_info info;
    memset(&info, 0, sizeof(info));
    mt32emu_get_rom_info(ctx, &info);
    return (info.control_rom_id != NULL) && (info.pcm_rom_id != NULL);
}

static void xmidi_mt32_try_add_rom_file(mt32emu_context ctx, const char *path, bool explicit_path)
{
    if (!path || !*path) {
        return;
    }
    if (mt32emu_add_rom_file(ctx, path) < 0) {
        if (explicit_path) {
            log_error("XMID/MT32: failed to load ROM '%s'\n", path);
        }
    }
}

static void xmidi_mt32_try_add_roms_in_dir(mt32emu_context ctx, const char *dir)
{
    static const char *const names[] = {
        "MT32_CONTROL.ROM", "MT32_PCM.ROM",
        "CM32L_CONTROL.ROM", "CM32L_PCM.ROM",
        "MT32_CONTROL.rom", "MT32_PCM.rom",
        "CM32L_CONTROL.rom", "CM32L_PCM.rom",
        "mt32_control.rom", "mt32_pcm.rom",
        "cm32l_control.rom", "cm32l_pcm.rom",
        "MT32.ROM", "CM32L.ROM",
        "mt32.rom", "cm32l.rom",
        NULL
    };
    int i;

    if (!dir || !*dir) {
        return;
    }
    for (i = 0; names[i] != NULL; ++i) {
        char *path = util_concat(dir, FSDEV_DIR_SEP_STR, names[i], NULL);
        mt32emu_add_rom_file(ctx, path);
        lib_free(path);
        if (xmidi_mt32_have_roms(ctx)) {
            return;
        }
    }
}

static bool xmidi_mt32_load_roms(mt32emu_context ctx, const char *rom_control, const char *rom_pcm)
{
    const char **paths;
    const char *env_path;
    int i;

    if (rom_control) {
        xmidi_mt32_try_add_rom_file(ctx, rom_control, true);
    }
    if (rom_pcm && (!rom_control || strcmp(rom_pcm, rom_control) != 0)) {
        xmidi_mt32_try_add_rom_file(ctx, rom_pcm, true);
    }
    if (xmidi_mt32_have_roms(ctx)) {
        return true;
    }

    env_path = getenv("MT32EMU_ROM_PATH");
    if (env_path && *env_path) {
        xmidi_mt32_try_add_roms_in_dir(ctx, env_path);
        if (xmidi_mt32_have_roms(ctx)) {
            return true;
        }
    }

    paths = os_get_paths_data();
    if (paths) {
        for (i = 0; paths[i] != NULL; ++i) {
            xmidi_mt32_try_add_roms_in_dir(ctx, paths[i]);
            if (xmidi_mt32_have_roms(ctx)) {
                return true;
            }
        }
    }

    return xmidi_mt32_have_roms(ctx);
}

bool xmidi_render_mt32_wav(const uint8_t *data_in, uint32_t len_in, uint32_t sample_rate,
                           const char *rom_control, const char *rom_pcm,
                           uint8_t **wav_out, uint32_t *wav_len, bool *tune_loops)
{
    xmidi_mevents_t evs = { 0 };
    mt32emu_context ctx = NULL;
    int16_t *pcm = NULL;
    uint8_t *wav = NULL;
    uint32_t frames = 0;
    uint32_t cap_frames = 0;
    uint32_t i = 0;
    uint64_t cur_tick = 0;
    bool ok = false;

    if (wav_out) *wav_out = NULL;
    if (wav_len) *wav_len = 0;
    if (tune_loops) *tune_loops = false;

    if (sample_rate == 0) {
        return false;
    }
    if (!xmidi_sequence(data_in, len_in, &evs, tune_loops)) {
        mevents_free(&evs);
        return false;
    }
    qsort(evs.v, evs.n, sizeof(evs.v[0]), ev_cmp);

    {
        mt32emu_report_handler_i rh;
        rh.v0 = NULL;
        ctx = mt32emu_create_context(rh, NULL);
    }
    if (!ctx) goto done;
    if (!xmidi_mt32_load_roms(ctx, rom_control, rom_pcm)) {
        log_error("XMID/MT32: control/PCM ROMs not found\n");
        goto done;
    }
    mt32emu_set_analog_output_mode(ctx, mt32emu_get_best_analog_output_mode((double)sample_rate));
    mt32emu_set_stereo_output_samplerate(ctx, (double)sample_rate);
    if (mt32emu_open_synth(ctx) != MT32EMU_RC_OK) goto done;

    while (i < evs.n) {
        uint64_t next_tick = evs.v[i].tick;
        if (next_tick > cur_tick) {
            uint64_t dt = next_tick - cur_tick;
            uint64_t nframes64 = (dt * (uint64_t)XMIDI_US_PER_TICK * (uint64_t)sample_rate + 500000ull) / 1000000ull;
            uint32_t nframes = (uint32_t)nframes64;
            if (frames + nframes > cap_frames) {
                uint32_t ncap = cap_frames ? cap_frames : 4096;
                int16_t *np;
                while (ncap < frames + nframes) ncap *= 2;
                np = (int16_t *)realloc(pcm, (size_t)ncap * 2 * sizeof(int16_t));
                if (!np) goto done;
                pcm = np;
                cap_frames = ncap;
            }
            if (nframes > 0) {
                mt32emu_render_bit16s(ctx, (mt32emu_bit16s *)(pcm + frames * 2), (mt32emu_bit32u)nframes);
                frames += nframes;
            }
            cur_tick = next_tick;
        }
        while (i < evs.n && evs.v[i].tick == cur_tick) {
            const uint8_t *d = evs.v[i].data;
            uint32_t dl = evs.v[i].len;
            if (dl >= 1 && dl <= 3 && (d[0] & 0x80) && d[0] < 0xf0) {
                uint32_t msg = d[0] | ((uint32_t)(dl > 1 ? d[1] : 0) << 8) | ((uint32_t)(dl > 2 ? d[2] : 0) << 16);
                mt32emu_play_msg(ctx, msg);
            }
            ++i;
        }
    }
    /* tail for release */
    {
        uint32_t tail = sample_rate; /* 1s */
        if (frames + tail > cap_frames) {
            int16_t *np = (int16_t *)realloc(pcm, (size_t)(frames + tail) * 2 * sizeof(int16_t));
            if (!np) goto done;
            pcm = np;
            cap_frames = frames + tail;
        }
        mt32emu_render_bit16s(ctx, (mt32emu_bit16s *)(pcm + frames * 2), (mt32emu_bit32u)tail);
        frames += tail;
    }

    {
        uint32_t data_bytes = frames * 4;
        uint32_t wav_bytes = 44 + data_bytes;
        wav = (uint8_t *)lib_malloc(wav_bytes);
        if (!wav) goto done;
        memcpy(wav + 0, "RIFF", 4);
        wav_put_u32(wav + 4, 36 + data_bytes);
        memcpy(wav + 8, "WAVE", 4);
        memcpy(wav + 12, "fmt ", 4);
        wav_put_u32(wav + 16, 16);
        wav_put_u16(wav + 20, 1);
        wav_put_u16(wav + 22, 2);
        wav_put_u32(wav + 24, sample_rate);
        wav_put_u32(wav + 28, sample_rate * 4);
        wav_put_u16(wav + 32, 4);
        wav_put_u16(wav + 34, 16);
        memcpy(wav + 36, "data", 4);
        wav_put_u32(wav + 40, data_bytes);
        memcpy(wav + 44, pcm, data_bytes);
        if (wav_out) *wav_out = wav; else lib_free(wav);
        if (wav_len) *wav_len = wav_bytes;
        wav = NULL;
    }
    ok = true;

done:
    if (ctx) {
        mt32emu_close_synth(ctx);
        mt32emu_free_context(ctx);
    }
    free(pcm);
    lib_free(wav);
    mevents_free(&evs);
    return ok;
}

#endif
