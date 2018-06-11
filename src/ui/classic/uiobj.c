#include "config.h"

#include <string.h>

#include "uiobj.h"
#include "comp.h"
#include "game.h"   /* only for UIOBJ_MAX */
#include "hw.h"
#include "kbd.h"
#include "lbxfont.h"
#include "lbxgfx.h"
#include "log.h"
#include "mouse.h"
#include "types.h"
#include "uicursor.h"
#include "uidefs.h"
#include "uidelay.h"
#include "uidraw.h"
#include "uihelp.h"
#include "uipal.h"

/* -------------------------------------------------------------------------- */

/*
    WASBUG? MOO1 allocs 0x1650 bytes fitting 150 which is not always enough.
    Absolute max is planets + orbits + fleets + transports + some buttons.
    This is overkill for actual gameplay but we are not limited by 1993 memory sizes.
*/
#define UIOBJ_MAX   (PLANETS_MAX * 7 + FLEET_ENROUTE_MAX + TRANSPORT_MAX + 32)

#define UIOBJ_OFFSCREEN (((UI_SCREEN_W > UI_SCREEN_H) ? UI_SCREEN_W : UI_SCREEN_H) + 100)

typedef struct uiobj_s {
    /*00*/ uint16_t x0;
    /*02*/ uint16_t y0;
    /*04*/ uint16_t x1;    /* inclusive */
    /*06*/ uint16_t y1;    /* inclusive */
    /* 12 types
        t0..3: buttons with gfx (and optional text) with different highlight conditions
        t1: toggle
        t2: set1
        t3: setval
        t4: textinput
        t6: slider
        t7: mousearea or inputkey
        t8: altstr
        ta: text line
        tb: scroll area
    */
    /*08*/ uint16_t type;
    /*0a*/ int16_t helpid;
    /*1a*/ int16_t *vptr;
    /*24*/ uint32_t key;
    union {
        struct {
            /*0c*/ uint16_t fontnum; /* t1..3? */
            /*0e*/ uint16_t fonta2; /* t1..3? */
            /*16*/ const char *str;
            /*18*/ int16_t z18; /* t3:uiobji? */
            /*1c*/ bool indep;  /* from lbxgfx offs 0x10 */
            /*20*/ uint8_t *lbxdata;
        } t0; /* t{0..3} */
        struct {
            /*0c*/ uint16_t fontnum;
            /*0e*/ uint16_t fonta2;
            /*14*/ uint16_t fonta4;
            /*16*/ char *buf;
            /*1a*/ uint8_t rectcolor;
            /*1c*/ bool align_right;
            /*1e*/ uint16_t z1e;    /* bool? */
            /*20*/ uint16_t buflen;
            /*22*/ const uint8_t *colortbl;
        } t4;
        struct {
            /*16*/ uint16_t fmin;
            /*18*/ uint16_t fmax;
            /*1c*/ uint16_t vmin;
            /*1e*/ uint16_t vmax;
            /*20*/ bool vertical;
        } t6;
        struct {
            /*16*/ const char *str;
            /*18*/ uint16_t pos;
            /*1a*/ uint16_t len;
        } t8;
        struct {
            /*16*/ uint16_t uiobji;
            /*18*/ int16_t z18;
        } t9;
        struct {
            /*0c*/ uint16_t fontnum;
            /*0e*/ uint16_t fonta2;
            /*10*/ uint16_t subtype;
            /*12*/ bool z12;
            /*14*/ uint16_t fonta2b;
            /*16*/ const char *str;
            /*18*/ int16_t z18;
            /*1c..22 are subtype specific parameters */
            /*1c: used as offs and value, determined by "z < 0x100"! adding extra var here for sanity */
            /*1c*/ uint8_t *sp0p;
            /*1c*/ uint16_t sp0v;
            /*1e*/ uint16_t sp1;
            /*20*/ uint16_t sp2;
            /*22*/ uint16_t sp3;
        } ta;
        struct {
            /*18*/ uint16_t xdiv;
            /*1a*/ uint16_t ydiv;
            /*1c*/ uint16_t *xptr;
            /*1e*/ uint16_t *yptr;
        } tb;
    };
} uiobj_t;

/* -------------------------------------------------------------------------- */

static uint16_t uiobj_table_num = 0;
static uint16_t uiobj_table_num_old = 0;
static int16_t uiobj_hmm1_oi = -1;
static int16_t uiobj_hmm2_oi = 0;
static int uiobj_hmm3_xoff = 1;
static int uiobj_hmm3_yoff = -1;
static uint16_t uiobj_hmm3_fonta4 = 0;
static int16_t uiobj_mouseoff = 0;
static int16_t uiobj_handle_downcount = 0;
static uint16_t uiobj_kbd_hmm1 = 0;
static uint16_t uiobj_hmm5_delay = 2;
static uint16_t uiobj_hmm6 = 0;
static int16_t uiobj_help_id = -1;
static int16_t uiobj_hmm8 = 1;
static bool uiobj_hmm9 = false;
static int16_t uiobj_kbd_movey = -1;
static bool uiobj_flag_skip_delay = false;

static bool uiobj_flag_have_cb = false;
static void (*uiobj_callback)(void *) = NULL;
static void *uiobj_cbdata = NULL;

static uiobj_t uiobj_tbl[UIOBJ_MAX];

/* -------------------------------------------------------------------------- */

int uiobj_minx = 0;
int uiobj_miny = 0;
int uiobj_maxx = UI_SCREEN_W - 1;
int uiobj_maxy = UI_SCREEN_H - 1;

/* -------------------------------------------------------------------------- */

#define UIOBJI_ALLOC()  uiobj_table_num++

static inline int16_t hmmdiv2(int16_t v)
{
    --v;
    if (v < 0) { ++v; }
    return v / 2;
}

static int smidx(const uiobj_t *p)
{
    return p->x0 + (p->x1 - p->x0) / 2;
}

static int smidy(const uiobj_t *p)
{
    return p->y0 + (p->y1 - p->y0) / 2;
}

static int smidyhmm2(const uiobj_t *p)
{
    return smidy(p) - hmmdiv2(lbxfont_get_height());
}

static inline bool uiobj_is_at_xy(const uiobj_t *p, int x, int y)
{
    x += uiobj_mouseoff;
    y += uiobj_mouseoff;
    if ((x < p->x0) || (x > p->x1) || (y < p->y0) || (y > p->y1)) {
        return false;
    }
    return true;
}

static void uiobj_handle_t03_cond(uiobj_t *p, bool cond)
{
    if (cond) {
        lbxgfx_set_frame_0(p->t0.lbxdata);
        lbxgfx_draw_frame(p->x0, p->y0, p->t0.lbxdata, UI_SCREEN_W);
        lbxfont_select(p->t0.fontnum, p->t0.fonta2, 0, 0);
        lbxfont_print_str_center(smidx(p), smidyhmm2(p), p->t0.str, UI_SCREEN_W);
    } else {
        if (p->t0.indep == 0) {
            lbxgfx_set_frame_0(p->t0.lbxdata);
            lbxgfx_draw_frame(p->x0, p->y0, p->t0.lbxdata, UI_SCREEN_W);
        } else {
            lbxgfx_set_new_frame(p->t0.lbxdata, 1);
        }
        lbxgfx_draw_frame(p->x0, p->y0, p->t0.lbxdata, UI_SCREEN_W);
        lbxfont_select(p->t0.fontnum, p->t0.fonta2, 0, 0);
        lbxfont_print_str_center(smidx(p) + uiobj_hmm3_xoff, smidyhmm2(p) + uiobj_hmm3_yoff, p->t0.str, UI_SCREEN_W);
    }
}

static void uiobj_handle_t4_sub2(uiobj_t *p, uint16_t len, uint16_t a4, const char *str)
{
    char strbuf[64];
    int16_t si, va;
    si = a4;
    ui_delay_prepare();
    strcpy(strbuf, str);
    uiobj_do_callback();
    /*ve = p->x1 - p->x0;*/
    lbxfont_select(p->t4.fontnum, p->t4.fonta2, p->t4.fonta4, 0);
    va = lbxfont_get_height() - 1;
    if (p->t4.rectcolor != 0) {
        ui_draw_filled_rect(p->x0, p->y0, p->x1, p->y1, p->t4.rectcolor);
    }
    if (p->t4.z1e == 0) {
        uint16_t l, w, x, vc;
        char c;
        c = strbuf[len];
        strbuf[len] = 0;
        x = lbxfont_calc_str_width(strbuf) + p->x0;
        strbuf[len] = c;
        w = lbxfont_get_char_w(c ? c : ' ');
        if ((si > 0) && (si <= va)) {
            vc = p->y0 + va;
            l = 0;
            while (si > 0) {
                ui_draw_line1(x, vc - si + 1, x + w + 1, vc - si + 1, p->t4.colortbl[l]);
                ++l;
                --si;
            }
        } else if (si != 0) {
            si = va - (si - va);
            l = 0;
            while (si > 0) {
                ui_draw_line1(x, p->y0 + si - 1, x + w + 1, p->y0 + si - 1, p->t4.colortbl[va - l - 1]);
                ++l;
                --si;
            }
        }
    }
    lbxfont_select_subcolors_13not1();
    lbxfont_print_str_normal(p->x0, p->y0, strbuf, UI_SCREEN_W);
    ui_palette_set_n();
    uiobj_finish_frame();
    ui_delay_ticks_or_click(uiobj_hmm5_delay);
}

