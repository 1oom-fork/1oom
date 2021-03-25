#include "config.h"

#include <string.h>

#include "lbxgfx.h"
#include "comp.h"
#include "hw.h"
#include "gfxscale.h"
#include "lbxpal.h"
#include "lib.h"
#include "log.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

struct thing_s;
typedef struct thing_s thing_t;
typedef void (*putpixel_func_t)(thing_t *, int);

/* RLE decompressor state and parameters */
struct thing_s {
    /* these get set once and then used */
    const uint8_t *src_data;
    uint8_t *dst; /* points to start of output column */
    int32_t pitch, scale; /* scale = scale */
    int32_t yskip; /* used for skipping first N pixels from each input column */
    int32_t ylimit; /* use at most this many input rows */
    int32_t xlimit; /* use at most this many input columns */
    putpixel_func_t put; /* this avoids a lot of conditionals in the hot innermost loop */

    /* loop variables */
    int32_t yskip_wr; /* how many pixels to skip from input (counts down) */
    uint8_t *dst_wr; /* write pixel here */
    uint8_t *dst_stop;
};

#define column_end_reached(_thing_) ((_thing_)->dst_wr >= (_thing_)->dst_stop)

static void begin_column(thing_t *buf)
{
    buf->dst_wr = buf->dst;
    buf->dst_stop = buf->dst_wr + buf->ylimit * buf->pitch * buf->scale;
    buf->dst += buf->scale; /* advance horizontally to next column */
    buf->yskip_wr = buf->yskip;
}

static int input_seek(thing_t *buf, const uint8_t *p[1], int count)
{
    if (buf->yskip_wr >= count) {
        buf->yskip_wr -= count;
        count = 0;
    } else {
        if (p) *p += buf->yskip_wr;
        count -= buf->yskip_wr;
        buf->yskip_wr = 0;
    }
    return count;
}

static inline void putpixel_fmt0_(thing_t *buf, int value, int scale)
{
    buf->dst_wr = gfxscale_draw_pixel(buf->dst_wr, value, buf->pitch, scale);
}

static inline void putpixel_fmt1_(thing_t *buf, int value, int scale)
{
    if (value >= 0xe8) {
        const uint8_t *tbl = lbxpal_colortable[value - 0xe8];
        buf->dst_wr = gfxscale_draw_pixel_fmt1(buf->dst_wr, tbl, buf->pitch, scale);
    } else {
        buf->dst_wr = gfxscale_draw_pixel(buf->dst_wr, value, buf->pitch, scale);
    }
}

/* -------------------------------------------------------------------------- */
/* Here we instantiate the poor man's templates
 * The compiler should inline and unroll the loops when scale is constant */

static void putpixel_fmt0_scale1(thing_t *b, int i) { putpixel_fmt0_(b, i, 1); }
static void putpixel_fmt0_scale2(thing_t *b, int i) { putpixel_fmt0_(b, i, 2); }
static void putpixel_fmt0_scale3(thing_t *b, int i) { putpixel_fmt0_(b, i, 3); }
static void putpixel_fmt0_scale4(thing_t *b, int i) { putpixel_fmt0_(b, i, 4); }
static void putpixel_fmt0_scaleN(thing_t *b, int i) { putpixel_fmt0_(b, i, b->scale); }

static void putpixel_fmt1_scale1(thing_t *b, int i) { putpixel_fmt1_(b, i, 1); }
static void putpixel_fmt1_scale2(thing_t *b, int i) { putpixel_fmt1_(b, i, 2); }
static void putpixel_fmt1_scaleN(thing_t *b, int i) { putpixel_fmt1_(b, i, b->scale); }

