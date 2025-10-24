#ifndef INC_1OOM_UIGMAP_H
#define INC_1OOM_UIGMAP_H

#include "game_types.h"
#include "types.h"

struct game_s;

extern bool ui_gmap(struct game_s *g, player_id_t pi);
extern void ui_gmap_basic_draw_only(void *ctx, planet_id_t pi);
extern void ui_gmap_draw_planet_border(const struct game_s *g, planet_id_t planet_i);

#endif
