#ifndef INC_1OOM_UIBATTLEPRE_H
#define INC_1OOM_UIBATTLEPRE_H

#include "types.h"

struct game_s;

extern bool ui_battle_pre(struct game_s *g, int party_u, int party_d, uint8_t planet_i, bool flag_human_att, bool hide_other);

#endif