static putpixel_func_t choose_putpixel(int format, int scale)
{
    switch(format) {
        case 0:
            switch(scale) {
                case 1: return putpixel_fmt0_scale1;
                case 2: return putpixel_fmt0_scale2;
                case 3: return putpixel_fmt0_scale3;
                case 4: return putpixel_fmt0_scale4;
                default: return putpixel_fmt0_scaleN;
            }
        case 1: switch(scale) {
                case 1: return putpixel_fmt1_scale1;
                case 2: return putpixel_fmt1_scale2;
                default: return putpixel_fmt1_scaleN;
            }
        default:
            log_fatal_and_die("LBXGFX: unimplemented format=%d scale=%d\n", format, scale);
            return NULL;
    }
}

/* -------------------------------------------------------------------------- */

static void copy_pixels(thing_t *buf, const uint8_t *src, int count)
{
    count = input_seek(buf, &src, count);
    /* More performance can be had by moving this loop into the callback but it cost many LOC */
    for(int i=0; !column_end_reached(buf) && i<count; ++i) {
        buf->put(buf, src[i]);
    }
}

static void fill_pixels(thing_t *buf, uint8_t value, int count)
{
    count = input_seek(buf, NULL, count);
    for(int i=0; !column_end_reached(buf) && i<count; ++i) {
        buf->put(buf, value);
    }
}

static void skip_output_gap(thing_t *buf, int gap)
{
    gap = input_seek(buf, NULL, gap);
    buf->dst_wr += gap * buf->pitch * buf->scale;
}

static const uint8_t *do_xskip(const uint8_t *data, int xskip)
{
    /* skip columns in rle stream */
    while (xskip-- > 0) {
        uint8_t b = *data++;
        if (b != 0xff) {
            b = *data++;
            data += b;
        }
    }
    return data;
}

static thing_t lbxgfx_params(const uint8_t *src_data, int format, uint8_t *dst_data, int32_t pitch, int scale, int w, int h, int xskip, int yskip)
{
    thing_t d;
    d.dst = dst_data;
    d.put = choose_putpixel(format, scale);
    d.src_data = do_xskip(src_data, xskip);
    d.yskip = yskip;
    d.pitch = pitch;
    d.scale = scale;
    d.xlimit = w;
    d.ylimit = h;
    return d;
}

/* -------------------------------------------------------------------------- */

static void lbxgfx_rle_draw(thing_t state)
{
    const uint8_t *data = state.src_data;
    int mode, len_total, len_run, gap;

    while (state.xlimit-- > 0) {

        begin_column(&state);

        mode = *data++;
        if (mode == 0xff) { /* skip column */
            continue;
        }

        len_total = *data++;
        if ((mode & 0x80) == 0) { /* regular data */
            do {
                len_run = *data++;
                gap = *data++;
                if (len_run == 0) {
                    len_run = 256;
                }
                skip_output_gap(&state, gap);
                copy_pixels(&state, data, len_run);
                data += len_run;
                len_total -= len_run + 2;
            } while (len_total >= 1);
        } else {    /* compressed data */
            do {
                len_run = *data++;
                gap = *data++;
                skip_output_gap(&state, gap);
                len_total -= len_run + 2;
                do {
                    uint8_t b = *data++;
                    if (b > 0xdf) { /* b-0xdf pixels, same color */
                        --len_run;
                        fill_pixels(&state, *data++, b - 0xdf);
                    } else {
                        fill_pixels(&state, b, 1);
                    }
                } while (--len_run);
            } while (len_total >= 1);
        }
    }
}

/* -------------------------------------------------------------------------- */

void lbxgfx_draw_frame_do(uint8_t *dst_p, uint8_t *data, uint16_t pitch, int scale)
{
    uint16_t frame, next_frame, w, h;
    uint8_t *frameptr;
    int format;

    w = lbxgfx_get_w(data);
    h = lbxgfx_get_h(data);
    frame = lbxgfx_get_frame(data);
    frameptr = lbxgfx_get_frameptr(data, frame) + 1;
    format = lbxgfx_get_format(data);

    lbxgfx_rle_draw(lbxgfx_params(frameptr, format, dst_p, pitch, scale, w, h, 0, 0));

    next_frame = frame + 1;
    lbxgfx_set_frame(data, next_frame);
    if (next_frame >= lbxgfx_get_frames(data)) {
        next_frame = lbxgfx_get_frames2(data);
    }
    lbxgfx_set_frame(data, next_frame);
}

