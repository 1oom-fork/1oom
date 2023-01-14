#ifndef INC_1OOM_GAME_NEW_H
#define INC_1OOM_GAME_NEW_H

#include "game.h"

struct game_new_options_s {
    uint32_t galaxy_seed;
    galaxy_size_t galaxy_size;
    difficulty_t difficulty;
    uint32_t ai_id;
    uint32_t players;
    uint32_t no_elections;
    uint32_t no_tohit_acc;
    uint32_t precap_tohit;
    uint32_t no_events;
    uint32_t population_growth_fix;
    uint32_t factory_cost_fix;
    uint32_t waste_calc_fix;
    bool players_distance_fix;
    bool fix_homeworld_satellites;
    bool fix_bad_satellites;
    research_rate_t research_rate;
    struct {
        char playername[EMPEROR_NAME_LEN];
        char homename[PLANET_NAME_LEN];
        race_t race;
        banner_t banner;
        bool is_ai;
    } pdata[PLAYER_NUM];
};

#define GAME_NEW_OPTS_DEFAULT \
    { \
        0, GALAXY_SIZE_SMALL, DIFFICULTY_SIMPLE, GAME_AI_DEFAULT, 2, false, true, true, false, true, true, true, false, false, false, RESEARCH_RATE_NORMAL, \
        { \
            { "", "", RACE_HUMAN, BANNER_BLUE, false }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }  \
        } \
    }

extern struct game_new_options_s game_opt_custom;
extern uint32_t game_opt_race_value;
extern uint32_t game_opt_banner_value;
extern uint32_t game_opt_isai_value;

struct game_aux_s;

extern int game_new(struct game_s *g, struct game_aux_s *gaux, struct game_new_options_s *opt);
extern int game_new_tutor(struct game_s *g, struct game_aux_s *gaux);

extern void game_new_generate_emperor_name(race_t race, char *buf, size_t bufsize);
extern void game_new_generate_home_name(race_t race, char *buf, size_t bufsize);

extern void game_new_generate_other_emperor_name(struct game_s *g, player_id_t player);

#endif
