#ifndef INC_1OOM_GAME_NEW_H
#define INC_1OOM_GAME_NEW_H

#include "game.h"

struct game_new_options_s {
    uint32_t galaxy_seed;
    galaxy_size_t galaxy_size;
    difficulty_t difficulty;
    uint8_t ai_id;
    uint8_t players;
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
        0, GALAXY_SIZE_SMALL, DIFFICULTY_SIMPLE, 0, 2, \
        { \
            { "", "", RACE_RANDOM, BANNER_RANDOM, false }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }  \
        } \
    }

struct game_aux_s;

extern int game_new(struct game_s *g, struct game_aux_s *gaux, struct game_new_options_s *opt);
extern int game_new_tutor(struct game_s *g, struct game_aux_s *gaux);

extern void game_new_generate_emperor_name(race_t race, char *buf);
extern void game_new_generate_home_name(race_t race, char *buf);

extern void game_new_generate_other_emperor_name(struct game_s *g, player_id_t player);

#endif
