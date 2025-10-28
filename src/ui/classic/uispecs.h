#ifndef INC_1OOM_UISPECS_H
#define INC_1OOM_UISPECS_H

#include "game_types.h"

struct game_s;

extern shipdesign_id_t ui_specs(struct game_s *g, player_id_t pi);
extern void ui_specs_before(struct game_s *g, player_id_t pi);
extern void ui_specs_mustscrap(struct game_s *g, player_id_t pi, shipdesign_id_t scrapi);

#endif