static void uiobj_handle_t4_sub1(uiobj_t *p)
{
    uint16_t len, pos, buflen, w, fonth, animpos;
    mookey_t key = MOO_KEY_UNKNOWN;
    bool flag_mouse_button = false, flag_got_first = false;
    char strbuf[64];

    while (mouse_buttons) {
        hw_event_handle();
        uiobj_do_callback();
    }

    animpos = 0;
    buflen = p->t4.buflen;
    w = p->x1 - p->x0;
    lbxfont_select(p->t4.fontnum, p->t4.fonta2, p->t4.fonta4, 0);
    strcpy(strbuf, p->t4.buf);
    len = strlen(strbuf);
    if (lbxfont_calc_str_width(strbuf) > w) {
        if (len != 0) {
            len = 0;
            strbuf[len] = '\0';
        }
    }
    pos = len;
    if (pos >= buflen) {
        pos = buflen;
    }
    strcpy(p->t4.buf, strbuf);
    fonth = lbxfont_get_height();
    uiobj_handle_t4_sub2(p, pos, animpos, strbuf);
    while ((key != MOO_KEY_RETURN) && (!flag_mouse_button)) {
        bool flag_ok;
        while (!(kbd_have_keypress() || flag_mouse_button)) {
            hw_event_handle();
            if ((1/*mouse_flag_initialized*/) && (mouse_buttons || (mouse_getclear_hmm4() != 0))) {
                flag_mouse_button = true;
                break;
            } else {
                ++animpos;
                if (((fonth << 1) - 1) < animpos) {
                    animpos = 0;
                }
                uiobj_handle_t4_sub2(p, pos, animpos, strbuf);
            }
        }
        if (flag_mouse_button) {
            break;
        }
        key = KBD_GET_KEY(kbd_get_keypress());
        switch (key) {
            case MOO_KEY_BACKSPACE:
                if (!flag_got_first) {
                    strbuf[0] = '\0';
                    len = 0;
                    pos = 0;
                    animpos = 0;
                    flag_got_first = true;
                } else {
                    if (len > 0) {
                        if (pos >= len) {
                            --len;
                            strbuf[len] = '\0';
                            --pos;
                            animpos = 0;
                        } else if (pos > 0) {
                            for (int i = pos; i < len; ++i) {
                                strbuf[i - 1] = strbuf[i];
                            }
                            --len;
                            --pos;
                        }
                    }
                }
                animpos = 0;
                strbuf[len] = '\0';
                break;
            case MOO_KEY_DELETE:
                if ((len > 0) && (pos < len)) {
                    for (int i = pos; i < len; ++i) {
                        strbuf[i] = strbuf[i + 1];
                    }
                    --len;
                    animpos = 0;
                    strbuf[len] = '\0';
                }
                break;
            case MOO_KEY_LEFT:
                flag_got_first = true;
                if (pos > 0) {
                    --pos;
                    animpos = 0;
                }
                break;
            case MOO_KEY_RIGHT:
                if ((pos < buflen) && (pos < len)) {
                    ++pos;
                    animpos = 0;
                    if (pos >= len) {
                        strbuf[len] = ' ';
                        strbuf[len + 1] = '\0';
                        if ((pos >= buflen) || (lbxfont_calc_str_width(strbuf) > w)) {
                            --pos;
                        }
                        strbuf[len] = '\0';
                    }
                }
                break;
            default:
                flag_ok = false;
                if ((key >= MOO_KEY_a) && (key <= MOO_KEY_z)) {
                    key -= 0x20;    /* az -> AZ */
                }
                if (0
                  || ((key >= 'A') && (key < ']'))
                  || ((key >= '.') && (key < ';'))
                  || (key == ' ') || (key == '-')
                ) {
                    flag_ok = true;
                }
                if (flag_ok) {
                    flag_got_first = true;
                    strbuf[len] = key;
                    strbuf[len + 1] = '\0';
                    if ((len < buflen) && (lbxfont_calc_str_width(strbuf) <= w)) {
                        strbuf[len] = '\0';
                        if (pos < len) {
                            for (int i = len; i > pos; --i) {
                                strbuf[i] = strbuf[i - 1];
                            }
                            ++len;
                            strbuf[pos] = key;
                            ++pos;
                        } else {
                            strbuf[len] = key;
                            ++len;
                            strbuf[len] = ' ';
                            strbuf[len + 1] = '\0';
                            if ((len < buflen) && (lbxfont_calc_str_width(strbuf) <= w)) {
                                ++pos;
                            }
                        }
                        strbuf[len] = '\0';
                        animpos = 0;
                    } else {
                        strbuf[len] = '\0';
                    }
                }
                break;
        }
        uiobj_handle_t4_sub2(p, pos, animpos, strbuf);
    }
    strcpy(p->t4.buf, strbuf);
    if (flag_mouse_button) /*&& (mouse_flag_initialized)*/ {
        while (mouse_buttons) {
            hw_event_handle();
        }
        mouse_getclear_hmm4();
        mouse_getclear_hmm5();
    }
    /* TODO ui_cursor_erase0(); */
    uiobj_hmm1_oi = -1;
}

static void uiobj_handle_t6_slider_input(uiobj_t *p)
{
    uint16_t sliderval, slideroff, di;
    if (p->t6.vertical == false) {
        di = mouse_x + uiobj_mouseoff;
        slideroff = ((p->t6.vmax - p->t6.vmin) * (di - p->x0)) / (p->x1 - p->x0);
        if (p->x1 <= di) {
            sliderval = p->t6.vmax;
        } else if (p->x0 >= di) {
            sliderval = p->t6.vmin;
        } else {
            sliderval = p->t6.vmin + slideroff;
        }
    } else {
        di = mouse_y + uiobj_mouseoff;
        slideroff = ((p->t6.vmax - p->t6.vmin) * (p->y1 - di)) / (p->y1 - p->y0);
        if (p->y1 <= di) {
            sliderval = p->t6.vmin;
        } else if (p->y0 >= di) {
            sliderval = p->t6.vmax;
        } else {
            sliderval = slideroff; /* bug? */
        }
    }
    if (p->t6.fmin > sliderval) {
        sliderval = p->t6.fmin;
    }
    if (p->t6.fmax < sliderval) {
        sliderval = p->t6.fmax;
    }
    *p->vptr = sliderval;
}

static void uiobj_handle_ta_sub1(int x0, int y0, int x1, int y1, uint16_t subtype, uint8_t *p0p, uint16_t p0v, uint16_t p1, uint16_t p2, uint16_t p3)
{
    switch (subtype) {
        case 1:
            ui_draw_filled_rect(x0, y0, x1, y1, p0v);
            break;
        case 3:
            p0v = 0;
            /* fall through */
        case 0xf:
            lbxgfx_apply_colortable(x0, y0, x1, y1, p0v, UI_SCREEN_W);
            break;
        case 7:
            ui_draw_box_fill(x0, y0, x1, y1, p0p, p0v, p1, p2, p3);
            break;
        case 0xd:
            ui_draw_box_grain(x0, y0, x1, y1, p0v, p1, p3);
            break;
        default:
            break;
    }
}

/* not a function in MOO1 but part of uiobj_handle_hmm1 */
static inline void uiobj_handle_hmm1_sub1(int i)
{
    uiobj_t *p = &uiobj_tbl[i];
    switch (p->type) {
        case 0:
            uiobj_handle_t03_cond(p, true);
            break;
        case 1:
            uiobj_handle_t03_cond(p, *p->vptr == 0);
            break;
        case 2:
            uiobj_handle_t03_cond(p, *p->vptr == 0);
            break;
        case 3:
            uiobj_handle_t03_cond(p, *p->vptr != p->t0.z18);
            break;
        case 0xa:
            lbxfont_select(p->ta.fontnum, p->ta.fonta2, uiobj_hmm3_fonta4, 0);
            if (*p->vptr != p->ta.z18) {
                if (!p->ta.z12) {
                    /* ?? what is the point in this second call? */
                    lbxfont_select(p->ta.fontnum, p->ta.fonta2b, uiobj_hmm3_fonta4, 0);
                }
                /*19ca3*/
                lbxfont_print_str_normal(p->x0, p->y0 + 1, p->ta.str, UI_SCREEN_W);
            } else {
                int16_t gap_h, char_h;
                gap_h = lbxfont_get_gap_h();
                if (gap_h < 0) { ++gap_h; }
                gap_h /= 2;
                if (gap_h == 0) {
                    gap_h = 1;
                }
                char_h = lbxfont_get_height();
                /*19ce2*/
                uiobj_handle_ta_sub1(p->x0 - 1, p->y0 - gap_h + 1, p->x1, p->y0 + char_h + 1, p->ta.subtype, p->ta.sp0p, p->ta.sp0v, p->ta.sp1, p->ta.sp2, p->ta.sp3);
                lbxfont_print_str_normal(p->x0, p->y0 + 1, p->ta.str, UI_SCREEN_W);
            }
            break;
        case 4:
            if (uiobj_hmm1_oi != i) {
                lbxfont_select(p->t4.fontnum, p->t4.fonta2, p->t4.fonta4, 0);
                ui_draw_filled_rect(p->x0, p->y0, p->x1, p->y1, p->t4.rectcolor);
                if (!p->t4.align_right) {
                    lbxfont_print_str_normal(p->x0, p->y0, p->t4.buf, UI_SCREEN_W);
                } else {
                    lbxfont_print_str_right(p->x1, p->y0, p->t4.buf, UI_SCREEN_W);
                }
            }
            break;
        case 6:
            {
                uint16_t v = *p->vptr;
                if (p->t6.fmin > v) {
                    v = p->t6.fmin;
                }
                if (p->t6.fmax < v) {
                    v = p->t6.fmax;
                }
                *p->vptr = v;
            }
            break;
        default:
            break;
    }
}

