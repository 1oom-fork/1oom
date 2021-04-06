#include "types.h"
#include "comp.h"
#include "hwsdl_aspect.h"
#include "hwsdl_opt.h"

void shrink_to_aspect_ratio(int wp[1], int hp[1], int rx, int ry)
{
    /* NOTE if window size >= 4295 then 32-bit unsigned multiply overflows
     * this is a real possibility if 8K monitors become a thing in the future */
    typedef int64_t T;
    T w = *wp * (T) ry;
    T h = *hp * (T) rx;
    w = h = MIN(w, h); /* could also use MAX. or average */
    *wp = w / ry;
    *hp = h / rx;
}

const char *hw_uiopt_cb_aspect_get(void)
{
    if (hw_opt_aspect == HW_DEFAULT_ASPECT) {
        return "VGA";
    } else if (hw_opt_aspect == 1000000) {
        return "1:1";
    } else if (hw_opt_aspect == 0) {
        return "Off";
    } else {
        return "Custom";
    }
}

bool hw_uiopt_cb_aspect_next(void)
{
    if (hw_opt_aspect == HW_DEFAULT_ASPECT) {
        hw_opt_aspect = 1000000;
    } else if (hw_opt_aspect == 1000000) {
        hw_opt_aspect = 0;
    } else {
        hw_opt_aspect = HW_DEFAULT_ASPECT;
    }
    return hw_video_update_aspect();
}

