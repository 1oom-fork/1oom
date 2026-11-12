#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <windowsx.h>

#include "hw_internal.h"

#include "hwwin32.h"
#include "main.h"
#include "options.h"

/* -------------------------------------------------------------------------- */

const char *idstr_hw = "win32";

bool hw_use_mouse = true;
bool hw_mouse_enabled = true;

/* -------------------------------------------------------------------------- */

#define HW_WIN32_CLASS_NAME "1oom_win32"
#define HW_WIN32_WINDOW_NAME "1oom"

/* -------------------------------------------------------------------------- */

static mookey_t key_xlat[0x100];

static void build_key_xlat(void)
{
    memset(key_xlat, 0, sizeof(key_xlat));
    key_xlat['A'] = MOO_KEY_a;
    key_xlat['B'] = MOO_KEY_b;
    key_xlat['C'] = MOO_KEY_c;
    key_xlat['D'] = MOO_KEY_d;
    key_xlat['E'] = MOO_KEY_e;
    key_xlat['F'] = MOO_KEY_f;
    key_xlat['G'] = MOO_KEY_g;
    key_xlat['H'] = MOO_KEY_h;
    key_xlat['I'] = MOO_KEY_i;
    key_xlat['J'] = MOO_KEY_j;
    key_xlat['K'] = MOO_KEY_k;
    key_xlat['L'] = MOO_KEY_l;
    key_xlat['M'] = MOO_KEY_m;
    key_xlat['N'] = MOO_KEY_n;
    key_xlat['O'] = MOO_KEY_o;
    key_xlat['P'] = MOO_KEY_p;
    key_xlat['Q'] = MOO_KEY_q;
    key_xlat['R'] = MOO_KEY_r;
    key_xlat['S'] = MOO_KEY_s;
    key_xlat['T'] = MOO_KEY_t;
    key_xlat['U'] = MOO_KEY_u;
    key_xlat['V'] = MOO_KEY_v;
    key_xlat['W'] = MOO_KEY_w;
    key_xlat['X'] = MOO_KEY_x;
    key_xlat['Y'] = MOO_KEY_y;
    key_xlat['Z'] = MOO_KEY_z;
    key_xlat['0'] = MOO_KEY_0;
    key_xlat['1'] = MOO_KEY_1;
    key_xlat['2'] = MOO_KEY_2;
    key_xlat['3'] = MOO_KEY_3;
    key_xlat['4'] = MOO_KEY_4;
    key_xlat['5'] = MOO_KEY_5;
    key_xlat['6'] = MOO_KEY_6;
    key_xlat['7'] = MOO_KEY_7;
    key_xlat['8'] = MOO_KEY_8;
    key_xlat['9'] = MOO_KEY_9;
    key_xlat[VK_NUMPAD0] = MOO_KEY_KP0;
    key_xlat[VK_NUMPAD1] = MOO_KEY_KP1;
    key_xlat[VK_NUMPAD2] = MOO_KEY_KP2;
    key_xlat[VK_NUMPAD3] = MOO_KEY_KP3;
    key_xlat[VK_NUMPAD4] = MOO_KEY_KP4;
    key_xlat[VK_NUMPAD5] = MOO_KEY_KP5;
    key_xlat[VK_NUMPAD6] = MOO_KEY_KP6;
    key_xlat[VK_NUMPAD7] = MOO_KEY_KP7;
    key_xlat[VK_NUMPAD8] = MOO_KEY_KP8;
    key_xlat[VK_NUMPAD9] = MOO_KEY_KP9;
    key_xlat[VK_MULTIPLY] = MOO_KEY_KP_MULTIPLY;
    key_xlat[VK_ADD] = MOO_KEY_KP_PLUS;
    key_xlat[VK_SUBTRACT] = MOO_KEY_KP_MINUS;
    key_xlat[VK_DECIMAL] = MOO_KEY_KP_PERIOD;
    key_xlat[VK_DIVIDE] = MOO_KEY_KP_DIVIDE;
    key_xlat[VK_F1] = MOO_KEY_F1;
    key_xlat[VK_F2] = MOO_KEY_F2;
    key_xlat[VK_F3] = MOO_KEY_F3;
    key_xlat[VK_F4] = MOO_KEY_F4;
    key_xlat[VK_F5] = MOO_KEY_F5;
    key_xlat[VK_F6] = MOO_KEY_F6;
    key_xlat[VK_F7] = MOO_KEY_F7;
    key_xlat[VK_F8] = MOO_KEY_F8;
    key_xlat[VK_F9] = MOO_KEY_F9;
    key_xlat[VK_F10] = MOO_KEY_F10;
    key_xlat[VK_F11] = MOO_KEY_F11;
    key_xlat[VK_F12] = MOO_KEY_F12;
    key_xlat[VK_ESCAPE] = MOO_KEY_ESCAPE;
    key_xlat[VK_BACK] = MOO_KEY_BACKSPACE;
    key_xlat[VK_TAB] = MOO_KEY_TAB;
    key_xlat[VK_RETURN] = MOO_KEY_RETURN;
    key_xlat[VK_SPACE] = MOO_KEY_SPACE;
    key_xlat[VK_INSERT] = MOO_KEY_INSERT;
    key_xlat[VK_DELETE] = MOO_KEY_DELETE;
    key_xlat[VK_HOME] = MOO_KEY_HOME;
    key_xlat[VK_END] = MOO_KEY_END;
    key_xlat[VK_PRIOR] = MOO_KEY_PAGEUP;
    key_xlat[VK_NEXT] = MOO_KEY_PAGEDOWN;
    key_xlat[VK_LEFT] = MOO_KEY_LEFT;
    key_xlat[VK_RIGHT] = MOO_KEY_RIGHT;
    key_xlat[VK_UP] = MOO_KEY_UP;
    key_xlat[VK_DOWN] = MOO_KEY_DOWN;
    key_xlat[VK_SNAPSHOT] = MOO_KEY_PRINT;
    key_xlat[VK_PAUSE] = MOO_KEY_PAUSE;
    key_xlat[VK_LSHIFT] = MOO_KEY_LSHIFT;
    key_xlat[VK_RSHIFT] = MOO_KEY_RSHIFT;
    key_xlat[VK_LCONTROL] = MOO_KEY_LCTRL;
    key_xlat[VK_RCONTROL] = MOO_KEY_RCTRL;
    key_xlat[VK_LMENU] = MOO_KEY_LALT;
    key_xlat[VK_RMENU] = MOO_KEY_RALT;
    key_xlat[VK_LWIN] = MOO_KEY_LSUPER;
    key_xlat[VK_RWIN] = MOO_KEY_RSUPER;
    key_xlat[VK_APPS] = MOO_KEY_MENU;
    key_xlat[VK_SCROLL] = MOO_KEY_SCROLLOCK;
    key_xlat[VK_NUMLOCK] = MOO_KEY_NUMLOCK;
    key_xlat[VK_CAPITAL] = MOO_KEY_CAPSLOCK;
    key_xlat[VK_OEM_1] = MOO_KEY_SEMICOLON;
    key_xlat[VK_OEM_PLUS] = MOO_KEY_EQUALS;
    key_xlat[VK_OEM_COMMA] = MOO_KEY_COMMA;
    key_xlat[VK_OEM_MINUS] = MOO_KEY_MINUS;
    key_xlat[VK_OEM_PERIOD] = MOO_KEY_PERIOD;
    key_xlat[VK_OEM_2] = MOO_KEY_SLASH;
    key_xlat[VK_OEM_3] = MOO_KEY_BACKQUOTE;
    key_xlat[VK_OEM_4] = MOO_KEY_LEFTBRACKET;
    key_xlat[VK_OEM_5] = MOO_KEY_BACKSLASH;
    key_xlat[VK_OEM_6] = MOO_KEY_RIGHTBRACKET;
    key_xlat[VK_OEM_7] = MOO_KEY_QUOTE;
}