static void uiobj_handle_hmm2(int i, uint16_t a2)
{
    uiobj_t *p = &uiobj_tbl[i];
    switch (p->type) {
        case 0:
            uiobj_handle_t03_cond(p, a2 == 0);
            break;
        case 1:
            uiobj_handle_t03_cond(p, (a2 == 0) || (*p->vptr == 1));
            break;
        case 2:
            uiobj_handle_t03_cond(p, (a2 == 0) && (*p->vptr == 0));
            break;
        case 3:
            if (a2 == 0) {
                *p->vptr = UIOBJI_INVALID; /* TODO or other 0xfc18? */
            } else {
                *p->vptr = p->t0.z18;
            }
            uiobj_handle_t03_cond(p, *p->vptr != p->t0.z18);
            break;
        case 0xa:
            if (a2 == 0) {
                *p->vptr = 0;
            } else if (p->ta.z12) {
                *p->vptr = p->ta.z18;
            }
            lbxfont_select(p->ta.fontnum, p->ta.fonta2, uiobj_hmm3_fonta4, 0);
            if (*p->vptr != p->ta.z18) {
                if (p->ta.z12) {
                    lbxfont_print_str_normal(p->x0, p->y0, p->ta.str, UI_SCREEN_W);
                } else {
                    lbxfont_select(p->ta.fontnum, p->ta.fonta2b, uiobj_hmm3_fonta4, 0);
                    lbxfont_print_str_normal(p->x0, p->y0 + 1, p->ta.str, UI_SCREEN_W);
                    lbxfont_select_subcolors_0();
                }
            } else {
                int16_t char_h, v4;
                v4 = lbxfont_get_gap_h();
                if (v4 < 0) { ++char_h; }
                v4 /= 2;
                if (v4 == 0) {
                    v4 = 1;
                }
                char_h = lbxfont_get_height();
                uiobj_handle_ta_sub1(p->x0 - 1, p->y0 - v4 + 1, p->x1, p->y0 + char_h + 1, p->ta.subtype, p->ta.sp0p, p->ta.sp0v, p->ta.sp1, p->ta.sp2, p->ta.sp3);
                lbxfont_print_str_normal(p->x0, p->y0 + 1, p->ta.str, UI_SCREEN_W);
            }
            break;
        case 0xb:
            if (a2 == 1) {
                *p->tb.xptr = (mouse_x - p->x0) / p->tb.xdiv;
                *p->tb.yptr = (mouse_y - p->y0) / p->tb.ydiv;
            }
            break;
        case 6:
            if (a2 == 1) {
                uiobj_handle_t6_slider_input(p);
            }
            break;
        case 9:
            if (a2 == 1) {
                *p->vptr = p->t9.z18;
            }
            break;
        case 4:
            uiobj_handle_t4_sub1(p);
            break;
        default:
            break;
    }
}

static int16_t uiobj_kbd_dir_key_dy(int diry)
{
    int16_t oi2 = uiobj_at_cursor();
    int16_t oi = oi2;
    uiobj_t *p;
    if (oi != 0) {
        if (diry == 1) {
            while ((++oi < (uiobj_table_num - 1)) && (uiobj_tbl[oi].type == 0xa)) {
                if (uiobj_tbl[oi].ta.z12) {
                    break;
                }
            }
            if (!((oi < (uiobj_table_num - 1)) && (uiobj_tbl[oi].type == 0xa))) {
                if (uiobj_hmm9) {
                    oi = oi2;
                    uiobj_kbd_movey = 1;
                } else {
                    oi = 0;
                    while (oi < uiobj_table_num) {
                        ++oi;
                        if (/*not tested in MOO1!*/(uiobj_tbl[oi].type == 0xa) && uiobj_tbl[oi].ta.z12) {
                            break;
                        }
                    }
                    if (oi >= uiobj_table_num) {
                        oi = oi2;
                    } else {
                        oi = oi2;
                        uiobj_kbd_movey = 1;
                    }
                }
            }
        } else {
            if (uiobj_hmm9 && (oi != 1)) {
                uiobj_kbd_movey = -1;
                oi = 1;
            } else {
                if (oi > 1) {
                    --oi;
                } else {
                    oi = uiobj_table_num - 1 - 1;
                }
                while (oi && (uiobj_tbl[oi].type != 0xa)) {
                    --oi;
                }
                while (oi && /*not tested in MOO1!*/(uiobj_tbl[oi].type == 0xa) && !uiobj_tbl[oi].ta.z12) {
                    --oi;
                }
                if (oi <= 0) {
                    if (uiobj_hmm9) {
                        uiobj_kbd_movey = -1;
                        oi = 1;
                    } else {
                        oi = uiobj_table_num - 1 - 1;
                        while (oi && /*not tested in MOO1!*/(uiobj_tbl[oi].type == 0xa) && uiobj_tbl[oi].ta.z12) {
                            --oi;
                        }
                        while (oi && /*not tested in MOO1!*/(uiobj_tbl[oi].type == 0xa) && !uiobj_tbl[oi].ta.z12) {
                            --oi;
                        }
                        if (oi == 0) {
                            oi = oi2;
                        }
                    }
                }
            }
        }
    } else {
        p = &uiobj_tbl[1];
        if (p->vptr && (*p->vptr >= 0)) {
            oi2 = *p->vptr + 1;
            if (oi2 >= uiobj_table_num) {
                oi2 = 0;
            }
            oi = oi2;
            if (diry == 1) {
                while ((++oi < (uiobj_table_num - 1)) && (uiobj_tbl[oi].type == 0xa)) {
                    if (uiobj_tbl[oi].ta.z12) {
                        break;
                    }
                }
                if (!((oi < uiobj_table_num) && (uiobj_tbl[oi].type == 0xa))) {
                    if (uiobj_hmm9) {
                        uiobj_kbd_movey = 1;
                    } else if (oi < uiobj_table_num) {
                        oi = 1;
                        while (oi && /*not tested in MOO1!*/(uiobj_tbl[oi].type == 0xa) && !uiobj_tbl[oi].ta.z12) {
                            ++oi;
                        }
                        if (oi >= uiobj_table_num) {
                            oi = oi2;
                        }
                    }
                }
            } else {
                if ((oi == 1) && uiobj_hmm9) {
                    uiobj_kbd_movey = -1;
                } else {
                    if (oi <= 1) {
                        oi = uiobj_table_num - 1 - 1;
                    } else {
                        --oi;
                    }
                    while (oi && /*not tested in MOO1!*/(uiobj_tbl[oi].type == 0xa) && !uiobj_tbl[oi].ta.z12) {
                        --oi;
                    }
                    if (oi == 0) {
                        oi = uiobj_table_num - 1 - 1;
                        while (oi && /*not tested in MOO1!*/(uiobj_tbl[oi].type == 0xa) && !uiobj_tbl[oi].ta.z12) {
                            --oi;
                        }
                        if (oi == 0) {
                            oi = oi2;
                        }
                    }
                }
            }
        } else {
            for (oi = 1; oi < uiobj_table_num; ++oi) {
                if (/*not tested in MOO1!*/(uiobj_tbl[oi].type == 0xa) && uiobj_tbl[oi].ta.z12) {
                    break;
                }
            }
            if (oi >= uiobj_table_num) {
                oi = 0;
            }
        }
    }
    if ((oi < 0) || (oi >= uiobj_table_num)) {
        oi = 0;
    }
    if (oi > 0) {
        p = &uiobj_tbl[oi];
        mouse_stored_x = smidx(p);
        mouse_stored_y = smidy(p);
        if ((mouse_x != mouse_stored_x) || (mouse_y != mouse_stored_y)) {
            ui_cursor_update_gfx_i(mouse_stored_x, mouse_stored_y);
            uiobj_mouseoff = ui_cursor_mouseoff;
            mouse_stored_x -= uiobj_mouseoff;
            mouse_stored_y -= uiobj_mouseoff;
            ui_cursor_erase0();
            ui_cursor_store_bg0(mouse_stored_x, mouse_stored_y);
            ui_cursor_draw0(mouse_stored_x, mouse_stored_y);
            /* TODO hw_redraw */
            mouse_set_xy(mouse_stored_x, mouse_stored_y);
            /* TODO *p->vprt = p->z18; */
        }
    }
    return oi;
}

static inline bool uiobj_kbd_dir_obj_ok(const uiobj_t *p)
{
    return ((p->type < 0xb) && (p->key == MOO_KEY_UNKNOWN) && ((p->type != 0xa) || p->ta.z12));
}

static int16_t uiobj_kbd_dir_key_dxdy(int dirx, int diry, int16_t oi2, int mx, int my)
{
    int dx = UIOBJ_OFFSCREEN, dy = UIOBJ_OFFSCREEN;
    int slope = UIOBJ_OFFSCREEN;
    int mind = UIOBJ_OFFSCREEN * 100;
    int dist;
    int16_t oi = oi2;
    uiobj_t *p;
    {
        bool flag_found = false;
        for (int i = 1; i < uiobj_table_num; ++i) {
            p = &uiobj_tbl[i];
            if (uiobj_kbd_dir_obj_ok(p)) {
                flag_found = true;
                break;
            }
        }
        if (!flag_found) {
            return UIOBJI_NONE;
        }
    }
    if ((diry != 0) && (dirx == 0)) {
        for (int i = 1; i < uiobj_table_num; ++i) {
            if (i == oi2) {
                if (i != (uiobj_table_num - 1)) {
                    ++i;
                } else {
                    break;
                }
            }
            p = &uiobj_tbl[i];
            if (uiobj_kbd_dir_obj_ok(p)) {
                dy = (diry < 0) ? (my - smidy(p)) : (smidy(p) - my);
                dx = smidx(p) - mx;
                if ((p->x0 <= mx) && (p->x1 >= mx) && ((dx < -6) || (dx > 6))) {
                    dx = 6;
                }
                if ((dx > -6) && (dx < 6) && (dy > 0) && (dy < mind)) {
                    mind = dy;
                    oi = i;
                }
            }
        }
        if (oi == oi2) {
            for (int i = 1; i < uiobj_table_num; ++i) {
                if (i == oi2) {
                    if (i != (uiobj_table_num - 1)) {
                        ++i;
                    } else {
                        break;
                    }
                }
                p = &uiobj_tbl[i];
                if (uiobj_kbd_dir_obj_ok(p)) {
                    dy = (diry < 0) ? (my - smidy(p)) : (smidy(p) - my);
                    dx = smidx(p) - mx;
                    if ((p->x0 <= mx) && (p->x1 >= mx) && ((dx < -6) || (dx > 6))) {
                        dx = 6;
                    }
                    if (dx < 0) {
                        dx = -dx;
                    }
                    if (dy < 0) {
                        dx = UIOBJ_OFFSCREEN;
                    }
                    if (dy == 0) {
                        dy = 1;
                    }
                    slope = (dx * 100) / dy;
                    if ((slope >= 0) && (slope < 0x2c)) {
                        dist = (dx * dx) + (dy * dy);
                        if (dist < mind) {
                            mind = dist;
                            oi = i;
                        }
                    }
                }
            }
        }
    }
    if ((dirx != 0) && (diry == 0)) {
        for (int i = 1; i < uiobj_table_num; ++i) {
            if (i == oi2) {
                if (i != (uiobj_table_num - 1)) {
                    ++i;
                } else {
                    break;
                }
            }
            p = &uiobj_tbl[i];
            if (uiobj_kbd_dir_obj_ok(p)) {
                dx = (dirx < 0) ? (mx - smidx(p)) : (smidx(p) - mx);
                dy = smidy(p) - my;
                if ((p->y0 <= my) && (p->y1 >= my) && ((dy < -6) || (dy > 6))) {
                    dy = 6;
                }
                if ((dy > -6) && (dy < 6) && (dx > 0) && (dx < mind)) {
                    mind = dx;
                    oi = i;
                }
            }
        }
        if (oi == oi2) {
            for (int i = 1; i < uiobj_table_num; ++i) {
                if (i == oi2) {
                    if (i != (uiobj_table_num - 1)) {
                        ++i;
                    } else {
                        break;
                    }
                }
                p = &uiobj_tbl[i];
                if (uiobj_kbd_dir_obj_ok(p)) {
                    dx = (dirx < 0) ? (mx - smidx(p)) : (smidx(p) - mx);
                    dy = smidy(p) - my;
                    if ((p->y0 <= my) && (p->y1 >= my) && ((dy < -6) || (dy > 6))) {
                        dy = 6;
                    }
                    if (dy < 0) {
                        dy = -dy;
                    }
                    if (dx < 0) {
                        dy = UIOBJ_OFFSCREEN;
                    }
                    if (dx == 0) {
                        dx = 1;
                    }
                    slope = (dy * 100) / dx;
                    if ((slope >= 0) && (slope < 0x2c)) {
                        dist = (dx * dx) + (dy * dy);
                        if (dist < mind) {
                            mind = dist;
                            oi = i;
                        }
                    }
                }
            }
        }
    }
    if ((dirx != 0) && (diry != 0)) {
        for (int i = 1; i < uiobj_table_num; ++i) {
            if (i == oi2) {
                if (i != (uiobj_table_num - 1)) {
                    ++i;
                } else {
                    break;
                }
            }
            p = &uiobj_tbl[i];
            if (uiobj_kbd_dir_obj_ok(p)) {
                dx = (dirx < 0) ? (mx - smidx(p)) : (smidx(p) - mx);
                dy = (diry < 0) ? (my - smidy(p)) : (smidy(p) - my);
                if ((dx < 0) || (dy < 0)) {
                    slope = UIOBJ_OFFSCREEN;
                    continue;
                }
                if ((dx >= dy) && (dy != 0)) {
                    slope = (dx * 100) / dy;
                }
                if ((dy > dx) && (dx != 0)) {
                    slope = (dy * 100) / dx;
                }
                if ((dx == 0) || (dy == 0)) {
                    slope = UIOBJ_OFFSCREEN;
                }
                if ((slope >= 34) && (slope <= 105)) {
                    dist = (dx * dx) + (dy * dy);
                    if (dist < mind) {
                        mind = dist;
                        oi = i;
                    }
                }
            }
        }
    }
    return oi;
}

