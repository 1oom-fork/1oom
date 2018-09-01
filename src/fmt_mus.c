#include "config.h"

#include <stdio.h>
#include <string.h>

#include "fmt_mus.h"
#include "bits.h"
#include "fmt_id.h"
#include "lib.h"
#include "log.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

#define DEBUGLEVEL_FMTMUS   4

#define HDR_MIDI_LEN    0x16

typedef struct noteoff_s {
    struct noteoff_s *next; /* next event, sorted by time */
    uint32_t t;     /* time of event */
    uint8_t buf[3]; /* first byte is 0 if unused, otherwise 0x8Z 0xNN 0x00 */
} noteoff_t;

/* MOO1 has max. 15 pending noteoffs */
#define NOTEOFFBUFSIZE  32

struct noteoffs_s {
    noteoff_t *top; /* noteoff with nearest time */
    int pos;        /* position of (likely) free tbl entry */
    int num;        /* number of pending events */
    int max;        /* max. number of pending events */
    noteoff_t tbl[NOTEOFFBUFSIZE];  /* noteoff events */
};

#define XMID_TICKSPERQ  55

/* -------------------------------------------------------------------------- */

static noteoff_t *xmid_find_free_noteoff(struct noteoffs_s *s)
{
    int i = s->pos;
    int num = NOTEOFFBUFSIZE;
    noteoff_t *n;
    while (num) {
        n = &s->tbl[i];
        if (++i == NOTEOFFBUFSIZE) {
            i = 0;
        }
        if (n->buf[0] == 0) {
            s->pos = i;
            return n;
        }
        --num;
    }
    return NULL;
}

static bool xmid_add_pending_noteoff(struct noteoffs_s *s, const uint8_t *data, uint32_t t_now, uint32_t duration)
{
    uint32_t t = t_now + duration;
    noteoff_t *n = xmid_find_free_noteoff(s);
    if (!n) {
        log_error("XMID: BUG noteoff tbl full!\n");
        return false;
    }
    n->next = NULL;
    n->t = t;
    n->buf[0] = data[0] & 0x8f; /* 9x -> 8x */
    n->buf[1] = data[1];
    n->buf[2] = 0;

    if (!s->top) {
        s->top = n;
    } else {
        noteoff_t *p, *q;
        p = s->top;
        q = NULL;
        while (p && (t >= p->t)) {
            q = p;
            p = p->next;
        }
        if (!q) {
            n->next = s->top;
            s->top = n;
        } else {
            q->next = n;
            n->next = p;
        }
    }
    if (++s->num > s->max) {
        s->max = s->num;
    }
    return true;
}

static uint32_t xmid_encode_delta_time(uint8_t *buf, uint32_t delta_time)
{
    uint32_t len_event = 0;
    uint32_t v = delta_time & 0x7f;
    while ((delta_time >>= 7) != 0) {
        v <<= 8;
        v |= (delta_time & 0x7f) | 0x80;
    }
    while (1) {
        buf[len_event++] = (uint8_t)(v & 0xff);
        if (v & 0x80) {
            v >>= 8;
        } else {
            return len_event;
        }
    }
}

