#ifndef INC_1OOM_UISWITCH_H
#define INC_1OOM_UISWITCH_H

#include "game_types.h"
#include "types.h"

struct game_s;

extern bool ui_switch(struct game_s *g, player_id_t *tbl_pi, int num_pi, bool allow_opts);

#endif
