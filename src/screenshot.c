#include "config.h"

#include "screenshot.h"
#include "fmt_pic.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "types.h"
#include "util.h"

/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */

bool screenshot_save(const uint8_t *screen, const uint8_t *pal, int w, int h)
{
    char *fname = lib_malloc(FSDEV_PATH_MAX);
    struct pic_s pic;
    bool res;
    os_get_fname_screenshot(fname, "pcx");
    pic.type = PIC_TYPE_PCX;
    pic.w = w;
    pic.h = h;
    pic.pitch = w;
    pic.pal = (uint8_t *)pal;
    pic.pix = (uint8_t *)screen;
    res = fmt_pic_save(fname, &pic);
    if (res) {
        log_message("Screenshot: saved '%s'\n", fname);
    } else {
        log_error("Screenshot: failed to save '%s'\n", fname);
    }
    lib_free(fname);
    return res;
}
