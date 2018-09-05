#ifndef INC_1OOM_GAME_DEBUG_H
#define INC_1OOM_GAME_DEBUG_H

#include "config.h"

#ifdef FEATURE_MODEBUG
#include "types.h"
struct game_s;
extern void game_debug_dump_sliders(const struct game_s *g, bool force);
extern void game_debug_dump_race_techs(struct game_s *g, bool force);
extern void game_debug_dump_race_spending(struct game_s *g, bool force);
extern void game_debug_dump_race_waste(struct game_s *g, bool force);
#else
#define game_debug_dump_sliders(_x_, _f_)
#define game_debug_dump_race_techs(_x_, _f_)
#define game_debug_dump_race_spending(_x_, _f_)
#define game_debug_dump_race_waste(_x_, _f_)
#endif

#endif
