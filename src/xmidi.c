#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "bits.h"
#include "fmt_id.h"
#include "lbx.h"
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
#define XMIDI_MT32_DEF_VOLUME 90
#define XMIDI_VOLUME_INVALID 0xffu
#define XMIDI_GTL_ENTRY_LEN 6u
#define XMIDI_GTL_TIMBRE_LEN 0xf6u
#define XMIDI_MT32_CTRL_NUM 9

static const uint8_t xmidi_mt32_prg_default[] = { 68, 48, 95, 78, 41, 3, 110, 122 };
static const uint8_t xmidi_mt32_logged_ctrls[XMIDI_MT32_CTRL_NUM] = { 7, 1, 10, 11, 64, 114, 110, 111, 112 };
static const uint8_t xmidi_mt32_ctrl_default[XMIDI_MT32_CTRL_NUM] = { 127, 0, 64, 127, 0, 0, 0, 0, 0 };

static xmidi_mt32_timbre_lookup_t xmidi_mt32_timbre_lookup = NULL;
static void *xmidi_mt32_timbre_lookup_ctx = NULL;

void xmidi_mt32_set_timbre_lookup(xmidi_mt32_timbre_lookup_t lookup, void *ctx)
{
    xmidi_mt32_timbre_lookup = lookup;
    xmidi_mt32_timbre_lookup_ctx = ctx;
}

typedef struct {
    uint8_t *data;
    uint32_t len;
    uint32_t header_off;
} xmidi_gtl_image_t;

static xmidi_gtl_image_t *xmidi_gtl_images = NULL;
static uint32_t xmidi_gtl_image_num = 0;

static bool xmidi_gtl_timbre_at(const uint8_t *data, uint32_t len, uint32_t off)
{
    uint16_t prefix;

    if ((off > len) || (len - off < 2u + XMIDI_GTL_TIMBRE_LEN)) {
        return false;
    }
    prefix = GET_LE_16(data + off);
    return (prefix == XMIDI_GTL_TIMBRE_LEN) || (prefix == XMIDI_GTL_TIMBRE_LEN + 2u);
}

static bool xmidi_gtl_valid_at(const uint8_t *data, uint32_t len, uint32_t header_off, uint32_t *timbre_num)
{
    uint32_t pos = header_off;
    uint32_t count = 0;

    if (timbre_num) {
        *timbre_num = 0;
    }
    while ((pos + XMIDI_GTL_ENTRY_LEN <= len) && (count < 4096)) {
        uint8_t patch = data[pos];
        uint8_t bank = data[pos + 1];
        uint32_t rel = GET_LE_32(data + pos + 2);

        if (bank == 0xffu) {
            if (timbre_num) {
                *timbre_num = count;
            }
            return count > 0;
        }
        if ((patch > 127u) || (bank > 127u) || !xmidi_gtl_timbre_at(data, len, rel)) {
            return false;
        }
        pos += XMIDI_GTL_ENTRY_LEN;
        ++count;
    }
    return false;
}

static uint32_t xmidi_gtl_find_header(const uint8_t *data, uint32_t len, uint32_t *timbre_num)
{
    uint32_t off;

    if (xmidi_gtl_valid_at(data, len, 0, timbre_num)) {
        return 0;
    }
    for (off = 1; off + XMIDI_GTL_ENTRY_LEN <= len; ++off) {
        if (xmidi_gtl_valid_at(data, len, off, timbre_num)) {
            return off;
        }
    }
    return 0xffffffffu;
}

static bool xmidi_mt32_gtl_lookup(void *ctx, uint8_t bank, uint8_t patch,
                                  const uint8_t **data, uint32_t *len)
{
    uint32_t i;
    (void)ctx;

    for (i = 0; i < xmidi_gtl_image_num; ++i) {
        const xmidi_gtl_image_t *img = &xmidi_gtl_images[i];
        uint32_t pos = img->header_off;
        while (pos + XMIDI_GTL_ENTRY_LEN <= img->len) {
            uint8_t e_patch = img->data[pos];
            uint8_t e_bank = img->data[pos + 1];
            uint32_t rel = GET_LE_32(img->data + pos + 2);

            if (e_bank == 0xffu) {
                break;
            }
            if ((e_patch > 127u) || (e_bank > 127u)) {
                break;
            }
            if ((e_bank == bank) && (e_patch == patch)
             && xmidi_gtl_timbre_at(img->data, img->len, rel)) {
                *data = img->data + rel;
                *len = 2u + XMIDI_GTL_TIMBRE_LEN;
                return true;
            }
            pos += XMIDI_GTL_ENTRY_LEN;
        }
    }
    return false;
}

static void xmidi_mt32_init_gtl(void)
{
    static bool checked = false;
    uint32_t image_num = 0;
    uint32_t timbre_num = 0;
    int n, i;

    if (checked || xmidi_mt32_timbre_lookup) {
        return;
    }
    checked = true;
    n = lbxfile_num_items(LBXFILE_SNDDRV);
    if (n <= 0) {
        return;
    }
    xmidi_gtl_images = lib_malloc((size_t)n * sizeof(*xmidi_gtl_images));
    for (i = 0; i < n; ++i) {
        uint32_t item_len = 0;
        uint32_t nt = 0;
        uint8_t *item = lbxfile_item_get(LBXFILE_SNDDRV, (uint16_t)i, &item_len);

        if (!item) {
            continue;
        }
        {
            uint32_t hdr = xmidi_gtl_find_header(item, item_len, &nt);
            if (hdr != 0xffffffffu) {
                xmidi_gtl_image_t *img = &xmidi_gtl_images[image_num++];
                img->data = lib_malloc(item_len);
                memcpy(img->data, item, item_len);
                img->len = item_len;
                img->header_off = hdr;
                timbre_num += nt;
            }
        }
        lbxfile_item_release(LBXFILE_SNDDRV, item);
    }
    if (image_num > 0) {
        xmidi_gtl_image_num = image_num;
        log_message("XMID/MT32: loaded %u GTL image(s), %u timbre(s) from snddrv.lbx\n", image_num, timbre_num);
        xmidi_mt32_set_timbre_lookup(xmidi_mt32_gtl_lookup, NULL);
    } else {
        lib_free(xmidi_gtl_images);
        xmidi_gtl_images = NULL;
    }
}

