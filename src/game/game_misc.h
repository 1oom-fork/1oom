#ifndef INC_1OOM_GAME_MISC_H
#define INC_1OOM_GAME_MISC_H

#include "game_planet.h"
#include "game_types.h"
#include "types.h"

struct game_s;
struct empiretechorbit_s;

extern void game_update_have_reserve_fuel(struct game_s *g);
extern void game_update_maint_costs(struct game_s *g);
extern void game_update_production(struct game_s *g);
extern void game_update_total_research(struct game_s *g);
extern void game_update_eco_on_waste(struct game_s *g, int player_i, bool a4);
extern void game_update_seen_by_orbit(struct game_s *g, player_id_t pi);
extern void game_update_within_range(struct game_s *g);
extern void game_update_empire_within_range(struct game_s *g);
extern void game_update_visibility(struct game_s *g);
extern void game_adjust_slider_group(int16_t *slidertbl, int slideri, int16_t value, int num, const uint16_t *locktbl);
extern int game_get_min_dist(const struct game_s *g, int player_i, int planet_i);
extern int game_adjust_prod_by_special(int prod, planet_special_t special);
extern int game_get_pop_growth_max(const struct game_s *g, int planet_i, int max_pop3);
extern int game_get_pop_growth_for_eco(const struct game_s *g, int planet_i, int eco);
extern int game_get_tech_prod(int prod, int slider, race_t race, planet_special_t special);
extern void game_print_prod_of_total(struct game_s *g, player_id_t pi, int prod, char *buf);
extern bool game_xy_is_in_nebula(const struct game_s *g, int x, int y);
extern int game_calc_eta(const struct game_s *g, int speed, int x0, int y0, int x1, int y1);
extern bool game_transport_dest_ok(const struct game_s *g, const planet_t *p, player_id_t api);

#endif