/* -------------------------------------------------------------------------- */

static void hw_mouse_button(int i, bool pressed)
{
    int b = mouse_buttons;
    if (i == 0) {
        if (pressed) {
            b |= MOUSE_BUTTON_MASK_LEFT;
        } else {
            b &= ~MOUSE_BUTTON_MASK_LEFT;
        }
    } else if (i == 1) {
        if (pressed) {
            b |= MOUSE_BUTTON_MASK_RIGHT;
        } else {
            b &= ~MOUSE_BUTTON_MASK_RIGHT;
        }
    }
    mouse_set_buttons_from_hw(b);
}

/* -------------------------------------------------------------------------- */

static LRESULT CALLBACK hw_win32_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
        case WM_CLOSE:
            hw_video_shutdown();
            hwwin32_window_destroy();
            exit(EXIT_SUCCESS);
            break;
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                i_hw_video.render();
                i_hw_video.update();
                EndPaint(hwnd, &ps);
            }
            return 0;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            {
                uint32_t mod = 0;
                mookey_t key;
                char c = 0;
                BYTE keys[256];
                WORD out[2] = { 0, 0 };
                if (GetKeyboardState(keys)) {
                    if (keys[VK_SHIFT] & 0x80) { mod |= MOO_MOD_SHIFT; }
                    if (keys[VK_CONTROL] & 0x80) { mod |= MOO_MOD_CTRL; }
                    if (keys[VK_MENU] & 0x80) { mod |= MOO_MOD_ALT; }
                    if (ToAscii((UINT)wparam, (lparam >> 16) & 0xff, keys, out, 0) == 1) {
                        c = (char)out[0];
                    }
                }
                key = key_xlat[(UINT)wparam];
                if (!kbd_handle_hotkey(key, mod)) {
                    kbd_add_keypress(key, mod, c);
                }
            }
            return 0;
        case WM_KEYUP:
            return 0;
        case WM_SYSKEYUP:
            return 0;
        case WM_MOUSEMOVE:
            mouse_set_xy_from_hw(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            return 0;
        case WM_LBUTTONDOWN:
            hw_mouse_button(0, true);
            return 0;
        case WM_LBUTTONUP:
            hw_mouse_button(0, false);
            return 0;
        case WM_RBUTTONDOWN:
            hw_mouse_button(1, true);
            return 0;
        case WM_RBUTTONUP:
            hw_mouse_button(1, false);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/* -------------------------------------------------------------------------- */

static int hwwin32_register_class(void)
{
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = hw_win32_wndproc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.lpszClassName = HW_WIN32_CLASS_NAME;
    if (!RegisterClass(&wc)) {
        log_error("RegisterClass failed!\n");
        return -1;
    }
    return 0;
}

int hwwin32_window_create(int w, int h)
{
    RECT rect;
    int ww, wh;

    if (hwwin32_register_class()) {
        return -1;
    }
    if (opt_fullscreen) {
        hwwin32_hwnd = CreateWindowEx(WS_EX_TOPMOST, HW_WIN32_CLASS_NAME, HW_WIN32_WINDOW_NAME,
                                      WS_POPUP, 0, 0, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
    } else {
        rect.left = 0;
        rect.top = 0;
        rect.right = w;
        rect.bottom = h;
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE);
        ww = rect.right - rect.left;
        wh = rect.bottom - rect.top;
        hwwin32_hwnd = CreateWindowEx(0, HW_WIN32_CLASS_NAME, HW_WIN32_WINDOW_NAME,
                                      WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
                                      CW_USEDEFAULT, CW_USEDEFAULT, ww, wh,
                                      NULL, NULL, GetModuleHandle(NULL), NULL);
    }
    if (!hwwin32_hwnd) {
        log_error("CreateWindowEx failed!\n");
        return -1;
    }
    ShowWindow(hwwin32_hwnd, SW_SHOW);
    UpdateWindow(hwwin32_hwnd);
    return 0;
}

void hwwin32_window_destroy(void)
{
    if (hwwin32_hwnd) {
        DestroyWindow(hwwin32_hwnd);
        hwwin32_hwnd = NULL;
    }
    UnregisterClass(HW_WIN32_CLASS_NAME, GetModuleHandle(NULL));
}

/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    return main_1oom(argc, argv);
}

int hw_early_init(void)
{
    log_message_direct("win32 backend, GDI/waveOut/mci\n");
    return 0;
}

int hw_init(void)
{
    if (hw_audio_init()) {
        return 12;
    }
    build_key_xlat();
    return 0;
}

void hw_shutdown(void)
{
    hw_audio_shutdown();
    hw_video_shutdown();
    hwwin32_window_destroy();
}

int hw_event_handle(void)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
            case WM_QUIT:
                exit(EXIT_SUCCESS);
                break;
            default:
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                break;
        }
    }
    hw_audio_update();
    return 0;
}

void hw_mouse_toggle_grab(void)
{
    hw_mouse_enabled = !hw_mouse_enabled;
    if (hw_mouse_enabled) {
        ShowCursor(TRUE);
        ClipCursor(NULL);
    } else {
        RECT rect;
        ShowCursor(FALSE);
        GetClientRect(hwwin32_hwnd, &rect);
        ClientToScreen(hwwin32_hwnd, (POINT *)&rect.left);
        ClientToScreen(hwwin32_hwnd, (POINT *)&rect.right);
        ClipCursor(&rect);
    }
}