typedef struct {
    uint8_t *data;
    uint32_t len;
    uint64_t tick;
    uint32_t seq;
    bool sysex;
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
    uint8_t addr[3];
    uint8_t data[32];
    uint32_t len;
} xmidi_sysex_queue_t;

typedef struct {
    bool mt32;
    uint16_t vol_percent;
    uint8_t phys_bank[16];
    uint8_t phys_patch[16];
    uint8_t patch_bank[128];
    uint8_t chan_map[16];
    bool phys_locked[16];
    bool chan_protected[16];
    uint8_t chan_vol[16];
    uint8_t chan_indirect[16];
    uint8_t ctrl_table[256];
    uint8_t global_ctrl[XMIDI_MT32_CTRL_NUM][16];
    uint8_t global_program[16];
    uint8_t global_pitch_l[16];
    uint8_t global_pitch_h[16];
    uint8_t active_notes[16];
    uint8_t timb_attribs[64];
    uint8_t timb_bank[64];
    uint8_t timb_num[64];
    uint8_t phys_timbre[16];
    uint64_t timb_hist[64];
    uint64_t note_seq;
    xmidi_sysex_queue_t sysex_q[3];
} xmidi_seq_state_t;

static int xmidi_mt32_ctrl_index(uint8_t cc)
{
    int i;
    for (i = 0; i < XMIDI_MT32_CTRL_NUM; ++i) {
        if (xmidi_mt32_logged_ctrls[i] == cc) {
            return i;
        }
    }
    return -1;
}

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
    e->seq = m->n - 1;
    e->sysex = false;
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

static bool xmidi_sysex_append(uint8_t **buf, uint32_t *len, uint32_t *cap, const uint8_t *data, uint32_t n)
{
    if (n == 0) {
        return true;
    }
    if (*len + n > *cap) {
        uint32_t ncap = *cap ? *cap : 64;
        uint8_t *nv;
        while (ncap < *len + n) {
            ncap *= 2;
        }
        nv = (uint8_t *)lib_realloc(*buf, ncap);
        if (!nv) {
            return false;
        }
        *buf = nv;
        *cap = ncap;
    }
    memcpy(*buf + *len, data, n);
    *len += n;
    return true;
}

