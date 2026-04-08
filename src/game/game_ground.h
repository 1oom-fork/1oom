#ifndef INC_1OOM_GAME_GROUND_H
#define INC_1OOM_GAME_GROUND_H

#include "game_types.h"
#include "types.h"

struct game_s;
struct spy_esp_s;

struct ground_side_s {
    player_id_t player;
    bool human;
    int force;
    int pop1;
    int pop2;
    int strnum;
    char str[3][0x40];
};

struct ground_s {
    struct game_s *g;
    planet_id_t planet_i;
    bool flag_swap;
    bool flag_rebel;
    int inbound;
    int total_inbound;
    int death;  /* 0, 1, -1 */
    int fact;
    int techchance;
    struct spy_esp_s *steal;
    struct ground_side_s s[2];
};

extern void game_ground_kill(struct ground_s *gr);
extern void game_ground_finish(struct ground_s *gr);
extern void game_turn_ground(struct game_s *g);

#endif
