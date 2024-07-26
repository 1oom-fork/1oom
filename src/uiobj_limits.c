#include "comp.h"
#include "uiobj_limits.h"

int uiobj_minx = 0;
int uiobj_miny = 0;
int uiobj_maxx = UI_SCREEN_W - 1;
int uiobj_maxy = UI_SCREEN_H - 1;

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