static int xmid_convert_evnt(const uint8_t *data_in, uint32_t len_in, const uint8_t *timbre_tbl, uint16_t timbre_num, uint8_t *p, bool *tune_loops)
{
    struct noteoffs_s *s = lib_malloc(sizeof(struct noteoffs_s));
    int rc = -1, noteons = 0, looppoint = -1;
    uint32_t len_out = 0, t_now = 0;
    bool is_delta_time, last_was_delta_time = false, end_found = false;
    *tune_loops = false;

    while ((len_in > 0) && (!end_found)) {
        uint32_t len_event, len_delta_time, delta_time, add_extra_bytes, skip_extra_bytes;
        uint8_t buf_delta_time[4];
        uint8_t buf_extra[4];

        is_delta_time = false;
        add_extra_bytes = 0;
        skip_extra_bytes = 0;
        len_delta_time = 0;

        switch (*data_in & 0xf0) {
            case 0x90:
                delta_time = 0;
                skip_extra_bytes = 1;

                while ((data_in[2 + skip_extra_bytes] & 0x80)) {
                    delta_time += data_in[2 + skip_extra_bytes] & 0x7f;
                    delta_time <<= 7;
                    ++skip_extra_bytes;
                }
                delta_time += data_in[2 + skip_extra_bytes];

                if (!xmid_add_pending_noteoff(s, data_in, t_now, delta_time)) {
                    goto fail;
                }
                ++noteons;
                len_event = 3;
                break;

            case 0xb0:
                len_event = 3;
                {
                    uint8_t c, b;
                    c = data_in[1];
                    b = data_in[2];
                    if (0
                      || ((c >= 0x20) && (c <= 0x2e))
                      || ((c >= 0x3a) && (c <= 0x3f))
                      || ((c >= 0x6e) && (c <= 0x78))
                    ) {
                        LOG_DEBUG((DEBUGLEVEL_FMTMUS, "XMID: dropping unhandled AIL CC event %02x %02x %02x, notes %i\n", *data_in, c, b, noteons));
                        is_delta_time = last_was_delta_time;
                        len_event = 0;
                        skip_extra_bytes = 3;
                    }
                    switch (c) {
                        case 0x74:  /* AIL loop: FOR loop = 1 to n */
                            if (looppoint >= 0) {
                                log_warning("XMID: nth FOR loop unimpl\n");
                            } else {
                                looppoint = noteons;
                                if (looppoint > 0) {
                                    log_warning("XMID: FOR loop after %i notes unimpl\n", looppoint);
                                }
                            }
                            break;
                        case 0x75:  /* AIL loop: NEXT/BREAK */
                            LOG_DEBUG((DEBUGLEVEL_FMTMUS, "XMID: NEXT/BREAK at %i after %i notes, forcing end\n", looppoint, noteons));
                            if (looppoint >= 0) {
                                *tune_loops = true;
                            } else {
                                log_warning("XMID: NEXT/BREAK without FOR\n");
                            }
                            len_event = 0;
                            end_found = true;
                            buf_extra[0] = 0xff;
                            buf_extra[1] = 0x2f;
                            buf_extra[2] = 0x00;
                            add_extra_bytes = 3;
                            break;
                        default:
                            break;
                    }
                }
                break;

            case 0xa0:
            case 0xe0:
                len_event = 3;
                break;
            case 0x80:
                log_error("XMID: unexpected noteoff\n");
                goto fail;
            case 0xc0:
                len_event = 2;
#ifdef XMID_USE_BANKS
                {
                    int ti;
                    uint8_t patch;
                    uint8_t bank = 0;
                    patch = data_in[1];
                    for (ti = 0; ti < timbre_num; ++ti) {
                        if (timbre_tbl[ti * 2] == patch) {
                            bank = timbre_tbl[ti * 2 + 1];
                            break;
                        }
                    }
                    if (ti < timbre_num) {
                        LOG_DEBUG((DEBUGLEVEL_FMTMUS, "XMID: TIMB found bank 0x%02x for patch 0x%02x\n", bank, patch));
                        buf_extra[0] = 0xb0 | (data_in[1] & 0xf);
                        buf_extra[1] = 0;
                        buf_extra[2] = bank;
                        buf_extra[3] = 0;
                        add_extra_bytes = 4;
                    } else {
                        LOG_DEBUG((DEBUGLEVEL_FMTMUS, "XMID: TIMB no bank for patch 0x%02x\n", patch));
                    }
                }
#endif /*XMID_USE_BANKS*/
                break;
            case 0xd0:
                len_event = 2;
                break;
            case 0xf0:
                switch (*data_in & 0xf) {
                    case 0x08:
                    case 0x0a:
                    case 0x0b:
                    case 0x0c:
                        len_event = 1;
                        break;
                    case 0x0f:
                        len_event = 3 + data_in[2];
                        if (data_in[1] == 0x2f) {
                            end_found = true;
                        }
                        break;
                    default:
                        log_error("XMID: unhandled event 0x%02x\n", *data_in);
                        goto fail;
                }
                break;
            default:    /* 0x00..0x7f */
                is_delta_time = true;
                delta_time = 0;
                while (!(*data_in & 0x80)) {
                    delta_time += *data_in++;
                    --len_in;
                }
                while (s->top && ((t_now + delta_time) >= s->top->t)) {
                    uint32_t delay_noff = s->top->t - t_now;
                    len_delta_time = xmid_encode_delta_time(buf_delta_time, delay_noff);
                    for (int i = 0; i < len_delta_time; ++i) {
                        *p++ = buf_delta_time[i];
                    }
                    len_out += len_delta_time;
                    for (int i = 0; i < 3; ++i) {
                        *p++ = s->top->buf[i];
                    }
                    len_out += 3;
                    delta_time -= delay_noff;
                    t_now += delay_noff;
                    s->top->buf[0] = 0;
                    s->top = s->top->next;
                    --s->num;
                }
                t_now += delta_time;
                len_delta_time = xmid_encode_delta_time(buf_delta_time, delta_time);
                len_event = 0;
                break;
        }

        if (!is_delta_time && !last_was_delta_time && (len_event > 0)) {
            *p++ = 0;
            ++len_out;
        }

        if (end_found) {
            /* last event, add remaining noteoffs */
            LOG_DEBUG((DEBUGLEVEL_FMTMUS, "XMID: %i noteoffs at end, max %i noteoffs, total %i noteons\n", s->num, s->max, noteons));
            while (s->top) {
                for (int i = 0; i < 3; ++i) {
                    *p++ = s->top->buf[i];
                }
                *p++ = 0;
                len_out += 4;
                s->top->buf[0] = 0;
                s->top = s->top->next;
            }
            s->num = 0;
        }

        for (int i = 0; i < len_delta_time; ++i) {
            *p++ = buf_delta_time[i];
        }

        for (int i = 0; i < add_extra_bytes; ++i) {
            *p++ = buf_extra[i];
        }

        for (int i = 0; i < len_event; ++i) {
            uint8_t c;
            c = *data_in++;
            --len_in;
            *p++ = c;
        }

        while (skip_extra_bytes--) {
            ++data_in;
            --len_in;
        }

        len_out += len_event + len_delta_time + add_extra_bytes;
        last_was_delta_time = is_delta_time;
    }

    rc = len_out;
fail:
    lib_free(s);
    return rc;
}

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
    uint8_t *data = NULL;
    const uint8_t *timbre_tbl;
    uint32_t len_timbre, len_evnt;
    uint16_t timbre_num;
    int len = 0;

    if (0
      || (fmt_mus_detect(data_in, len_in) != MUS_TYPE_LBXXMID)
      || (len_in < 0x4e)
      || (memcmp(&data_in[0x10], (const uint8_t *)"FORM", 4) != 0)
      || (memcmp(&data_in[0x3e], (const uint8_t *)"TIMB", 4) != 0)
      || ((len_timbre = GET_BE_32(&data_in[0x42])), (len_in < (0x4e + len_timbre)))
      || (memcmp(&data_in[0x46 + len_timbre], (const uint8_t *)"EVNT", 4) != 0)
      || ((len_evnt = GET_BE_32(&data_in[0x4a + len_timbre])), (len_in < (0x4e + len_timbre + len_evnt)))
    ) {
        log_error("XMID: invalid header\n");
        goto fail;
    }
    LOG_DEBUG((DEBUGLEVEL_FMTMUS, "XMID: lent %i lene %i\n", len_timbre, len_evnt));

    timbre_num = GET_LE_16(&data_in[0x46]);
    timbre_tbl = &data_in[0x48];

    data_in += 0x4e + len_timbre;

    /* max. len_out/len_in ratio for MOO1 data is about 1.8 */
    data = lib_malloc(HDR_MIDI_LEN + len_evnt * 2);
    {
        uint8_t hdr[] = {
            /*00*/ 'M', 'T', 'h', 'd',
            /*04*/ 0, 0, 0, 6,
            /*08*/ 0, 0, 0, 1,
            /*0c*/ (XMID_TICKSPERQ >> 8) & 0xff, XMID_TICKSPERQ & 0xff,
            /*0e*/ 'M', 'T', 'r', 'k'
            /*12*/ /* length, big endian */
        };
        memcpy(data, hdr, sizeof(hdr));
    }

    len = xmid_convert_evnt(data_in, len_evnt, timbre_tbl, timbre_num, &data[HDR_MIDI_LEN], tune_loops);
    LOG_DEBUG((DEBUGLEVEL_FMTMUS, "XMID: lene %i len %i (%f) %s\n", len_evnt, len, (double)len / (double)len_evnt, *tune_loops ? "loop" : "once"));
    if (len < 0) {
        goto fail;
    }
    SET_BE_32(&data[0x12], len);
    len += HDR_MIDI_LEN;
    data = lib_realloc(data, len);

    if (data_out_ptr) {
        *data_out_ptr = data;
    }
    if (len_out_ptr) {
        *len_out_ptr = len;
    }
    return true;

fail:
    lib_free(data);
    if (data_out_ptr) {
        *data_out_ptr = NULL;
    }
    if (len_out_ptr) {
        *len_out_ptr = 0;
    }
    return false;
}