static int16_t uiobj_kbd_dir_key(int dirx, int diry)
{
    if ((uiobj_hmm6 != 0) && (diry != 0)) {
        return uiobj_kbd_dir_key_dy(diry);
    } else {
        int mx, my;
        int16_t oi, oi2;
        uiobj_t *p;
        if (1/*mouse_initialized*/) {
            mx = mouse_x;
            my = mouse_y;
        }  else {
            mx = mouse_stored_x;
            my = mouse_stored_y;
        }
        oi2 = 0;
        oi = uiobj_table_num - 1;
        while (oi > 0) {
            p = &uiobj_tbl[oi];
            if (p->type < 0xb) {
                if (uiobj_is_at_xy(p, mx, my)) {
                    if (p->type == 0xa) {
                        if (p->ta.z12) {
                            oi2 = oi;
                        }
                    } else {
                        oi2 = oi;
                    }
                }
            }
            --oi;
        }
        /*15a7a*/
        oi = oi2;
        if ((dirx == 0) && (diry == 0)) {
            return oi;
        }
        oi = uiobj_kbd_dir_key_dxdy(dirx, diry, oi2, mx, my);
        if ((oi != oi2) && (oi != UIOBJI_NONE)) {
            uiobj_t *p = &uiobj_tbl[oi];
            mouse_stored_x = smidx(p);
            mouse_stored_y = smidy(p);
            if ((mouse_stored_x >= 0) && (mouse_stored_x < UI_SCREEN_W) && (mouse_stored_y >= 0) && (mouse_stored_y < UI_SCREEN_H)) {
                ui_cursor_update_gfx_i(mouse_stored_x, mouse_stored_y);
                uiobj_mouseoff = ui_cursor_mouseoff;
                mouse_stored_x -= uiobj_mouseoff;
                mouse_stored_y -= uiobj_mouseoff;
                mouse_set_xy(mouse_stored_x, mouse_stored_y);
/*
                ui_cursor_erase0();
                ui_cursor_store_bg0(mouse_stored_x, mouse_stored_y);
                ui_cursor_draw0(mouse_stored_x, mouse_stored_y);
*/
                /*hw_video_redraw_front();*/
            }
        }
        return oi;
    }
}

static int16_t uiobj_handle_kbd_find_alt(int16_t oi, uint32_t key)
{
    const uiobj_t *p = &uiobj_tbl[oi];
    while (1
      && (oi != uiobj_table_num)
      && (!((KBD_GET_KEYMOD(key) == p->key) && (p->type != 8)))
    ) {
        if ((p->type == 8) && KBD_MOD_ONLY_ALT(key) && (KBD_GET_KEY(key) == p->key)) {
            break;
        }
        ++oi;
        p = &uiobj_tbl[oi];
    }
    return oi;
}

static uint32_t uiobj_handle_kbd(int16_t *oiptr)
{
    uint32_t key = kbd_get_keypress();
    uiobj_t *p;
    int16_t /*si*/oi, /*di*/oi2;
    bool flag_reset_alt_str;
#ifdef FEATURE_MODEBUG
    if (KBD_GET_KEY(key) == 0) {
        LOG_DEBUG((0, "%s: got 0 key 0x%x\n", KBD_GET_KEY(key), key));
    }
#endif
    if (uiobj_kbd_hmm1 >= uiobj_table_num) {
        uiobj_kbd_hmm1 = 0;
    }
    oi = uiobj_kbd_hmm1 + 1;
    oi = uiobj_handle_kbd_find_alt(oi, key);
    p = &uiobj_tbl[oi];
    /*key = ucase(key)*/
    if (oi == uiobj_table_num) {
        oi = uiobj_handle_kbd_find_alt(1, key);
        p = &uiobj_tbl[oi];
    }
    uiobj_kbd_hmm1 = oi;
    flag_reset_alt_str = true;
    if (oi < uiobj_table_num) {
        *oiptr = oi;
        p = &uiobj_tbl[oi];
        if ((p->x0 < UI_SCREEN_W) && (p->y0 < UI_SCREEN_H)) {
            mouse_stored_x = p->x0 + (p->x1 - p->x0) / 2;
            mouse_stored_y = p->y0 + (p->y1 - p->y0) / 2;
            if ((mouse_stored_x < UI_SCREEN_W) && (mouse_stored_y < UI_SCREEN_H)) {
                ui_cursor_update_gfx_i(mouse_stored_x, mouse_stored_y);
                uiobj_mouseoff = ui_cursor_mouseoff;
                mouse_stored_x -= uiobj_mouseoff;
                mouse_stored_y -= uiobj_mouseoff;
                mouse_set_xy(mouse_stored_x, mouse_stored_y);
/*
                ui_cursor_erase0();
                ui_cursor_store_bg0(mouse_stored_x, mouse_stored_y);
                ui_cursor_draw0(mouse_stored_x, mouse_stored_y);
*/
                /*hw_video_redraw_front();*/
            }
        }
        if (p->type == 8) {
            if (++p->t8.pos >= p->t8.len) {
                p->t8.pos = 0;
            } else {
                *oiptr = 0;
                key = MOO_KEY_UNKNOWN;
            }
            p->key = p->t8.str[p->t8.pos];
            flag_reset_alt_str = false;
        }
    } else {
        int dirx, diry;
        dirx = 0;
        diry = 0;
        oi2 = *oiptr;
        switch (KBD_GET_KEYMOD(key)) {
            case MOO_KEY_LEFT:
            case MOO_KEY_KP4:
                dirx = -1;
                break;
            case MOO_KEY_RIGHT:
            case MOO_KEY_KP6:
                dirx = 1;
                break;
            case MOO_KEY_UP:
            case MOO_KEY_KP8:
                diry = -1;
                break;
            case MOO_KEY_DOWN:
            case MOO_KEY_KP2:
                diry = 1;
                break;
            case MOO_KEY_KP7:
                dirx = -1;
                diry = -1;
                break;
            case MOO_KEY_KP9:
                dirx = 1;
                diry = -1;
                break;
            case MOO_KEY_KP1:
                dirx = -1;
                diry = 1;
                break;
            case MOO_KEY_KP3:
                dirx = 1;
                diry = 1;
                break;
            default:
                break;
        }
        if ((dirx || diry) && (KBD_GET_MOD(key) == 0)) {
            oi2 = uiobj_kbd_dir_key(dirx, diry);
        }
        *oiptr = oi2;
    }
    if (flag_reset_alt_str) {
        for (int16_t oi3 = 0; oi3 < uiobj_table_num; ++oi3) {
            p = &uiobj_tbl[oi3];
            if (p->type == 8) {
                p->t8.pos = 0;
                p->key = p->t8.str[0];
            }
        }
    }
    return key;
}

static void uiobj_cursor_redraw_hmm2(int16_t oi, int mx, int my)
{
    if ((mx < 0) || (mx >= UI_SCREEN_W) || (my < 0) || (my >= UI_SCREEN_H)) {
        return;
    }
    if (1/*mouse_flag_initialized*/) {
        uiobj_t *p = &uiobj_tbl[oi];
        if (uiobj_hmm1_oi != oi) {
            ui_cursor_erase0();
            if (uiobj_hmm1_oi != -1) {
                uiobj_t *q = &uiobj_tbl[uiobj_hmm1_oi];
                /*if (uiobj_hmm1_oi != oi) {  redundant, checked above */
                if ((q->type != 3) || (p->type == 3)) {
                    if (q->type == 0xa) {
                        if ((p->type == 0xa) && p->ta.z12) {
                            uiobj_handle_hmm2(uiobj_hmm1_oi, 0);
                        }
                    } else {
                        uiobj_handle_hmm2(uiobj_hmm1_oi, 0);
                    }
                }
            }
            uiobj_hmm1_oi = oi;
            uiobj_handle_hmm2(oi, 1);
            if (p->type == 4) {
                mx = mouse_x;
                my = mouse_y;
            }
            ui_cursor_store_bg0(mx, my);
            ui_cursor_draw0(mx, my);
            mouse_set_xy(mx, my);
        }
    } else {
        /*don't care*/
    }
}

