#ifndef INC_1OOM_UINEWGAME_H
#define INC_1OOM_UINEWGAME_H

#include "types.h"

struct game_new_options_s;
/* returns false on cancel  */
extern bool ui_new_game(struct game_new_options_s *newopts);

#endif
