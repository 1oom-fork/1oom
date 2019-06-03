#ifndef INC_1OOM_GAME_AI_H
#define INC_1OOM_GAME_AI_H

#include "game_types.h"
#include "types.h"

struct game_s;
struct battle_s;
struct election_s;

struct game_ai_s {
    char const * const name;
    void (*turn_p1)(struct game_s *g);
    void (*turn_p2)(struct game_s *g);
    void (*turn_p3)(struct game_s *g);
    bool (*battle_ai_ai_resolve)(struct battle_s *bt); /* true if right side won */
    void (*battle_ai_turn)(struct battle_s *bt);
    bool (*battle_ai_retreat)(struct battle_s *bt); /* true if retreat all */
    uint8_t (*tech_next)(struct game_s *g, player_id_t player, tech_field_t field, uint8_t *tbl, int num);
    bool (*bomb)(struct game_s *g, player_id_t player, uint8_t planet, int pop_inbound);
    void (*plague)(struct game_s *g, uint8_t planet);
    void (*nova)(struct game_s *g, uint8_t planet);
    void (*comet)(struct game_s *g, uint8_t planet);
    void (*pirates)(struct game_s *g, uint8_t planet);
    int (*vote)(struct election_s *el, player_id_t player); /* 0 = abstain, N = candidate N */
    void (*turn_diplo_p1)(struct game_s *g);
    void (*turn_diplo_p2)(struct game_s *g);
};

extern const struct game_ai_s *game_ai;

#endif
