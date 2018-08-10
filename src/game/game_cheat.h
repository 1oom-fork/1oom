#ifndef INC_1OOM_GAME_CHEAT_H
#define INC_1OOM_GAME_CHEAT_H

#include "game_types.h"
#include "types.h"

struct game_s;

extern bool game_cheat_galaxy(struct game_s *g, player_id_t pi);
extern bool game_cheat_events(struct game_s *g, player_id_t pi);
extern bool game_cheat_moola(struct game_s *g, player_id_t pi);
extern bool game_cheat_traits(struct game_s *g, player_id_t pi);

#endif
