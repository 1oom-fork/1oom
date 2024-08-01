#ifndef INC_1OOM_UIOBJ_LIMITS_H
#define INC_1OOM_UIOBJ_LIMITS_H

#define UI_SCREEN_W 320
#define UI_SCREEN_H 200

#define UI_STARMAP_LIMITS 6, 6, 221, 177

/* HACK for lbxgfx_draw_frame_offs params */
extern int uiobj_minx;
extern int uiobj_miny;
extern int uiobj_maxx;
extern int uiobj_maxy;

extern void uiobj_set_limits(int minx, int miny, int maxx, int maxy);
extern void uiobj_set_limits_all(void);

#endif
