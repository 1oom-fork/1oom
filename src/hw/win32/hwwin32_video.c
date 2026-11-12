#include "config.h"

#include <stdio.h>
#include <string.h>

#include <windows.h>

#include "hw_internal.h"

#include "hwwin32.h"
#include "lib.h"
#include "options.h"

/* -------------------------------------------------------------------------- */

struct win32_bmi_s {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[256];
};

static struct win32_video_s {
    HBITMAP dib;
    uint8_t *bits;
    RGBQUAD color[256];
    int w, h;
    int win_w, win_h;
    bool fullscreen_saved;
    WINDOWPLACEMENT saved_placement;
} video = { 0 };

HWND hwwin32_hwnd = NULL;

/* -------------------------------------------------------------------------- */

static void video_create_dib(int w, int h)
{
    BITMAPINFO bmi;
    HDC hdc;
    void *bits;

    if (video.dib) {
        DeleteObject(video.dib);
        video.dib = NULL;
        video.bits = NULL;
    }
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;    /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 8;
    bmi.bmiHeader.biCompression = BI_RGB;
    hdc = GetDC(hwwin32_hwnd);
    video.dib = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(hwwin32_hwnd, hdc);
    if (!video.dib) {
        log_error("CreateDIBSection(%i, %i) failed!\n", w, h);
        return;
    }
    video.bits = (uint8_t *)bits;
    video.w = w;
    video.h = h;
}

static int video_sw_set(int w, int h)
{
    RECT rect;

    if (!hwwin32_hwnd) {
        if (hwwin32_window_create(w, h)) {
            return -1;
        }
    }
    if (opt_fullscreen) {
        SetWindowPos(hwwin32_hwnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
    } else {
        rect.left = 0;
        rect.top = 0;
        rect.right = w;
        rect.bottom = h;
        AdjustWindowRect(&rect, (DWORD)GetWindowLongPtr(hwwin32_hwnd, GWL_STYLE), FALSE);
        SetWindowPos(hwwin32_hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
    }
    video.win_w = w;
    video.win_h = h;
    video_create_dib(w, h);
    return 0;
}

static void video_render_8bpp(void)
{
    const uint8_t *q = vgabuf_get_front();
    uint8_t *p = video.bits;
    if (!p) {
        return;
    }
    for (int y = 0; y < video.h; ++y) {
        memcpy(p, q, (size_t)video.w);
        q += video.w;
        p += video.w;
    }
}

static void video_update_8bpp(void)
{
    struct win32_bmi_s bmi;
    HDC hdc;
    int dw, dh;

    if (!video.dib) {
        return;
    }
    hdc = GetDC(hwwin32_hwnd);
    if (opt_fullscreen) {
        dw = GetSystemMetrics(SM_CXSCREEN);
        dh = GetSystemMetrics(SM_CYSCREEN);
    } else {
        RECT rect;
        GetClientRect(hwwin32_hwnd, &rect);
        dw = rect.right;
        dh = rect.bottom;
        video.win_w = dw;
        video.win_h = dh;
    }
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = video.w;
    bmi.bmiHeader.biHeight = -video.h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 8;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biClrUsed = 256;
    memcpy(bmi.bmiColors, video.color, sizeof(bmi.bmiColors));
    SetStretchBltMode(hdc, COLORONCOLOR);
    StretchDIBits(hdc, 0, 0, dw, dh, 0, 0, video.w, video.h, video.bits, (const BITMAPINFO *)&bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwwin32_hwnd, hdc);
}

static void video_setpal_8bpp(const uint8_t *pal, int first, int num)
{
    for (int i = first; i < (first + num); ++i) {
        video.color[i].rgbRed = vgapal_6bit_to_8bit(*pal++);
        video.color[i].rgbGreen = vgapal_6bit_to_8bit(*pal++);
        video.color[i].rgbBlue = vgapal_6bit_to_8bit(*pal++);
        video.color[i].rgbReserved = 0;
    }
    video_update_8bpp();
}

/* -------------------------------------------------------------------------- */

int hw_video_toggle_fullscreen(void)
{
    return 0;
}

int hw_video_init(int w, int h)
{
    i_hw_video.setmode = video_sw_set;
    i_hw_video.render = video_render_8bpp;
    i_hw_video.update = video_update_8bpp;
    i_hw_video.setpal = video_setpal_8bpp;
    memset(video.color, 0, sizeof(video.color));
    if (i_hw_video.setmode(w, h)) {
        return -1;
    }
    return 0;
}

void hw_video_shutdown(void)
{
    if (video.dib) {
        DeleteObject(video.dib);
        video.dib = NULL;
        video.bits = NULL;
    }
}

int hw_icon_set(const uint8_t *data, const uint8_t *pal, int w, int h)
{
    return 0;
}
