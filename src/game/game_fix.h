#ifndef INC_1OOM_GAME_FIX_H
#define INC_1OOM_GAME_FIX_H

#include "types.h"

/* Remove broken redundant code. */
extern bool game_fix_broken_garbage;

/* At least 5% of missile will always hit (was 6%) */
extern bool game_fix_bt_min_missile_hit;

/* Yes, dead AI design ships in MOO1.
Fixing that is enough to avoid some hangs against Repulsor Beam. */
extern bool game_fix_dead_ai_designs_ships;

/* Give Guardian Advanced Damage Control on impossible.
MOO1 has Automated Repair (but shows ADC) on hard and nothing on impossible. */
extern bool game_fix_guardian_repair;

/* The 2500 factories limit is too low for Meklars. */
extern bool game_fix_max_factories;

/* Fix (enable) Oracle Interface. */
extern bool game_fix_oracle_interface;

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

/* 8th stargate bug corrected (no ship maintenance overflow). */
extern bool game_fix_sg_maint_overflow;

/* Silicoids have most of the tech useless to them not appear in their
research tree. However, Advanced Eco Restoration and
Advanced Soil Enrichment can still appear. This must be an oversight. */
extern bool game_fix_silicoid_tech;

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

/* Entering the general "threaten / break treaty" dialogue no more equals
to a penalty of -100 to all temporary diplomatic. */
extern bool game_ai_fix_cancelled_threat;

/* The AI intentionally aggressive fleet positioning during the final war
now actually works. */
extern bool game_ai_fix_final_war_fronts;

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

#endif
