#ifndef INC_1OOM_GAME_OPTIONS_H
#define INC_1OOM_GAME_OPTIONS_H

#include <stdint.h>

#define GAMEOPTS sizeof(gameopts_t)
#define MAXOPTS 9

typedef struct gameopts_s {
    uint8_t lock;
    uint8_t deterministic;
    uint8_t council;
    uint8_t guardian;
    uint8_t retreat;
    uint8_t spec_war;
    uint8_t fix_bait_yoyo;
    uint8_t nebula_speed;
    uint8_t enforce_nap;
    uint8_t threats;
} gameopts_t;

#define GALAXYOPTS 7
#define PLANETOPTS 6
#define NEWOPTS (GALAXYOPTS+PLANETOPTS)
#define NEWOPTS_DEFAULTS { 1,  1, 2,  0,  3,  0,  0,  1,  1,  1,  3,  2,  1 }

typedef struct newopts_s {
    uint8_t density;
    uint8_t aspect;
    uint8_t gaps;
    uint8_t cluster;
    uint8_t neb;
    uint8_t homeworlds;
    uint8_t start;

    uint8_t psize;
    uint8_t env;
    uint8_t res;
    uint8_t ultra;
    uint8_t artefacts;
    uint8_t astroids;
} newopts_t;

typedef struct optdescr_s {
    int opts;
    int dflt;
    const char *name;
    const char *opt[MAXOPTS];
    const char *descr;
} optdescr_t;

extern optdescr_t gameopt_descr[GAMEOPTS];
extern optdescr_t newopt_descr[NEWOPTS];



#endif