static void xmidi_sysex_flush(xmidi_mevents_t *out, uint64_t tick, uint8_t **buf, uint32_t *len, uint32_t *cap, bool *active)
{
    if (!*active || *len == 0) {
        goto reset;
    }
    if ((*buf)[*len - 1] != 0xf7u
     && !xmidi_sysex_append(buf, len, cap, (const uint8_t[]){ 0xf7u }, 1)) {
        goto reset;
    }
    mevents_push(out, tick, *buf, *len);
    if (out->n > 0) {
        out->v[out->n - 1].sysex = true;
    }
reset:
    lib_free(*buf);
    *buf = NULL;
    *len = 0;
    *cap = 0;
    *active = false;
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

static void xmidi_seq_state_init(xmidi_seq_state_t *s, bool mt32)
{
    int i, j;
    memset(s, 0, sizeof(*s));
    s->mt32 = mt32;
    s->vol_percent = mt32 ? XMIDI_MT32_DEF_VOLUME : 100;
    for (i = 0; i < 16; ++i) {
        s->chan_map[i] = (uint8_t)i;
        s->phys_patch[i] = 0xff;
        s->phys_timbre[i] = 0xff;
        s->chan_vol[i] = (uint8_t)XMIDI_VOLUME_INVALID;
        s->chan_indirect[i] = (uint8_t)XMIDI_VOLUME_INVALID;
        s->global_program[i] = (uint8_t)XMIDI_VOLUME_INVALID;
        s->global_pitch_l[i] = (uint8_t)XMIDI_VOLUME_INVALID;
        s->global_pitch_h[i] = (uint8_t)XMIDI_VOLUME_INVALID;
        for (j = 0; j < XMIDI_MT32_CTRL_NUM; ++j) {
            s->global_ctrl[j][i] = (uint8_t)XMIDI_VOLUME_INVALID;
        }
    }
    for (j = 0; j < XMIDI_MT32_CTRL_NUM; ++j) {
        s->ctrl_table[xmidi_mt32_logged_ctrls[j]] = xmidi_mt32_ctrl_default[j];
    }
    if (mt32) {
        for (i = 1; i <= 9; ++i) {
            for (j = 0; j < XMIDI_MT32_CTRL_NUM; ++j) {
                s->global_ctrl[j][i] = xmidi_mt32_ctrl_default[j];
            }
            s->global_pitch_l[i] = 0;
            s->global_pitch_h[i] = 64;
        }
        for (i = 1; i <= (int)sizeof(xmidi_mt32_prg_default); ++i) {
            s->phys_patch[i] = xmidi_mt32_prg_default[i - 1];
            s->global_program[i] = xmidi_mt32_prg_default[i - 1];
        }
    }
}

static void xmidi_sysex_addr_add(uint8_t addr[3], uint32_t addend)
{
    uint32_t v = (uint32_t)addr[2] + addend;
    addr[2] = (uint8_t)(v & 0x7f);
    v = (uint32_t)addr[1] + (v >> 7);
    addr[1] = (uint8_t)(v & 0x7f);
    addr[0] = (uint8_t)((addr[0] + (v >> 7)) & 0x7f);
}

static void xmidi_mt32_sysex_push(xmidi_mevents_t *out, uint64_t tick, const uint8_t addr[3], const uint8_t *data, uint32_t len)
{
    uint8_t msg[80];
    uint32_t sum = (uint32_t)addr[0] + (uint32_t)addr[1] + (uint32_t)addr[2];
    uint32_t i;
    msg[0] = 0xf0;
    msg[1] = 0x41;
    msg[2] = 0x10;
    msg[3] = 0x16;
    msg[4] = 0x12;
    msg[5] = addr[0];
    msg[6] = addr[1];
    msg[7] = addr[2];
    for (i = 0; i < len; ++i) {
        msg[8 + i] = data[i];
        sum += data[i];
    }
    msg[8 + len] = (uint8_t)((0u - sum) & 0x7fu);
    msg[9 + len] = 0xf7;
    mevents_push(out, tick, msg, 10 + len);
    if (out->n > 0) {
        out->v[out->n - 1].sysex = true;
    }
}

static void xmidi_mt32_init(xmidi_mevents_t *out)
{
    static const uint8_t part_chans[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    static const uint8_t part_rsv[] = { 3, 4, 3, 4, 3, 4, 3, 4, 4 };
    static const uint8_t reverb[] = { 0, 3, 2 };
    static const uint8_t addr_reset[] = { 0x7f, 0x00, 0x00 };
    static const uint8_t addr_chans[] = { 0x10, 0x00, 0x0d };
    static const uint8_t addr_rsv[] = { 0x10, 0x00, 0x04 };
    static const uint8_t addr_reverb[] = { 0x10, 0x00, 0x01 };
    static const uint8_t reset[] = { 1 };
    int ch;

    xmidi_mt32_sysex_push(out, 0, addr_reset, reset, (uint32_t)sizeof(reset));
    xmidi_mt32_sysex_push(out, 0, addr_chans, part_chans, (uint32_t)sizeof(part_chans));
    xmidi_mt32_sysex_push(out, 0, addr_rsv, part_rsv, (uint32_t)sizeof(part_rsv));
    xmidi_mt32_sysex_push(out, 0, addr_reverb, reverb, (uint32_t)sizeof(reverb));

    for (ch = 1; ch <= 9; ++ch) {
        uint8_t msg[3];
        msg[0] = (uint8_t)(0xb0 | ch);
        msg[1] = 7; msg[2] = 127; mevents_push(out, 0, msg, 3);
        msg[1] = 1; msg[2] = 0; mevents_push(out, 0, msg, 3);
        msg[1] = 10; msg[2] = 64; mevents_push(out, 0, msg, 3);
        msg[1] = 11; msg[2] = 127; mevents_push(out, 0, msg, 3);
        msg[1] = 64; msg[2] = 0; mevents_push(out, 0, msg, 3);
        msg[0] = (uint8_t)(0xe0 | ch);
        msg[1] = 0; msg[2] = 64; mevents_push(out, 0, msg, 3);
        if (ch <= (int)sizeof(xmidi_mt32_prg_default)) {
            msg[0] = (uint8_t)(0xc0 | ch);
            msg[1] = xmidi_mt32_prg_default[ch - 1]; mevents_push(out, 0, msg, 2);
        }
    }
}

static void xmidi_mt32_sysex_ctrl(xmidi_seq_state_t *s, xmidi_mevents_t *out, uint64_t tick, uint8_t cc, uint8_t val)
{
    uint32_t off = (uint32_t)(cc - 32);
    xmidi_sysex_queue_t *q = &s->sysex_q[off / 5];
    switch (off % 5) {
        case 0:
        case 1:
        case 2:
            q->addr[off % 5] = val;
            return;
        default:
            break;
    }
    if (q->len < (uint32_t)sizeof(q->data)) {
        q->data[q->len++] = val;
    }
    if ((off % 5) == 4 || q->len == (uint32_t)sizeof(q->data)) {
        bool final = (off % 5) == 4;
        xmidi_mt32_sysex_push(out, tick, q->addr, q->data, q->len);
        xmidi_sysex_addr_add(q->addr, (!final && q->len == (uint32_t)sizeof(q->data)) ? q->len : q->len - 1u);
        q->len = 0;
    }
}

static int xmidi_mt32_index_timbre(const xmidi_seq_state_t *s, uint8_t bank, uint8_t patch)
{
    int i;
    for (i = 0; i < 64; ++i) {
        if ((s->timb_attribs[i] & 0x80) != 0 && s->timb_bank[i] == bank && s->timb_num[i] == patch) {
            return i;
        }
    }
    return -1;
}

static void xmidi_mt32_setup_patch(xmidi_seq_state_t *s, xmidi_mevents_t *out, uint64_t tick, uint8_t patch, uint8_t bank)
{
    static const uint8_t builtin_groups[] = { 0, 1 };
    uint8_t data[2];
    uint8_t addr[3];
    uint8_t group = builtin_groups[patch >= 64 ? 1 : 0];
    uint8_t timbre = (uint8_t)(patch & 63);
    int idx;

    if (bank != 0) {
        idx = xmidi_mt32_index_timbre(s, bank, patch);
        if (idx >= 0) {
            group = 2;
            timbre = (uint8_t)idx;
        }
    }
    s->patch_bank[patch] = bank;
    addr[0] = 0x05;
    addr[1] = 0x00;
    addr[2] = 0x00;
    xmidi_sysex_addr_add(addr, (uint32_t)patch * 8);
    data[0] = group;
    data[1] = timbre;
    xmidi_mt32_sysex_push(out, tick, addr, data, 2);
}

static bool xmidi_mt32_install_timbre(xmidi_seq_state_t *s, xmidi_mevents_t *out, uint64_t tick,
                                      uint8_t bank, uint8_t patch, const uint8_t *data, uint32_t len)
{
    static const uint8_t addr_base[5][3] = {
        { 8, 0, 0x00 }, { 8, 0, 0x0e }, { 8, 0, 0x48 }, { 8, 1, 0x02 }, { 8, 1, 0x3c }
    };
    static const uint8_t data_offs[5] = { 0x00, 0x0e, 0x48, 0x82, 0xbc };
    static const uint8_t chunk_lens[5] = { 0x0e, 0x3a, 0x3a, 0x3a, 0x3a };
    int slot = -1;
    int i;

    if (bank == 0) {
        xmidi_mt32_setup_patch(s, out, tick, patch, bank);
        return true;
    }
    if (bank == 127 || !data || len < 2u + 0xf6u) {
        return false;
    }
    if (xmidi_mt32_index_timbre(s, bank, patch) >= 0) {
        xmidi_mt32_setup_patch(s, out, tick, patch, bank);
        return true;
    }
    for (i = 0; i < 64; ++i) {
        if ((s->timb_attribs[i] & 0x80) == 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        uint64_t best_hist = 0;
        bool have_best = false;
        for (i = 0; i < 64; ++i) {
            if ((s->timb_attribs[i] & 0x40) == 0) {
                if (!have_best || s->timb_hist[i] <= best_hist) {
                    best_hist = s->timb_hist[i];
                    slot = i;
                    have_best = true;
                }
            }
        }
        if (slot < 0) {
            return false;
        }
    }
    s->timb_attribs[slot] = 0x80;
    s->timb_bank[slot] = bank;
    s->timb_num[slot] = patch;
    s->timb_hist[slot] = s->note_seq++;
    for (i = 0; i < 5; ++i) {
        uint8_t addr[3];
        addr[0] = addr_base[i][0];
        addr[1] = (uint8_t)(slot * 2 + addr_base[i][1]);
        addr[2] = addr_base[i][2];
        xmidi_mt32_sysex_push(out, tick, addr, data + 2 + data_offs[i], chunk_lens[i]);
    }
    xmidi_mt32_setup_patch(s, out, tick, patch, bank);
    return true;
}

static void xmidi_mt32_load_timbres(xmidi_seq_state_t *s, xmidi_mevents_t *out,
                                    const uint8_t *data, uint32_t len)
{
    xmidi_form_t form;
    uint32_t i;

    if (!xmidi_mt32_timbre_lookup || !xmidi_parse(data, len, &form)) {
        return;
    }
    for (i = 0; i < form.timb_num; ++i) {
        uint8_t patch = form.timb_tbl[i * 2];
        uint8_t bank = form.timb_tbl[i * 2 + 1];
        const uint8_t *timb_data = NULL;
        uint32_t timb_len = 0;

        if (bank == 0 || bank == 127) {
            continue;
        }
        if (xmidi_mt32_index_timbre(s, bank, patch) >= 0) {
            continue;
        }
        if (!xmidi_mt32_timbre_lookup(xmidi_mt32_timbre_lookup_ctx, bank, patch, &timb_data, &timb_len)) {
            continue;
        }
        xmidi_mt32_install_timbre(s, out, 0, bank, patch, timb_data, timb_len);
    }
}

static uint8_t xmidi_channel_out(const xmidi_seq_state_t *s, uint8_t ch)
{
    if (!s->mt32 || ch >= 16) {
        return ch;
    }
    return s->chan_map[ch];
}

static uint8_t xmidi_mt32_scale_volume(const xmidi_seq_state_t *s, uint8_t val)
{
    uint32_t v = (uint32_t)val * (uint32_t)s->vol_percent / 100u;
    return (uint8_t)(v > 127u ? 127u : v);
}

static bool xmidi_midi_push(xmidi_mevents_t *out, xmidi_seq_state_t *s, uint64_t tick, uint8_t status, uint8_t d1, uint8_t d2, uint32_t len)
{
    uint8_t ch = (uint8_t)(status & 0x0f);
    uint8_t msg[3];
    if (s->mt32 && (s->phys_locked[ch] || xmidi_channel_out(s, ch) >= 10)) {
        return false;
    }
    msg[0] = (uint8_t)((status & 0xf0) | xmidi_channel_out(s, ch));
    msg[1] = d1;
    msg[2] = d2;
    return mevents_push(out, tick, msg, len);
}

static void noteoff_emit(xmidi_mevents_t *out, xmidi_seq_state_t *s, xmidi_noteoff_t *n)
{
    uint8_t ch = (uint8_t)(n->status & 0x0f);
    uint8_t phys = xmidi_channel_out(s, ch);
    uint8_t msg[3];
    msg[0] = (uint8_t)(0x80 | phys);
    msg[1] = n->note;
    msg[2] = 0;
    if (!s->mt32 || phys < 10) {
        mevents_push(out, n->tick, msg, 3);
        if (s->mt32 && phys >= 1 && s->active_notes[phys] > 0) {
            --s->active_notes[phys];
        }
    }
    n->used = false;
}

static void xmidi_flush_physical_notes(xmidi_mevents_t *out, xmidi_seq_state_t *s, xmidi_noteoff_t *tbl, uint64_t tick, uint8_t phys)
{
    int i;
    for (i = 0; i < XMIDI_NOTE_SLOTS; ++i) {
        if (tbl[i].used && xmidi_channel_out(s, (uint8_t)(tbl[i].status & 0x0f)) == phys) {
            tbl[i].tick = tick;
            noteoff_emit(out, s, &tbl[i]);
        }
    }
    s->active_notes[phys] = 0;
}

static void xmidi_mt32_apply_program(xmidi_seq_state_t *s, xmidi_mevents_t *out, uint64_t tick,
                                     uint8_t ch, uint8_t phys, uint8_t patch, bool emit)
{
    int idx;
    s->global_program[phys] = patch;
    s->phys_patch[phys] = patch;
    if (s->phys_bank[phys] != s->patch_bank[patch]) {
        xmidi_mt32_setup_patch(s, out, tick, patch, s->phys_bank[phys]);
    }
    idx = xmidi_mt32_index_timbre(s, s->phys_bank[phys], patch);
    s->phys_timbre[phys] = (idx >= 0) ? (uint8_t)idx : 0xff;
    if (emit) {
        xmidi_midi_push(out, s, tick, (uint8_t)(0xc0 | ch), patch, 0, 2);
    }
}

static void xmidi_mt32_lock_chan(xmidi_seq_state_t *s, xmidi_mevents_t *out, xmidi_noteoff_t *notes, uint64_t tick, uint8_t ch, uint8_t val)
{
    if (val >= 64) {
        int phys = -1;
        int i, pass;
        for (pass = 0; pass < 2 && phys < 0; ++pass) {
            for (i = 9; i >= 2; --i) {
                if (s->phys_locked[i] || (pass == 0 && s->chan_protected[i])) {
                    continue;
                }
                if (phys < 0 || s->active_notes[i] < s->active_notes[phys]) {
                    phys = i;
                }
            }
        }
        if (phys < 0) {
            return;
        }
        s->phys_locked[phys] = true;
        mevents_push(out, tick, (const uint8_t[]){ (uint8_t)(0xb0 | phys), 64, 0 }, 3);
        xmidi_flush_physical_notes(out, s, notes, tick, (uint8_t)phys);
        s->chan_map[ch] = (uint8_t)phys;
    } else {
        uint8_t phys = s->chan_map[ch];
        if (phys >= 1 && phys <= 9) {
            int i;
            s->phys_locked[phys] = false;
            xmidi_flush_physical_notes(out, s, notes, tick, phys);
            mevents_push(out, tick, (const uint8_t[]){ (uint8_t)(0xb0 | phys), 64, 0 }, 3);
            mevents_push(out, tick, (const uint8_t[]){ (uint8_t)(0xb0 | phys), 123, 0 }, 3);
            for (i = 0; i < XMIDI_MT32_CTRL_NUM; ++i) {
                uint8_t v = s->global_ctrl[i][phys];
                uint8_t cc = xmidi_mt32_logged_ctrls[i];
                if (v == XMIDI_VOLUME_INVALID) {
                    continue;
                }
                if (cc == 114) {
                    s->phys_bank[phys] = v;
                } else if (cc < 110) {
                    mevents_push(out, tick, (const uint8_t[]){ (uint8_t)(0xb0 | phys), cc, v }, 3);
                }
            }
            if (s->global_program[phys] != XMIDI_VOLUME_INVALID) {
                xmidi_mt32_apply_program(s, out, tick, phys, phys, s->global_program[phys], true);
            }
            if (s->global_pitch_l[phys] != XMIDI_VOLUME_INVALID && s->global_pitch_h[phys] != XMIDI_VOLUME_INVALID) {
                mevents_push(out, tick, (const uint8_t[]){ (uint8_t)(0xe0 | phys), s->global_pitch_l[phys], s->global_pitch_h[phys] }, 3);
            }
        }
        s->chan_map[ch] = ch;
    }
}

static void flush_due_noteoffs(xmidi_mevents_t *out, xmidi_seq_state_t *s, xmidi_noteoff_t *tbl, uint64_t now_tick)
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
        noteoff_emit(out, s, &tbl[best]);
    }
}

static void flush_all_noteoffs(xmidi_mevents_t *out, xmidi_seq_state_t *s, xmidi_noteoff_t *tbl, uint64_t now_tick)
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
        noteoff_emit(out, s, &tbl[best]);
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

static bool xmidi_sequence(xmidi_seq_state_t *s, const uint8_t *data, uint32_t len, xmidi_mevents_t *out, bool *tune_loops)
{
    xmidi_form_t form;
    xmidi_noteoff_t notes[XMIDI_NOTE_SLOTS];
    uint32_t pos = 0;
    uint64_t tick = 0;
    uint32_t interval = 0;
    int32_t loop_count[XMIDI_FOR_NEST];
    uint32_t loop_pos[XMIDI_FOR_NEST];
    bool end = false;
    bool infinite = false;
    uint8_t *sysex = NULL;
    uint32_t sysex_len = 0;
    uint32_t sysex_cap = 0;
    bool sysex_active = false;

    if (tune_loops) {
        *tune_loops = false;
    }
    memset(notes, 0, sizeof(notes));
    for (int i = 0; i < XMIDI_FOR_NEST; ++i) { loop_count[i] = -1; loop_pos[i] = 0; }

    if (!xmidi_parse(data, len, &form)) {
        log_error("XMID: invalid/corrupt data\n");
        return false;
    }

    while (pos < form.evnt_len && !end) {
        uint8_t st = form.evnt[pos++];
        uint32_t ev_pos = pos - 1;
        if (st < 0x80) {
            interval += st;
            continue;
        }
        tick += interval;
        interval = 0;
        flush_due_noteoffs(out, s, notes, tick);
        if (st != 0xf0u && st != 0xf7u) {
            xmidi_sysex_flush(out, tick, &sysex, &sysex_len, &sysex_cap, &sysex_active);
        }

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
                    flush_all_noteoffs(out, s, notes, tick);
                    mevents_push(out, tick, msg, 3);
                    end = true;
                } else if (type == 0x51 && mlen == 3) {
                } else if (type == 0x58 && mlen >= 2) {
                    uint8_t msg[7] = { 0xff, 0x58, 0x04, form.evnt[pos], form.evnt[pos + 1], (mlen > 2 ? form.evnt[pos + 2] : 24), (mlen > 3 ? form.evnt[pos + 3] : 8) };
                    mevents_push(out, tick, msg, 7);
                }
                pos += mlen;
            } else if (st == 0xf0 || st == 0xf7) {
                uint32_t slen = xmidi_vlq_read(form.evnt, form.evnt_len, &pos);
                if (pos + slen > form.evnt_len) break;
                if (s->mt32) {
                    static const uint8_t f0 = 0xf0u;
                    bool ok = true;
                    bool complete = false;
                    if (st == 0xf0u) {
                        xmidi_sysex_flush(out, tick, &sysex, &sysex_len, &sysex_cap, &sysex_active);
                        ok = xmidi_sysex_append(&sysex, &sysex_len, &sysex_cap, &f0, 1)
                          && xmidi_sysex_append(&sysex, &sysex_len, &sysex_cap, form.evnt + pos, slen);
                        sysex_active = ok;
                    } else {
                        if (!sysex_active) {
                            ok = xmidi_sysex_append(&sysex, &sysex_len, &sysex_cap, &f0, 1);
                            sysex_active = ok;
                        }
                        if (ok) {
                            ok = xmidi_sysex_append(&sysex, &sysex_len, &sysex_cap, form.evnt + pos, slen);
                        }
                    }
                    if (!ok) {
                        lib_free(sysex);
                        sysex = NULL;
                        sysex_len = 0;
                        sysex_cap = 0;
                        sysex_active = false;
                    } else {
                        complete = (slen > 0) && (form.evnt[pos + slen - 1] == 0xf7u);
                        if (complete) {
                            xmidi_sysex_flush(out, tick, &sysex, &sysex_len, &sysex_cap, &sysex_active);
                        }
                    }
                }
                pos += slen;
            } else {
                uint8_t msg[1] = { st };
                mevents_push(out, tick, msg, 1);
            }
            continue;
        }

        switch (st & 0xf0) {
        case 0x80:
        case 0x90: {
            uint8_t ch = (uint8_t)(st & 0x0f);
            uint8_t note, vel;
            uint32_t dur;
            if (pos + 1 >= form.evnt_len) { end = true; break; }
            note = form.evnt[pos++];
            vel = form.evnt[pos++];
            dur = xmidi_vlq_read(form.evnt, form.evnt_len, &pos);
            if (xmidi_midi_push(out, s, tick, (uint8_t)(0x90 | ch), note, vel, 3)) {
                uint8_t phys = xmidi_channel_out(s, ch);
                if (s->mt32) {
                    ++s->note_seq;
                    if (s->phys_timbre[phys] != 0xff) {
                        s->timb_hist[s->phys_timbre[phys]] = s->note_seq;
                    }
                    if (phys >= 1 && phys <= 9) {
                        ++s->active_notes[phys];
                    }
                }
                schedule_noteoff(notes, ch, note, tick + (dur ? dur : 1));
            }
            break;
        }
        case 0xa0:
        case 0xe0: {
            uint8_t ch = (uint8_t)(st & 0x0f);
            uint8_t d1, d2;
            if (pos + 1 >= form.evnt_len) { end = true; break; }
            d1 = form.evnt[pos++]; d2 = form.evnt[pos++];
            if (s->mt32 && (st & 0xf0) == 0xe0) {
                uint8_t phys = xmidi_channel_out(s, ch);
                s->global_pitch_l[phys] = d1;
                s->global_pitch_h[phys] = d2;
            }
            xmidi_midi_push(out, s, tick, st, d1, d2, 3);
            break;
        }
        case 0xb0: {
            uint8_t ch = (uint8_t)(st & 0x0f);
            uint8_t cc, val;
            if (pos + 1 >= form.evnt_len) { end = true; break; }
            cc = form.evnt[pos++]; val = form.evnt[pos++];
            if (s->chan_indirect[ch] != XMIDI_VOLUME_INVALID) {
                uint8_t src = s->chan_indirect[ch];
                s->chan_indirect[ch] = (uint8_t)XMIDI_VOLUME_INVALID;
                val = s->ctrl_table[src];
            }
            s->ctrl_table[cc] = val;
            if (cc == 115) {
                s->chan_indirect[ch] = val;
            } else if (cc == 116) {
                int i, slot = -1;
                for (i = 0; i < XMIDI_FOR_NEST; ++i) {
                    if (loop_count[i] == -1) {
                        slot = i;
                        break;
                    }
                }
                if (slot >= 0) {
                    loop_count[slot] = val;
                    loop_pos[slot] = ev_pos;
                }
            } else if (cc == 117) {
                int i, slot = -1;
                for (i = XMIDI_FOR_NEST - 1; i >= 0; --i) {
                    if (loop_count[i] != -1) {
                        slot = i;
                        break;
                    }
                }
                if (slot >= 0 && val >= 64) {
                    if (loop_count[slot] == 0) {
                        infinite = true;
                        end = true;
                    } else if (--loop_count[slot] > 0) {
                        pos = loop_pos[slot];
                    } else {
                        loop_count[slot] = -1;
                    }
                }
            } else if (s->mt32) {
                uint8_t phys = xmidi_channel_out(s, ch);
                int ci = xmidi_mt32_ctrl_index(cc);
                if (ci >= 0) {
                    s->global_ctrl[ci][phys] = (cc == 7) ? xmidi_mt32_scale_volume(s, val) : val;
                }
                if (cc == 7) {
                    s->chan_vol[ch] = val;
                }
                if (cc == 110) {
                    xmidi_mt32_lock_chan(s, out, notes, tick, ch, val);
                } else if (cc == 111) {
                    s->chan_protected[ch] = (val >= 64);
                } else if (cc == 112 || s->phys_locked[phys]) {
                } else if (cc == 114) {
                    s->phys_bank[phys] = val;
                } else if (cc >= 32 && cc <= 46) {
                    xmidi_mt32_sysex_ctrl(s, out, tick, cc, val);
                } else if (cc == 58) {
                    if (val >= 24 && s->phys_timbre[phys] != 0xff) {
                        uint8_t addr[3] = { 0x03, 0x01, 0x10 };
                        uint8_t data = s->phys_timbre[phys];
                        xmidi_sysex_addr_add(addr, (uint32_t)(val - 24) * 4u);
                        xmidi_mt32_sysex_push(out, tick, addr, &data, 1);
                    }
                } else if (cc == 59 || cc == 60) {
                    if (s->phys_patch[phys] != 0xff) {
                        uint8_t addr[3] = { 0x05, 0x00, 0x00 };
                        xmidi_sysex_addr_add(addr, (uint32_t)s->phys_patch[phys] * 8u + (cc == 59 ? 6u : 4u));
                        xmidi_mt32_sysex_push(out, tick, addr, &val, 1);
                        xmidi_midi_push(out, s, tick, (uint8_t)(0xc0 | ch), s->phys_patch[phys], 0, 2);
                    }
                } else if (cc >= 61 && cc <= 63) {
                    uint8_t addr[3] = { 0x10, 0x00, (uint8_t)(cc - 60) };
                    xmidi_mt32_sysex_push(out, tick, addr, &val, 1);
                } else if (cc == 113) {
                    int idx = s->phys_timbre[phys];
                    if (idx != 0xff) {
                        if (val >= 64) {
                            s->timb_attribs[idx] |= 0x40;
                        } else {
                            s->timb_attribs[idx] &= (uint8_t)~0x40;
                        }
                    }
                } else if (cc >= 118 && cc <= 120) {
                } else if (cc == 7) {
                    uint8_t v = xmidi_mt32_scale_volume(s, val);
                    xmidi_midi_push(out, s, tick, st, cc, v, 3);
                } else {
                    xmidi_midi_push(out, s, tick, st, cc, val, 3);
                }
            } else if (!((cc >= 32 && cc <= 46) || (cc >= 58 && cc <= 63) || (cc >= 110 && cc <= 120))) {
                xmidi_midi_push(out, s, tick, st, cc, val, 3);
            }
            break;
        }
        case 0xc0: {
            uint8_t patch;
            uint8_t ch = (uint8_t)(st & 0x0f);
            if (pos >= form.evnt_len) { end = true; break; }
            patch = form.evnt[pos++];
            if (s->mt32) {
                uint8_t phys = xmidi_channel_out(s, ch);
                if (s->phys_locked[ch] || s->phys_locked[phys]) {
                    s->global_program[phys] = patch;
                } else {
                    xmidi_mt32_apply_program(s, out, tick, ch, phys, patch, true);
                }
            } else {
                xmidi_midi_push(out, s, tick, st, patch, 0, 2);
            }
            break;
        }
        case 0xd0: {
            uint8_t d1;
            if (pos >= form.evnt_len) { end = true; break; }
            d1 = form.evnt[pos++];
            xmidi_midi_push(out, s, tick, st, d1, 0, 2);
            break;
        }
        default:
            end = true;
            break;
        }
    }

    xmidi_sysex_flush(out, tick, &sysex, &sysex_len, &sysex_cap, &sysex_active);
    if (!end) {
        uint8_t msg[3] = { 0xff, 0x2f, 0x00 };
        flush_all_noteoffs(out, s, notes, tick);
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
    if (ea->seq < eb->seq) return -1;
    if (ea->seq > eb->seq) return 1;
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
    xmidi_seq_state_t seq;
    uint8_t *smf = NULL;
    uint32_t cap, pos, i;
    uint64_t last = 0;

    if (data_out) *data_out = NULL;
    if (len_out) *len_out = 0;
    if (tune_loops) *tune_loops = false;

    xmidi_seq_state_init(&seq, false);
    if (!xmidi_sequence(&seq, data_in, len_in, &evs, tune_loops)) {
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

static bool xmidi_mt32_roms_ready(mt32emu_context ctx)
{
    static const uint32_t max_machines = 16;
    static const uint32_t max_rom_ids = 64;
    const char *machine_ids[16];
    const char *rom_ids[64];
    mt32emu_rom_info info;
    size_t nmachines;
    size_t i, j;

    memset(&info, 0, sizeof(info));
    mt32emu_get_rom_info(ctx, &info);
    if (!info.control_rom_id || !info.pcm_rom_id) {
        return false;
    }
    nmachines = mt32emu_get_machine_ids(machine_ids, max_machines);
    for (i = 0; i < nmachines; ++i) {
        size_t nroms = mt32emu_get_rom_ids(rom_ids, max_rom_ids, machine_ids[i]);
        bool have_control = false;
        bool have_pcm = false;
        for (j = 0; j < nroms; ++j) {
            if (strcmp(rom_ids[j], info.control_rom_id) == 0) {
                have_control = true;
            }
            if (strcmp(rom_ids[j], info.pcm_rom_id) == 0) {
                have_pcm = true;
            }
        }
        if (have_control && have_pcm) {
            return true;
        }
    }
    return false;
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
        if (xmidi_mt32_roms_ready(ctx)) {
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
    if (xmidi_mt32_roms_ready(ctx)) {
        return true;
    }

    env_path = getenv("MT32EMU_ROM_PATH");
    if (env_path && *env_path) {
        xmidi_mt32_try_add_roms_in_dir(ctx, env_path);
        if (xmidi_mt32_roms_ready(ctx)) {
            return true;
        }
    }

    paths = os_get_paths_data();
    if (paths) {
        for (i = 0; paths[i] != NULL; ++i) {
            xmidi_mt32_try_add_roms_in_dir(ctx, paths[i]);
            if (xmidi_mt32_roms_ready(ctx)) {
                return true;
            }
        }
    }

    if (!xmidi_mt32_roms_ready(ctx)) {
        mt32emu_rom_info info;
        memset(&info, 0, sizeof(info));
        mt32emu_get_rom_info(ctx, &info);
        log_error("XMID/MT32: incompatible or missing ROMs (control=%s, pcm=%s)\n",
                  info.control_rom_id ? info.control_rom_id : "none",
                  info.pcm_rom_id ? info.pcm_rom_id : "none");
        return false;
    }
    return true;
}

bool xmidi_render_mt32_wav(const uint8_t *data_in, uint32_t len_in, uint32_t sample_rate,
                           const char *rom_control, const char *rom_pcm,
                           uint8_t **wav_out, uint32_t *wav_len, bool *tune_loops)
{
    xmidi_mevents_t evs = { 0 };
    xmidi_seq_state_t seq;
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
    xmidi_seq_state_init(&seq, true);
    xmidi_mt32_init_gtl();
    xmidi_mt32_init(&evs);
    xmidi_mt32_load_timbres(&seq, &evs, data_in, len_in);
    if (!xmidi_sequence(&seq, data_in, len_in, &evs, tune_loops)) {
        mevents_free(&evs);
        return false;
    }
    qsort(evs.v, evs.n, sizeof(evs.v[0]), ev_cmp);

    {
        mt32emu_report_handler_i rh;
        memset(&rh, 0, sizeof(rh));
        ctx = mt32emu_create_context(rh, NULL);
    }
    if (!ctx) goto done;
    if (!xmidi_mt32_load_roms(ctx, rom_control, rom_pcm)) {
        log_error("XMID/MT32: control/PCM ROMs not found\n");
        goto done;
    }
    mt32emu_set_analog_output_mode(ctx, mt32emu_get_best_analog_output_mode((double)sample_rate));
    mt32emu_set_stereo_output_samplerate(ctx, (double)sample_rate);
    {
        mt32emu_return_code rc = mt32emu_open_synth(ctx);
        if (rc != MT32EMU_RC_OK) {
            mt32emu_rom_info info;
            memset(&info, 0, sizeof(info));
            mt32emu_get_rom_info(ctx, &info);
            log_error("XMID/MT32: mt32emu_open_synth() failed (rc=%d, control=%s, pcm=%s)\n",
                      (int)rc,
                      info.control_rom_id ? info.control_rom_id : "none",
                      info.pcm_rom_id ? info.pcm_rom_id : "none");
            goto done;
        }
    }

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
            if (evs.v[i].sysex && dl > 0) {
                mt32emu_play_sysex(ctx, (const mt32emu_bit8u *)d, (mt32emu_bit32u)dl);
            } else if (dl >= 1 && dl <= 3 && (d[0] & 0x80) && d[0] < 0xf0) {
                uint32_t msg = d[0] | ((uint32_t)(dl > 1 ? d[1] : 0) << 8) | ((uint32_t)(dl > 2 ? d[2] : 0) << 16);
                mt32emu_play_msg(ctx, msg);
            }
            ++i;
        }
    }
    /* tail for release; looped WAVs must not repeat a trailing gap */
    {
        uint32_t tail = (tune_loops && *tune_loops) ? 0 : sample_rate; /* 1s */
        if (tail > 0 && frames + tail > cap_frames) {
            int16_t *np = (int16_t *)realloc(pcm, (size_t)(frames + tail) * 2 * sizeof(int16_t));
            if (!np) goto done;
            pcm = np;
            cap_frames = frames + tail;
        }
        if (tail > 0) {
            mt32emu_render_bit16s(ctx, (mt32emu_bit16s *)(pcm + frames * 2), (mt32emu_bit32u)tail);
            frames += tail;
        }
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
