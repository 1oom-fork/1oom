#ifndef INC_1OOM_GFXLIMITS_H
#define INC_1OOM_GFXLIMITS_H

#include "types.h"

extern int gfxlim_minx;
extern int gfxlim_miny;
extern int gfxlim_maxx;
extern int gfxlim_maxy;

static inline void gfxlim_set(int minx, int miny, int maxx, int maxy)
{
    gfxlim_minx = minx;
    gfxlim_miny = miny;
    gfxlim_maxx = maxx;
    gfxlim_maxy = maxy;
}

#endif
