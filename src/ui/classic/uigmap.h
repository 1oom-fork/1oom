#ifndef INC_1OOM_UIGMAP_H
#define INC_1OOM_UIGMAP_H

#include "game_types.h"
#include "types.h"

struct game_s;

extern bool ui_gmap(struct game_s *g, player_id_t pi);
extern void ui_gmap_basic_draw_only(void *ctx, int pi);
extern void ui_gmap_draw_planet_border(void *ctx, const struct game_s *g, uint8_t planet_i);
extern void ui_gmap_draw_planet_flag(void *ctx, const struct game_s *g, uint8_t planet_i);
extern int16_t ui_gmap_add_planet_mousearea(void *ctx, const struct game_s *g, uint8_t planet_i);

#endif