void lbxgfx_draw_frame(int x, int y, uint8_t *data, uint16_t pitch, int scale)
{
    uint8_t *p = hw_video_get_buf() + (y * pitch + x) * scale;
    lbxgfx_draw_frame_do(p, data, pitch, scale);
}

void lbxgfx_draw_frame_pal(int x, int y, uint8_t *data, uint16_t pitch, int scale)
{
    uint8_t *p;
    uint16_t frame;
    frame = lbxgfx_get_frame(data);
    if (frame == 0) {
        lbxgfx_apply_palette(data);
    }
    p = hw_video_get_buf() + (y * pitch + x) * scale;
    lbxgfx_draw_frame_do(p, data, pitch, scale);
}

void lbxgfx_draw_frame_offs(int x, int y, uint8_t *data, int lx0, int ly0, int lx1, int ly1, uint16_t pitch, int scale)
{
    int xskip, yskip, x0, y0, x1, y1, w, h;

    if ((x > lx1) || (y > ly1)) {
        return;
    }
    x1 = x + lbxgfx_get_w(data) - 1;
    if (x1 < lx0) {
        return;
    }
    y1 = y + lbxgfx_get_h(data) - 1;
    if (y1 < ly0) {
        return;
    }
    if (x >= lx0) {
        xskip = 0;
        x0 = x;
    } else {
        xskip = lx0 - x;
        x0 = lx0;
    }
    if (y >= ly0) {
        yskip = 0;
        y0 = y;
    } else {
        yskip = ly0 - y;
        y0 = ly0;
    }
    w = ((x1 < lx1) ? x1 : lx1) - x0 + 1;
    h = ((y1 < ly1) ? y1 : ly1) - y0 + 1;

    if (lbxgfx_get_epage(data) == 0) {
        lbxgfx_set_epage(data, 1);
        lbxgfx_apply_palette(data);
    }

    uint16_t frame = lbxgfx_get_frame(data);
    int format = lbxgfx_get_format(data);
    const uint8_t *frameptr = lbxgfx_get_frameptr(data, frame) + 1;
    uint8_t *dst_p = hw_video_get_buf() + (y0 * pitch + x0) * scale;
    lbxgfx_rle_draw(lbxgfx_params(frameptr, format, dst_p, pitch, scale, w, h, xskip, yskip));

    ++frame;
    if (frame >= lbxgfx_get_frames(data)) {
        frame = lbxgfx_get_frames2(data);
    }
    lbxgfx_set_frame(data, frame);
}

void lbxgfx_set_new_frame(uint8_t *data, uint16_t newframe)
{
    uint16_t frames = lbxgfx_get_frames(data);
    if (newframe >= frames) {
        uint16_t frames2 = lbxgfx_get_frames2(data);
        newframe = frames2 + (newframe - frames) % (frames - frames2);
    }
    lbxgfx_set_frame(data, newframe);
}

void lbxgfx_apply_colortable(int x0, int y0, int x1, int y1, uint8_t ctbli, uint16_t pitch, int scale)
{
    uint8_t *pixbuf = hw_video_get_buf();
    const uint8_t *tbl = lbxpal_colortable[ctbli];
    x0 *= scale;
    y0 *= scale;
    x1 = x1 * scale + scale - 1;
    y1 = y1 * scale + scale - 1;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            pixbuf[y * pitch + x] = tbl[pixbuf[y * pitch + x]];
        }
    }
}

void lbxgfx_apply_palette(uint8_t *data)
{
    if (lbxgfx_has_palette(data)) {
        uint8_t *p = lbxgfx_get_palptr(data);
        int first = lbxgfx_get_palfirst(data);
        int num = lbxgfx_get_palnum(data);
        lbxpal_set_palette(p, first, num);
    }
}
