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

/* Enable space scanners for ships.
In MOO1 space scanners on ships are disabled. */
extern bool game_fix_space_scanners;

/* Fix spy cost progression.
MOO1 does not reset spycost between target players and spies become
disproportionately more expensive for each subsequent opponent. */
extern bool game_fix_spy_cost;

/* Continue sending colony ships when there are more than three of them.
In MOO1, 4th colony ship disables sending colony ships. */
extern bool game_ai_fix_4th_colony_curse;

/* Use correct formula for first tech cost.
In MOO1, AI ​​uses a fixed formula that makes early technologies too
expensive and does not take racial bonuses into account. */
extern bool game_ai_fix_first_tech_cost;

/* Fix ship slider accumulation.
In MOO1, at some point ship slider starts doubling.
This bug radically changes the game balance. */
extern bool game_ai_fix_ship_slider;

/* Fix threatened AI spy hiding.
In MOO1, when a threat is successful,
it is the player's spies that hide, not the AI. */
extern bool game_ai_fix_spy_hiding;

/* Fix transport range.
In MOO1, AI can transport population beyond its fuel range. */
extern bool game_ai_fix_transport_range;

extern void game_enable_fix_bugs(void);
extern void game_enable_fair_ai(void);

#endif
