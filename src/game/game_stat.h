#ifndef INC_1OOM_GAME_STAT_H
#define INC_1OOM_GAME_STAT_H

#include "game_types.h"

struct game_s;

extern int game_stat_fleet(const struct game_s *g, player_id_t pi);
extern int game_stat_tech(const struct game_s *g, player_id_t pi);
extern int game_stat_prod(const struct game_s *g, player_id_t pi);
extern int game_stat_pop(const struct game_s *g, player_id_t pi);
extern int game_stat_planets(const struct game_s *g, player_id_t pi);

#endif
