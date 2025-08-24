#ifndef INC_1OOM_GAME_FIX_H
#define INC_1OOM_GAME_FIX_H

#include "types.h"

/* Allow orbital bombardment with any weapon type.
In MOO1 neither Death Ray, Crystal Ray nor Amoeba Stream can be used for
orbital bombardment, but can destroy a colony in space combat. */
extern bool game_fix_orbital_weap_any;

/* Allow orbital bombardment with weapon in slot 4.
In MOO1 the weapon in slot 4 is not used in orbital bombardment. */
extern bool game_fix_orbital_weap_4;

/* Fix orbital bombardment missile/torpedo damage halving.
In MOO1 the missile damage is halved but torpedo damage is untouched. */
extern bool game_fix_orbital_torpedo;

/* Fix orbital bombardment undeserved battle computer bonus.
In MOO1 the bonus is given for the best battle computer in any ship design
regardless of if any such ships are present in the fleet in orbit. */
extern bool game_fix_orbital_comp;

/* Enable space scanners for ships.
In MOO1 space scanners on ships are disabled. */
extern bool game_fix_space_scanners;

extern void game_enable_fix_bugs(void);

#endif