static void uiobj_finish_callback_delay_p(int delay)
{
    if (uiobj_flag_have_cb) {
        ui_delay_prepare();
        uiobj_do_callback();
        ui_palette_set_n();
        uiobj_finish_frame();
        ui_delay_ticks_or_click(delay);
    } else {
        ui_delay_prepare();
        ui_palette_set_n();
        uiobj_finish_frame();
        ui_delay_ticks_or_click(delay); /* MOO1 does not do this, but we need it to update mouse_* etc */
    }
}

static void uiobj_finish_callback_delay_1(void)
{
    uiobj_finish_callback_delay_p(1);
}

static void uiobj_finish_callback_delay_hmm5(void)
{
    uiobj_finish_callback_delay_p(uiobj_hmm5_delay);
}

static void uiobj_slider_plus(uiobj_t *p)
{
    uint16_t vmin = p->t6.vmin;
    uint16_t value = *p->vptr;
    uint16_t vdiff = p->t6.vmax - vmin;
    int newval = ((value - vmin) * 100) / vdiff + 5;
    if (newval <= 100) {
        newval = (newval * vdiff) / 100 + vmin;
    } else {
        newval = p->t6.vmax;
    }
    value = newval;
    if (p->t6.fmax < value) {
        value = p->t6.fmax;
    }
    *p->vptr = value;
}

static void uiobj_slider_minus(uiobj_t *p)
{
    uint16_t vmin = p->t6.vmin;
    uint16_t value = *p->vptr;
    uint16_t vdiff = p->t6.vmax - vmin;
    int newval = ((value - vmin) * 100) / vdiff - 5;
    if (newval >= 0) {
        newval = (newval * vdiff) / 100 + vmin;
    } else {
        newval = p->t6.vmin;
    }
    value = newval;
    if (p->t6.fmin > value) {
        value = p->t6.fmin;
    }
    *p->vptr = value;
}

static int16_t uiobj_handle_input_sub0(void)
{
    int16_t oi = 0;
    uiobj_t *p, *q;
    int mx = mouse_x, my = mouse_y, mb;
    uiobj_hmm1_oi = -1;
    uiobj_hmm2_oi = 0;
    uiobj_mouseoff = ui_cursor_mouseoff;
    if (kbd_have_keypress()) {
        uint32_t key = uiobj_handle_kbd(&oi);
        if (KBD_GET_KEY(key) == MOO_KEY_UNKNOWN) {
            return 0;
        }
        /* checks for F11 and F12 debug keys omitted */
        if (KBD_GET_KEY(key) == MOO_KEY_F1) {
            if (uiobj_help_id != -1) {
                int id;
                id = uiobj_help_id;
                oi = uiobj_at_cursor();
                if ((oi != 0) && (uiobj_tbl[oi].helpid != -1)) {
                    id = uiobj_tbl[oi].helpid;
                }
                ui_help(id);
            }
            return 0;
        }
        if (KBD_GET_KEYMOD(key) == MOO_KEY_ESCAPE) {
            return -1;
        }
        p = &uiobj_tbl[oi];
        if (p->type == 8) {
            return oi;
        }
        if (KBD_GET_KEYMOD(key) == p->key) {
            if (p->type == 6) {
                return 0;
            }
            if (oi != 0) {
                mx = p->x0 + (p->x1 - p->x0) / 2;
                my = p->y0 + (p->y1 - p->y0) / 2;
                uiobj_cursor_redraw_hmm2(oi, mx, my);
                if (p->type == 1) {
                    if (*p->vptr == 0) {
                        *p->vptr = 1;
                    } else {
                        *p->vptr = 0;
                    }
                } else if (p->type == 2) {
                    if (*p->vptr == 0) {
                        *p->vptr = 1;
                    }
                } else if (p->type == 9) {
                    uiobj_hmm1_oi = -1;
                    return p->t9.uiobji;
                }
            }
            uiobj_finish_callback_delay_1();
            uiobj_hmm1_oi = -1;
            return oi;
        }
        if (KBD_GET_KEY(key) == MOO_KEY_RETURN) {
            oi = uiobj_find_obj_at_cursor();
            if (oi != 0) {
                p = &uiobj_tbl[oi];
                if (p->type != 6) {
                    uiobj_cursor_redraw_hmm2(oi, mx, my);
                }
                if (p->type == 1) {
                    if (*p->vptr == 0) {
                        *p->vptr = 1;
                    } else {
                        *p->vptr = 0;
                    }
                } else if (p->type == 2) {
                    if (*p->vptr == 0) {
                        *p->vptr = 1;
                    }
                } else if (p->type == 9) {
                    uiobj_hmm1_oi = -1;
                    return p->t9.uiobji;
                }
                if (uiobj_flag_skip_delay == 0) {
                    uiobj_finish_callback_delay_1();
                }
                uiobj_hmm1_oi = -1;
                return oi;
            } else {
                if (uiobj_hmm6 != 0) {
                    for (oi = 1; oi < uiobj_table_num; ++oi) {
                        p = &uiobj_tbl[oi];
                        if ((p->type == 0xa) && (*p->vptr == p->ta.z18) && p->ta.z12) {
                            uiobj_hmm1_oi = -1;
                            return oi;
                        }
                    }
                }
            }
        }
        if ((KBD_GET_CHAR(key) == '+') || (KBD_GET_CHAR(key) == '-')) {
            oi = uiobj_find_obj_at_cursor();
            if (oi != 0) {
                p = &uiobj_tbl[oi];
                if (p->type == 6) {
                    if (KBD_GET_CHAR(key) == '+') {
                        uiobj_slider_plus(p);
                    } else {
                        uiobj_slider_minus(p);
                    }
                    uiobj_hmm1_oi = -1;
                    return oi;
                } else {
                    uiobj_hmm1_oi = -1;
                    return 0;
                }
            }
        }
        uiobj_hmm1_oi = -1;
        return 0;
    }
    if (mouse_buttons == 0) {
        if (!mouse_getclear_hmm4()) {
            return 0;
        }
        mb = mouse_click_buttons;
        if (mb == MOUSE_BUTTON_MASK_RIGHT) {
            mouse_getclear_hmm4();
            mouse_getclear_hmm5();
            return -1;
        } else {
            mx = mouse_click_x;
            my = mouse_click_y;
            oi = 0;
            /*key = 0;*/
            ui_cursor_update_gfx_i(mx, my);
            uiobj_mouseoff = ui_cursor_mouseoff;
            for (int oi2 = 0; oi2 < uiobj_table_num; ++oi2) {
                p = &uiobj_tbl[oi2];
                if (!uiobj_is_at_xy(p, mx, my)) {
                    continue;
                }
                oi = oi2;
                break;
            }
            p = &uiobj_tbl[oi];
            if (oi != 0) {
                uiobj_hmm2_oi = oi;
                uiobj_cursor_redraw_hmm2(oi, mx, my);
                uiobj_finish_callback_delay_1();
            }
            uiobj_hmm1_oi = -1;
            if (oi != 0) {
                mouse_getclear_hmm5();
            }
            if (p->type == 9) {
                p = &uiobj_tbl[oi];
                return p->t9.uiobji;
            } else {
                uiobj_hmm2_oi = oi;
                if (mb == MOUSE_BUTTON_MASK_RIGHT) {
                    return -oi;
                } else {
                    return oi;
                }
            }
        }
    } else {
        mb = mouse_buttons;
        if (mb == MOUSE_BUTTON_MASK_RIGHT) {
            while ((mb = mouse_buttons) == MOUSE_BUTTON_MASK_RIGHT) {
                uiobj_finish_callback_delay_1();
            }
            mouse_getclear_hmm4();
            mouse_getclear_hmm5();
            return -1;
        }
        while (mouse_buttons != 0) {
            mx = mouse_x;
            my = mouse_y;
            uiobj_mouseoff = ui_cursor_mouseoff;
            oi = uiobj_find_obj_at_cursor();
            if (oi == 0) {
                if (uiobj_hmm1_oi != -1) {
                    p = &uiobj_tbl[uiobj_hmm1_oi];
                    if (p->type == 6) {
                        uiobj_do_callback();
                    }
                    if ((p->type != 3) && (p->type != 0xa)) {
                        ui_cursor_erase0();
                        uiobj_handle_hmm2(uiobj_hmm1_oi, 0);
                        ui_cursor_store_bg0(mx, my);
                        ui_cursor_draw0(mx, my);
                        mouse_set_xy(mx, my);
                    }
                    uiobj_hmm1_oi = -1;
                }
                mouse_set_click_xy(mx, my);
                break;
            }
            q = &uiobj_tbl[uiobj_hmm1_oi];
            p = &uiobj_tbl[oi];
            if ((oi != uiobj_hmm1_oi) && (p->type != 4)) {
                if (q->type == 6) {
                    uiobj_do_callback();
                }
                uiobj_cursor_redraw_hmm2(oi, mx, my);
            }
            uiobj_hmm2_oi = oi;
            if (uiobj_flag_skip_delay != 0) {
                break;
            }
            if (mouse_buttons != 0) {
                uiobj_finish_callback_delay_hmm5();
            }
        }
        q = &uiobj_tbl[uiobj_hmm2_oi];
        if (q->type == 6) {
            uiobj_do_callback();
        }
        uiobj_hmm2_oi = 0;
        if (oi != 0) {
            mouse_getclear_hmm4();
            mouse_getclear_hmm5();
            switch (p->type) {
                case 2:
                    if (*p->vptr == 0) {
                        *p->vptr = 1;
                    }
                    break;
                case 9:
                    if (mb != MOUSE_BUTTON_MASK_RIGHT) {
                        uiobj_hmm1_oi = -1;
                        p = &uiobj_tbl[oi];
                        return p->t9.uiobji;
                    }
                    return -1;
                case 1:
                    if (*p->vptr == 0) {
                        *p->vptr = 1;
                    } else {
                        *p->vptr = 0;
                    }
                    break;
                case 4:
                    uiobj_cursor_redraw_hmm2(oi, mx, my);
                default:
                    break;
            }
        }
        uiobj_hmm1_oi = -1;
        if (mb == MOUSE_BUTTON_MASK_RIGHT) {
            return -oi;
        } else {
            return oi;
        }
    }

    return 0;
}

