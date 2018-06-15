#ifndef INC_1OOM_GAME_AUDIENCE_H
#define INC_1OOM_GAME_AUDIENCE_H

#include "game_types.h"

struct game_s;

#define AUDIENCE_STR_MAX    7
#define AUDIENCE_BC_MAX     5
#define AUDIENCE_BUF2_POS   320
#define AUDIENCE_CBUF_POS   640

struct audience_s {
    struct game_s *g;
    void *uictx;
    char *buf;
    player_id_t ph;
    player_id_t pa;
    player_id_t pwar;
    player_id_t pstartwar;
    uint8_t mode;   /*0,1,2,6*/
    uint8_t dtype;
    uint8_t dtype_next;
    uint8_t gfxi;   /*0..2*/
    uint8_t musi;   /*0..2*/
    uint8_t num_bc;   /*0..5*/
    int new_trade_bc;
    int16_t tribute_bc;
    tech_field_t tribute_field;
    uint8_t tribute_tech;
    const char *strtbl[AUDIENCE_STR_MAX];
    bool *condtbl;
    uint16_t bctbl[AUDIENCE_BC_MAX];
};

extern void game_audience(struct game_s *g, player_id_t ph, player_id_t pa);

#endif
