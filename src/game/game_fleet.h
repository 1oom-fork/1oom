#ifndef INC_1OOM_GAME_FLEET_H
#define INC_1OOM_GAME_FLEET_H

#include "boolvec.h"
#include "game_types.h"
#include "types.h"

struct game_s;
struct planet_s;
struct fleet_enroute_s;

typedef struct fleet_enroute_s {
    player_id_t owner;
    uint16_t x;
    uint16_t y;
    uint8_t dest;   /* planet index */
    uint8_t speed;
    bool retreat;
    BOOLVEC_DECLARE(visible, PLAYER_NUM);
    shipcount_t ships[NUM_SHIPDESIGNS];
} fleet_enroute_t;

#define FLEET_ENROUTE_AI_MAX    208
#define FLEET_ENROUTE_MAX   260

typedef struct transport_s {
    player_id_t owner;
    uint16_t x;
    uint16_t y;
    uint8_t dest;   /* planet index */
    uint8_t speed;
    BOOLVEC_DECLARE(visible, PLAYER_NUM);
    uint16_t pop;
} transport_t;

#define TRANSPORT_MAX   100

typedef struct fleet_orbit_s {
    BOOLVEC_DECLARE(visible, PLAYER_NUM);
    shipcount_t ships[NUM_SHIPDESIGNS];
} fleet_orbit_t;

#define FLEET_SPEED_STARGATE    35

extern bool game_send_fleet_from_orbit(struct game_s *g, player_id_t owner, uint8_t from, uint8_t dest, const shipcount_t ships[NUM_SHIPDESIGNS], const uint8_t shiptypes[NUM_SHIPDESIGNS], uint8_t numtypes);
extern bool game_send_fleet_retreat(struct game_s *g, player_id_t owner, uint8_t from, uint8_t dest, const shipcount_t ships[NUM_SHIPDESIGNS], const uint8_t shiptypes[NUM_SHIPDESIGNS], uint8_t numtypes);
extern bool game_send_fleet_reloc(struct game_s *g, player_id_t owner, uint8_t from, uint8_t dest, uint8_t si, shipcount_t shipnum);
extern bool game_send_transport(struct game_s *g, struct planet_s *p);
extern void game_remove_empty_fleets(struct game_s *g);
extern void game_remove_player_fleets(struct game_s *g, player_id_t owner);
extern bool game_fleet_any_dest_player(const struct game_s *g, player_id_t owner, player_id_t target);
extern void game_fleet_unrefuel(struct game_s *g);
extern uint8_t game_fleet_get_speed(const struct game_s *g, const struct fleet_enroute_s *r, uint8_t pfrom, uint8_t pto);
extern void game_fleet_redirect(struct game_s *g, struct fleet_enroute_s *r, uint8_t pfrom, uint8_t pto);

#endif