static void uiobj_add_t03_do(uint16_t x, uint16_t y, const char *str, uint8_t *lbxdata, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    p->x0 = x;
    p->y0 = y;
    p->x1 = x + lbxgfx_get_w(lbxdata) - 1;
    p->y1 = y + lbxgfx_get_h(lbxdata) - 1;
    p->t0.str = str;
    p->t0.fontnum = lbxfont_get_current_fontnum();
    p->t0.fonta2 = lbxfont_get_current_fonta2();
    p->helpid = helpid;
    p->t0.lbxdata = lbxdata;
    p->t0.indep = lbxgfx_get_indep(lbxdata);
    p->key = key;
}

/* -------------------------------------------------------------------------- */

void uiobj_table_clear(void)
{
    uiobj_table_num = 1;
    uiobj_hmm1_oi = -1;
    uiobj_hmm2_oi = 0;
}

void uiobj_table_set_last(int16_t oi)
{
    uiobj_table_num = oi + 1;
    uiobj_hmm1_oi = -1;
}

void uiobj_table_num_store(void)
{
    uiobj_table_num_old = uiobj_table_num;
    uiobj_table_num = 0;
    uiobj_flag_have_cb = false;
}

void uiobj_table_num_restore(void)
{
    uiobj_table_num = uiobj_table_num_old;
    if (uiobj_callback) {
        uiobj_flag_have_cb = true;
    }
}

void uiobj_handle_hmm1(void)
{
    for (int i = 1; i < uiobj_table_num; ++i) {
        uiobj_t *p = &uiobj_tbl[i];
        if ((i == uiobj_hmm1_oi) && (p->type != 4)) {
            uiobj_handle_hmm2(i, 1);
        } else {
            uiobj_handle_hmm1_sub1(i);
        }
    }
}

int16_t uiobj_handle_input_cond(void)
{
    if (uiobj_handle_downcount > 0) {
        --uiobj_handle_downcount;
        return 0;
    }
    uiobj_handle_downcount = 0;
    if (uiobj_table_num <= 1) {
        return 0;
    }
    if (1/*mouse_initialized*/) {
        return uiobj_handle_input_sub0();
    } else {
        return 0;/*uiobj_handle_input_sub1();*/
    }
}

void uiobj_finish_frame(void)
{
    int mx, my;
    mx = mouse_x;
    my = mouse_y;
    uiobj_handle_hmm1();
    ui_cursor_update_gfx_i(mx, my);
    ui_cursor_store_bg1(mx, my);
    ui_cursor_draw1(mx, my);
    hw_video_draw_buf();
#if 1
    /* FIXME HACK just erase it right after draw... */
    ui_cursor_copy_bg1_to_bg0();
    ui_cursor_erase0();
#else
    ui_cursor_erase0();
    ui_cursor_copy_bg1_to_bg0();
#endif
}

void uiobj_set_downcount(int16_t v)
{
    uiobj_handle_downcount = v;
    mouse_getclear_hmm4();
    mouse_getclear_hmm5();
}

void uiobj_set_hmm3_xyoff(int xoff, int yoff)
{
    uiobj_hmm3_xoff = xoff;
    uiobj_hmm3_yoff = yoff;
}

void uiobj_set_limits(int minx, int miny, int maxx, int maxy)
{
    SETMAX(minx, 0);
    SETMAX(miny, 0);
    SETMIN(maxx, UI_SCREEN_W - 1);
    SETMIN(maxy, UI_SCREEN_H - 1);
    if (minx > maxx) { int t = minx; minx = maxx; maxx = t; }
    if (miny > maxy) { int t = miny; miny = maxy; maxy = t; }
    uiobj_minx = minx;
    uiobj_miny = miny;
    uiobj_maxx = maxx;
    uiobj_maxy = maxy;
}

void uiobj_set_limits_all(void)
{
    uiobj_minx = 0;
    uiobj_miny = 0;
    uiobj_maxx = UI_SCREEN_W - 1;
    uiobj_maxy = UI_SCREEN_H - 1;
}

void uiobj_set_help_id(int16_t v)
{
    uiobj_help_id = v;
}

void uiobj_set_hmm8_0(void)
{
    uiobj_hmm8 = 0;
}

int16_t uiobj_get_hmm2_oi(void)
{
    return uiobj_hmm2_oi;
}

void uiobj_set_skip_delay(bool v)
{
    uiobj_flag_skip_delay = v;
}

void uiobj_set_callback_and_delay(void (*cb)(void *), void *data, uint16_t delay)
{
    uiobj_callback = cb;
    uiobj_cbdata = data;
    uiobj_flag_have_cb = true;
    uiobj_hmm5_delay = ((delay > 0) && (delay < 10)) ? delay : 2;
}

void uiobj_unset_callback(void)
{
    uiobj_callback = NULL;
    uiobj_cbdata = NULL;
    uiobj_flag_have_cb = false;
}

void uiobj_do_callback(void)
{
    if (uiobj_flag_have_cb) {
        uiobj_callback(uiobj_cbdata);
    }
}

void uiobj_set_focus(int16_t uiobji)
{
    uiobj_t *p = &uiobj_tbl[uiobji];
    int x, y;
    x = smidx(p);
    y = smidy(p);
    if ((y < 0) || (y >= UI_SCREEN_H) || (x < 0) || (x >= UI_SCREEN_W)) {
        return;
    }
    ui_cursor_update_gfx_i(x, y);
    uiobj_mouseoff = ui_cursor_mouseoff;
    x -= uiobj_mouseoff;
    y -= uiobj_mouseoff;
    mouse_set_xy(x, y);
    ui_cursor_erase0();
    ui_cursor_store_bg0(x, y);
    ui_cursor_draw0(x, y);
    /* needed anywhere? */
    mouse_stored_x = x;
    mouse_stored_y = y;
}

int16_t uiobj_find_obj_at_cursor(void)
{
    int x = mouse_x, y = mouse_y;
    ui_cursor_update_gfx_i(x, y);
    uiobj_mouseoff = ui_cursor_mouseoff;
    for (int i = 1; i < uiobj_table_num; ++i) {
        uiobj_t *p = &uiobj_tbl[i];
        if (!uiobj_is_at_xy(p, x, y)) {
            continue;
        }
        return i;
    }
    return 0;
}

int16_t uiobj_at_cursor(void)
{
    uiobj_t *p;
    int i, x = mouse_x, y = mouse_y;
    ui_cursor_update_gfx_i(x, y);
    uiobj_mouseoff = ui_cursor_mouseoff;
    i = uiobj_find_obj_at_cursor();
    p = &uiobj_tbl[i];
    if (p->type == 9) {
        *p->vptr = p->t9.z18;
        i = p->t9.uiobji;
    } else if ((p->type == 0xa) && !p->ta.z12) {
        i = 0;
    }
    return i;
}

