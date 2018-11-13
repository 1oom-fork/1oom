#ifndef INC_1OOM_GAME_FIX_H
#define INC_1OOM_GAME_FIX_H

#include "types.h"

/* The 2500 factories limit is too low for Meklars */
extern bool game_fix_max_factories;

/* Allow orbital bombardment with any weapon type.
In MOO1 neither Death Ray, Crystal Ray nor Amoeba Stream can be used for
orbital bombardment, but can destroy a colony in space combat. */
extern bool game_fix_orbital_weap_any;

/* Fix orbital bombardment missile/torpedo damage halving.
In MOO1 the missile damage is halved but torpedo damage is untouched. */
extern bool game_fix_orbital_torpedo;

/* Fix orbital bombardment undeserved battle computer bonus.
In MOO1 the bonus is given for the best battle computer in any ship design
regardless of if any such ships are present in the fleet in orbit. */
extern bool game_fix_orbital_comp;

/* Fix sliders not adjusting correctly after building a Stargate. */
extern bool game_fix_sg_finished;

/* Fix spy cost progression.
MOO1 does not reset spycost between target players and spies become
disproportionately more expensive for each subsequent opponent. */
extern bool game_fix_spy_cost;

#endif
