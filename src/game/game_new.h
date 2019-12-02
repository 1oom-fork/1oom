#ifndef INC_1OOM_GAME_NEW_H
#define INC_1OOM_GAME_NEW_H

#include "game.h"
#include "game_options.h"

#define GALAXY_BORDER_LEFT    10
#define GALAXY_BORDER_RIGHT   27
#define GALAXY_BORDER_TOP     8
#define GALAXY_BORDER_BOTTOM  25

#define GALAXY_AUX_MAX 64

typedef struct star_s {
    union {
        uint64_t info;
        struct {
            uint16_t y;
            uint16_t x;
            uint16_t z;
            uint16_t t;
        };
    };
} star_t;

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
    uint8_t stars;
    uint8_t naux;
    int16_t height;
    int16_t width;
    union {
        newopts_t opt;
        uint8_t popt[GAMEOPTS];
    };
    star_t aux[GALAXY_AUX_MAX];
    star_t star[PLANETS_MAX];
};

extern optdescr_t newopt_descr[NEWOPTS];

#define GAME_NEW_OPTS_DEFAULT \
    { \
        0, GALAXY_SIZE_SMALL, DIFFICULTY_SIMPLE, GAME_AI_DEFAULT, 2, \
        { \
            { "", "", RACE_RANDOM, BANNER_RANDOM, false }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }, \
            { "", "", RACE_RANDOM, BANNER_RANDOM, true }  \
        }, \
        0, 0, 0, 0, \
        { NEWOPTS_DEFAULTS } \
    }

struct game_aux_s;

extern int game_new(struct game_s *g, struct game_new_options_s *opt);
extern int game_new_tutor(struct game_s *g);

extern void game_new_generate_emperor_name(race_t race, char *buf, size_t bufsize);
extern void game_new_generate_home_name(race_t race, char *buf, size_t bufsize);

extern void game_new_generate_other_emperor_name(struct game_s *g, player_id_t player);

#endif