int16_t uiobj_add_t0(uint16_t x, uint16_t y, const char *str, uint8_t *lbxdata, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    uiobj_add_t03_do(x, y, str, lbxdata, key, helpid);
    p->type = 0;
    p->vptr = 0;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_t1(uint16_t x, uint16_t y, const char *str, uint8_t *lbxdata, int16_t *vptr, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    uiobj_add_t03_do(x, y, str, lbxdata, key, helpid);
    p->type = 1;
    p->vptr = vptr;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_t2(uint16_t x, uint16_t y, const char *str, uint8_t *lbxdata, int16_t *vptr, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    uiobj_add_t03_do(x, y, str, lbxdata, key, helpid);
    p->type = 2;
    p->vptr = vptr;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_t3(uint16_t x, uint16_t y, const char *str, uint8_t *lbxdata, int16_t *vptr, int16_t z18, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    uiobj_add_t03_do(x, y, str, lbxdata, key, helpid);
    p->type = 3;
    p->vptr = vptr;
    p->t0.z18 = z18;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_textinput(int x, int y, int w, char *buf, uint16_t buflen, uint8_t rcolor, bool alignr, uint16_t z1e, const uint8_t *colortbl, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    p->x0 = x;
    p->y0 = y;
    p->x1 = x + w;
    p->y1 = y + lbxfont_get_height();
    p->t4.fontnum = lbxfont_get_current_fontnum();
    p->t4.fonta2 = lbxfont_get_current_fonta2();
    p->t4.fonta4 = lbxfont_get_current_fonta2b();
    p->t4.buflen = buflen;
    p->t4.buf = buf;
    p->t4.rectcolor = rcolor;
    p->t4.align_right = alignr;
    p->t4.z1e = z1e;
    p->t4.colortbl = colortbl;
    p->type = 4;
    p->helpid = helpid;
    p->vptr = 0;
    p->key = key;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_slider(uint16_t x0, uint16_t y0, uint16_t vmin, uint16_t vmax, uint16_t fmin, uint16_t fmax, uint16_t w, uint16_t h, int16_t *vptr, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    p->x0 = x0;
    p->y0 = y0;
    p->x1 = x0 + w;
    p->y1 = y0 + h;
    p->t6.vmin = vmin;
    p->t6.vmax = vmax;
    p->t6.fmin = fmin;
    p->t6.fmax = fmax;
    p->t6.vertical = (h > w);
    p->type = 6;
    p->helpid = helpid;
    p->vptr = vptr;
    p->key = key;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_mousearea(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    p->x0 = x0;
    p->y0 = y0;
    p->x1 = x1;
    p->y1 = y1;
    p->type = 7;
    p->helpid = helpid;
    p->vptr = 0;
    p->key = key;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_mousearea_limited(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, mookey_t key, int16_t helpid)
{
    if ((x1 < uiobj_minx) || (x0 > uiobj_maxx) || (y1 < uiobj_miny) || (y0 > uiobj_maxy)) {
        return UIOBJI_OUTSIDE;
    }
    x0 = MAX(x0, uiobj_minx);
    x1 = MIN(x1, uiobj_maxx);
    y0 = MAX(y0, uiobj_miny);
    y1 = MIN(y1, uiobj_maxy);
    return uiobj_add_mousearea(x0, y0, x1, y1, key, helpid);
}

int16_t uiobj_add_inputkey(uint32_t key)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    p->x0 = UIOBJ_OFFSCREEN;
    p->y0 = UIOBJ_OFFSCREEN;
    p->x1 = UIOBJ_OFFSCREEN;
    p->y1 = UIOBJ_OFFSCREEN;
    p->type = 7;
    p->helpid = -1;
    p->vptr = 0;
    p->key = key;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_alt_str(const char *str)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    int len = strnlen(str, 0x1e);
    p->x0 = UIOBJ_OFFSCREEN;
    p->y0 = UIOBJ_OFFSCREEN;
    p->x1 = UIOBJ_OFFSCREEN;
    p->y1 = UIOBJ_OFFSCREEN;
    p->type = 8;
    p->helpid = -1;
    p->vptr = 0;
    p->t8.str = str;
    p->t8.pos = 0;
    p->t8.len = len;
    {
        char b = *str;
        if ((b >= 'a') && (b <= 'z')) {
            b -= 'a' - 'A';
        }
        p->key = b;
    }
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_ta(uint16_t x, uint16_t y, uint16_t w, const char *str, bool z12, int16_t *vptr, int16_t z18, uint16_t subtype, uint8_t *sp0p, uint16_t sp0v, uint16_t sp1, uint16_t sp2, uint16_t sp3, mookey_t key, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    p->x0 = x;
    p->y0 = y - 1;
    p->x1 = x + w;
    p->y1 = y + lbxfont_get_height() + 1;
    p->ta.fontnum = lbxfont_get_current_fontnum();
    p->ta.fonta2 = lbxfont_get_current_fonta2();
    p->ta.fonta2b = lbxfont_get_current_fonta2b();
    p->ta.z18 = z18;
    p->ta.z12 = z12;
    p->ta.str = str;
    p->ta.subtype = subtype;
    p->ta.sp0p = sp0p;
    p->ta.sp0v = sp0v;
    p->ta.sp1 = sp1;
    p->ta.sp2 = sp2;
    p->ta.sp3 = sp3;
    p->type = 0xa;
    p->helpid = helpid;
    p->vptr = vptr;
    p->key = key;
    return UIOBJI_ALLOC();
}

int16_t uiobj_add_tb(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t xscale, uint16_t yscale, uint16_t *xptr, uint16_t *yptr, int16_t helpid)
{
    uiobj_t *p = &uiobj_tbl[uiobj_table_num];
    p->x0 = x;
    p->y0 = y;
    p->x1 = x + w * xscale;
    p->y1 = y + h * yscale;
    p->tb.xdiv = w;
    p->tb.ydiv = h;
    p->tb.xptr = xptr;
    p->tb.yptr = yptr;
    p->type = 0xb;
    p->helpid = helpid;
    p->vptr = 0;
    p->key = MOO_KEY_UNKNOWN;
    return UIOBJI_ALLOC();
}

void uiobj_dec_y1(int16_t oi)
{
    --uiobj_tbl[oi].y1;
}

void uiobj_ta_set_val_0(int16_t oi)
{
    uiobj_t *p = &uiobj_tbl[oi];
    if (p->type == 0xa) {
        *p->vptr = 0;
    }
}

void uiobj_ta_set_val_1(int16_t oi)
{
    uiobj_t *p = &uiobj_tbl[oi];
    if (p->type == 0xa) {
        *p->vptr = 1;
    }
}

int16_t uiobj_select_from_list1(int x, int y, int w, const char *title, char const * const *strtbl, int16_t *selptr, const bool *condtbl, uint16_t subtype, uint8_t *sp0p, uint16_t sp0v, uint16_t sp1, uint16_t sp2, uint16_t sp3, int16_t helpid)
{
    int h, dy, ty = y, di = -1;
    bool flag_done = false, toz12, flag_copy_buf = false;
    uint16_t itemi = 0, v6 = 0;
    int16_t oi = 0, oi_title, v18 = 0;
    char const * const *s = strtbl;

    uiobj_hmm6 = 1;
    uiobj_set_downcount(1);
    uiobj_table_clear();
    h = lbxfont_get_height();
    dy = lbxfont_get_gap_h() + h;

    while (!flag_done) {
        if (*s == 0) {
            flag_done = true;
            break;
        }
        if (!v6) {
            if ((!condtbl) || condtbl[itemi]) {
                di = itemi;
                v6 = 1;
            }
        }
        /*18a50*/
        ty += dy;
        if (!condtbl) {
            toz12 = true;
        } else {
            toz12 = condtbl[itemi];
        }
        uiobj_add_ta(x, ty, w, *s, toz12, selptr, itemi, subtype, sp0p, sp0v, sp1, sp2, sp3, MOO_KEY_UNKNOWN, helpid);
        ++itemi;
        ++s;
    }

    v6 = itemi;
    lbxfont_select(lbxfont_get_current_fontnum(), lbxfont_get_current_fonta2(), lbxfont_get_current_fonta4(), 0);
    oi_title = uiobj_add_ta(x, y, w, title, false, &v18, 1, 0, 0, 0, 0, 0, 0, MOO_KEY_UNKNOWN, uiobj_help_id);

    if ((*selptr < 0) || (*selptr >= v6) || (*selptr < di)) {
        if ((di >= 0) && (di < v6)) {
            *selptr = uiobj_tbl[di + 1].ta.z18;
        } else {
            *selptr = -1;
        }
    }

    flag_done = false;
    oi = 0;

    while (!flag_done) {
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if (oi != 0) {
            flag_done = true;
        }
        if ((oi == oi_title) || ((oi > 0) && condtbl && !condtbl[oi - 1])) {
            flag_done = false;
        }
        if (flag_done) {
            break;
        }
        uiobj_do_callback();
        if (uiobj_hmm8 != 0) {
            int oi2;
            oi2 = uiobj_at_cursor();
            if (oi2 > 0) {
                *selptr = uiobj_tbl[oi2].ta.z18;
            }
        }
        ui_palette_set_n();
        uiobj_finish_frame();
        if (!flag_copy_buf) {
            ui_draw_copy_buf();
            flag_copy_buf = true;
        }
        ui_delay_ticks_or_click(uiobj_hmm5_delay);
    }
    uiobj_table_clear();
    uiobj_hmm6 = 0;
    uiobj_hmm8 = 1;
    mouse_getclear_hmm4();
    mouse_getclear_hmm5();
    if (oi < 0) {
        return -1;
    }
    return oi - 1;
}

int16_t uiobj_select_from_list2(int x, int y, int w, const char *title, char const * const *strtbl, int16_t *selptr, const bool *condtbl, int linenum, int upx, int upy, uint8_t *uplbx, int dnx, int dny, uint8_t *dnlbx, uint16_t subtype, uint8_t *sp0p, uint16_t sp0v, uint16_t sp1, uint16_t sp2, uint16_t sp3, int16_t helpid)
{
    int h, dy, ty, linei = 0, itemi = 0, itemnum, itemoffs, foundi = 0;
    bool flag_done = false, flag_copy_buf = false, flag_found = false;
    uint16_t fonta4, fonta2b;
    int16_t oi = 0, oi_title, oi_up, oi_dn, v18 = 0, upvar, dnvar, curval;
    char const * const *s = strtbl;

    uiobj_hmm6 = 1;
    uiobj_hmm9 = 1;
    uiobj_kbd_movey = 0;
    fonta4 = lbxfont_get_current_fonta4();
    fonta2b = lbxfont_get_current_fonta2b();
    uiobj_set_downcount(1);
    uiobj_table_clear();
    h = lbxfont_get_height();
    dy = lbxfont_get_gap_h() + h;
    ty = y + dy;

    while (!flag_done) {
        if (*s == 0) {
            flag_done = true;
            break;
        }
        if (!flag_found) {
            if ((!condtbl) || condtbl[itemi]) {
                foundi = itemi;
                flag_found = true;
            }
        }
        ++itemi;
        ++s;
    }
    itemnum = itemi;

    {
        int i;
        i = *selptr;
        if ((i < 0) || (i >= itemnum)) {
            i = 0;
        }
        if ((i + linenum) > itemnum) {
            i = itemnum - linenum;
            SETMAX(i, 0);
        }
        itemoffs = i;
    }
    s = &strtbl[itemoffs];
    for (itemi = itemoffs; (itemi < itemnum) && (linei < linenum); ++itemi, ++linei, ++s, ty += dy) {
        uiobj_add_ta(x, ty, w, *s, (!condtbl) || condtbl[itemi], selptr, itemi, subtype, sp0p, sp0v, sp1, sp2, sp3, MOO_KEY_UNKNOWN, helpid);
    }

    if ((*selptr < 0) || (*selptr >= itemnum)) {
        if ((foundi >= 0) && (foundi < itemnum)) {
            *selptr = itemoffs;
        } else {
            *selptr = -1;
        }
    }

    lbxfont_select(lbxfont_get_current_fontnum(), lbxfont_get_current_fonta2(), fonta4, 0);
    oi_title = uiobj_add_ta(x, y, w, title, false, &v18, 1, 0, 0, 0, 0, 0, 0, MOO_KEY_UNKNOWN, uiobj_help_id);

    upvar = (itemoffs == 0) ? 1 : 0;
    dnvar = (itemi < itemnum) ? 1 : 0;
    oi_up = uiobj_add_t2(upx, upy, "", uplbx, &upvar, MOO_KEY_PAGEUP, -1);
    oi_dn = uiobj_add_t2(dnx, dny, "", dnlbx, &dnvar, MOO_KEY_PAGEDOWN, -1);

    flag_done = false;
    curval = *selptr;

    while (!flag_done) {
        bool flag_rebuild;
        ui_delay_prepare();
        oi = uiobj_handle_input_cond();
        if ((oi < 0) || ((oi > 0) && (oi < oi_title) && uiobj_tbl[oi].ta.z12)) {
            flag_done = true;
            break;
        }
        flag_rebuild = false;
        if (oi == oi_up) {
            itemoffs -= linenum;
            SETMAX(itemoffs, 0);
            flag_rebuild = true;
        } else if (oi == oi_dn) {
            int i;
            i = itemoffs + linenum;
            if ((i + linenum) > itemnum) {
                i = itemnum - linenum;
                SETMAX(i, 0);
            }
            if (i >= itemnum) {
                i = itemoffs;
            }
            itemoffs = i;
            flag_rebuild = true;
        }
        if (uiobj_kbd_movey == 1) {
            int i, j;
            i = itemoffs + 1;
            j = itemoffs + linenum;
            while ((j < itemnum) && condtbl && (!condtbl[j])) {
                ++j;
                ++i;
            }
            if (j >= itemnum) {
                i = itemoffs;
            }
            if ((i + linenum) > itemnum) {
                i = itemnum - linenum;
                SETMAX(i, 0);
            }
            itemoffs = i;
            flag_rebuild = true;
        } else if (uiobj_kbd_movey == -1) {
            int i;
            i = itemoffs - 1;
            SETMAX(i, 0);
            while ((i >= 0) && condtbl && (!condtbl[i])) {
                --i;
            }
            SETMAX(i, 0);
            itemoffs = i;
            flag_rebuild = true;
        }
        uiobj_kbd_movey = 0;
        if (flag_rebuild) {
            uiobj_table_clear();
            lbxfont_select(lbxfont_get_current_fontnum(), lbxfont_get_current_fonta2(), fonta2b, 0);
            *selptr = -1;
            if ((!condtbl) || condtbl[itemoffs]) {
                *selptr = itemoffs;
            } else {
                int i = itemoffs;
                while (i <= (itemoffs + linenum)) {
                    if ((!condtbl) || condtbl[i]) {
                        *selptr = i;
                        break;
                    }
                    ++i;
                }
            }
            s = &strtbl[itemoffs];
            linei = 0;
            ty = y + dy;
            for (itemi = itemoffs; (itemi < itemnum) && (linei < linenum); ++itemi, ++linei, ++s, ty += dy) {
                uiobj_add_ta(x, ty, w, *s, (!condtbl) || condtbl[itemi], selptr, itemi, subtype, sp0p, sp0v, sp1, sp2, sp3, MOO_KEY_UNKNOWN, helpid);
            }
            lbxfont_select(lbxfont_get_current_fontnum(), lbxfont_get_current_fonta2(), fonta4, 0);
            oi_title = uiobj_add_ta(x, y, w, title, false, &v18, 1, 0, 0, 0, 0, 0, 0, MOO_KEY_UNKNOWN, uiobj_help_id);
            upvar = (itemoffs == 0) ? 1 : 0;
            dnvar = (itemi < itemnum) ? 1 : 0;
            oi_up = uiobj_add_t2(upx, upy, "", uplbx, &upvar, MOO_KEY_PAGEUP, -1);
            oi_dn = uiobj_add_t2(dnx, dny, "", dnlbx, &dnvar, MOO_KEY_PAGEDOWN, -1);
        }
        if (uiobj_hmm8 != 0) {
            int oi2;
            oi2 = uiobj_at_cursor();
            if (oi2 > 0) {
                *selptr = uiobj_tbl[oi2].ta.z18;
            }
        }
        uiobj_do_callback();
        ui_palette_set_n();
        uiobj_finish_frame();
        if (!flag_copy_buf) {
            ui_draw_copy_buf();
            flag_copy_buf = true;
        }
        ui_delay_ticks_or_click(uiobj_hmm5_delay);
    }
    uiobj_table_clear();
    uiobj_hmm6 = 0;
    uiobj_hmm8 = 1;
    uiobj_hmm9 = 0;
    mouse_getclear_hmm4();
    mouse_getclear_hmm5();
    if (oi < 0) {
        *selptr = curval;
        return -1;
    }
    return oi + itemoffs - 1;
}

bool uiobj_read_str(int x, int y, int w, char *buf, int buflen, uint8_t rcolor, bool alignr, uint16_t z1e, const uint8_t *ctbl, int16_t helpid)
{
    char strbuf[64];
    uint16_t fonth, v4 = 0, vc = 0, va;
    int si, di;
    bool flag_done = false, flag_quit = false;
    mookey_t key = 0;
    uiobj_t *p;

    uiobj_table_clear();
    if (1/*mouse_flag_initialized*/) {
        while (mouse_buttons) {
            hw_event_handle();
        }
        mouse_getclear_hmm4();
        mouse_getclear_hmm5();
    }
    uiobj_set_downcount(1);
    {
        int16_t oi = uiobj_add_textinput(x, y, w, buf, buflen, rcolor, alignr, z1e, ctbl, MOO_KEY_UNKNOWN, helpid);
        uiobj_hmm1_oi = oi;
        p = &uiobj_tbl[oi];
    }
    fonth = lbxfont_get_height();
    strcpy(strbuf, buf);
    si = strlen(strbuf);
    if (lbxfont_calc_str_width(strbuf) >= w) {
        strbuf[0] = 0;
        si = 0;
    }
    di = si;
    if (di > buflen) {
        di = buflen;
    }
    uiobj_handle_t4_sub2(p, di, vc, strbuf);

    while ((key != MOO_KEY_RETURN) && !flag_done) {
        uint32_t keyp;
        bool flag_ok;
        char c;
        while (!kbd_have_keypress() && !flag_done) {
            if ((1/*mouse_flag_initialized*/) && (mouse_buttons || mouse_getclear_hmm4())) {
                flag_done = true;
                break;
            }
            if (++vc >= ((fonth << 1) - 1)) { vc = 0; }
            uiobj_handle_t4_sub2(p, di, vc, strbuf);
        }
        if (flag_done) {
            break;
        }
        keyp = kbd_get_keypress();
        key = KBD_GET_KEY(keyp);
        c = KBD_GET_CHAR(keyp);
        switch (key) {
            case MOO_KEY_BACKSPACE:
                if (v4 == 0) {
                    strbuf[0] = '\0';
                    si = di = 0;
                    vc = 0;
                    v4 = 1;
                } else if (si > 0) {
                    if (di >= si) {
                        --si;
                        strbuf[si] = '\0';
                        --di;
                        vc = 0;
                    } else if (di > 0) {
                        va = di;
                        while (va < si) {
                            strbuf[va - 1] = strbuf[va];
                            ++va;
                        }
                        --si;
                        --di;
                    }
                    strbuf[si] = '\0';
                }
                break;
            case MOO_KEY_DELETE:
                if ((si > 0) && (di < si)) {
                    va = di;
                    while (va < si) {
                        strbuf[va] = strbuf[va + 1];
                        ++va;
                    }
                    --si;
                    vc = 0;
                    strbuf[si] = '\0';
                }
                break;
            case MOO_KEY_LEFT:
                v4 = 1;
                if (di > 0) {
                    --di;
                    vc = 0;
                }
                break;
            case MOO_KEY_RIGHT:
                if ((di < buflen) && (di < si)) {
                    ++di;
                    vc = 0;
                    if (di >= si) {
                        strbuf[si] = ' ';
                        strbuf[si + 1] = '\0';
                        if ((di >= buflen) || (lbxfont_calc_str_width(strbuf) > w)) {
                            --di;
                        }
                        strbuf[si] = '\0';
                    }
                }
                break;
            case MOO_KEY_ESCAPE:
                flag_done = true;
                flag_quit = true;
                break;
            default:
                flag_ok = false;
                if (0
                  || ((c >= 'A') && (c < ']'))
                  || ((c >= 'a') && (c < '{'))
                  || ((c >= '-') && (c < ';'))
                  || (c == ' ') || (c == '-')
                ) {
                    flag_ok = true;
                }
                if (flag_ok) {
                    v4 = 1;
                    strbuf[si] = c;
                    strbuf[si + 1] = '\0';
                    if ((si < buflen) && (lbxfont_calc_str_width(strbuf) <= w)) {
                        strbuf[si] = '\0';
                        if (di < si) {
                            va = si;
                            while (va > di) {
                                strbuf[va] = strbuf[va - 1];
                                --va;
                            }
                            ++si;
                            strbuf[di] = c;
                            ++di;
                        } else {
                            strbuf[si] = c;
                            ++si;
                            strbuf[si] = ' ';
                            strbuf[si + 1] = '\0';
                            if ((si < buflen) && (lbxfont_calc_str_width(strbuf) <= w)) {
                                ++di;
                            }
                        }
                        strbuf[si] = '\0';
                        vc = 0;
                    } else {
                        strbuf[si] = '\0';
                    }
                }
                break;
        }
        uiobj_handle_t4_sub2(p, di, vc, strbuf);
    }
    strcpy(buf, strbuf);
    if (flag_done != 0) /*&& (mouse_flag_initialized)*/ {
        while (mouse_buttons) {
            hw_event_handle();
        }
    }
    /* TODO ui_cursor_erase0(); */
    uiobj_hmm1_oi = -1;
    uiobj_table_clear();
    mouse_getclear_hmm4();
    mouse_getclear_hmm5();
    return !flag_quit;
}

void uiobj_input_flush(void)
{
    uiobj_hmm2_oi = 0;
    while (kbd_have_keypress()) {
        kbd_get_keypress();
    }
    while (mouse_buttons) {
        uiobj_finish_callback_delay_hmm5();
    }
}

void uiobj_input_wait(void)
{
    bool got_any = false, got_mb = false;
    uiobj_input_flush();
    while (!got_any) {
        if (mouse_buttons || mouse_getclear_hmm4()) {
            got_any = true;
            got_mb = true;
        }
        if (kbd_have_keypress()) {
            kbd_get_keypress();
            got_any = true;
            mouse_getclear_hmm4();
        }
        uiobj_finish_callback_delay_hmm5();
    }
    if (got_mb) {
        while (mouse_buttons) {
            uiobj_finish_callback_delay_hmm5();
        }
    }
    mouse_getclear_hmm4();
    mouse_getclear_hmm5();
}
