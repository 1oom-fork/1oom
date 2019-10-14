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
extern void game_update_eco_on_waste(struct game_s *g, player_id_t player_i, bool force_adjust);
extern void game_update_seen_by_orbit(struct game_s *g, player_id_t pi);
extern void game_update_within_range(struct game_s *g);
extern void game_update_empire_contact(struct game_s *g);
extern void game_update_visibility(struct game_s *g);
extern void game_adjust_slider_group(int16_t *slidertbl, int slideri, int16_t value, int num, const uint16_t *locktbl);
extern int game_get_min_dist(const struct game_s *g, player_id_t player_i, int planet_i);
extern int game_get_pop_growth_max(const struct game_s *g, int planet_i, int max_pop3);
extern int game_get_pop_growth_for_eco(const struct game_s *g, int planet_i, int eco);
extern void game_print_prod_of_total(const struct game_s *g, player_id_t pi, int prod, char *buf);
extern bool game_xy_is_in_nebula(const struct game_s *g, int x, int y);
extern int game_calc_eta_ship(const struct game_s *g, int speed, int x0, int y0, int x1, int y1);
extern int game_calc_eta_trans(const struct game_s *g, int speed, int x0, int y0, int x1, int y1);
extern bool game_transport_dest_ok(const struct game_s *g, const planet_t *p, player_id_t api);
extern void game_rng_step(struct game_s *g);
extern void game_turn_atmos_tform(struct planet_s *p);
extern void game_turn_soil_enrich(struct planet_s *p, int best_tform, bool advanced);

#endif
