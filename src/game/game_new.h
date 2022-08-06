#ifndef INC_1OOM_GAME_NEW_H
#define INC_1OOM_GAME_NEW_H

#include "game.h"

typedef enum {
    GAME_NEW_SPACE_COMBAT_MOO_1_3 = 0,
    GAME_NEW_SPACE_COMBAT_FAIR_1_3,
    GAME_NEW_SPACE_COMBAT_BALANCED,
    GAME_NEW_SPACE_COMBAT_NUM,
} game_new_space_combat_rules_t;

struct game_new_options_s {
    uint32_t galaxy_seed;
    galaxy_size_t galaxy_size;
    difficulty_t difficulty;
    uint8_t ai_id;
    uint8_t players;
    uint8_t no_elections;
    uint8_t space_combat_rules;
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
        0, GALAXY_SIZE_SMALL, DIFFICULTY_SIMPLE, GAME_AI_DEFAULT, 2, \
        false, GAME_NEW_SPACE_COMBAT_BALANCED, \
        { \
            { "", "", RACE_HUMAN, BANNER_BLUE, false }, \
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

extern void game_new_generate_emperor_name(race_t race, char *buf, size_t bufsize);
extern void game_new_generate_home_name(race_t race, char *buf, size_t bufsize);

extern void game_new_generate_other_emperor_name(struct game_s *g, player_id_t player);

#endif
