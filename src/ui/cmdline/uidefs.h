#ifndef INC_1OOM_UIDEFS_H
#define INC_1OOM_UIDEFS_H

#include "boolvec.h"
#include "game_types.h"
#include "types.h"

#define UI_INPUT_TOKEN_MAX  32

struct game_s;

struct input_token_s;

struct input_cmd_s {
    const char *str_cmd;
    const char *str_param;
    const char *str_help;
    int num_param_min;
    int num_param_max;
    int flags;
    int (*handle)(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
    void *var;
};

struct input_token_s {
    const char *str;
    union {
        void *ptr;
        const struct input_cmd_s *cmd;
        int num;
    } data;
    enum {
        INPUT_TOKEN_NONE,
        INPUT_TOKEN_UNKNOWN,
        INPUT_TOKEN_COMMAND,
        INPUT_TOKEN_NUMBER,
        INPUT_TOKEN_RELNUMBER
    } type;
};

struct ui_data_s {
    struct {
        struct input_token_s tok[UI_INPUT_TOKEN_MAX];
        int num;
    } input;
    struct {
        char screen[8 * 3][10 * 4 + 7];
    } battle;
    struct {
        uint32_t item[PLANETS_MAX + FLEET_ENROUTE_MAX + TRANSPORT_MAX];
    } view;
    BOOLVEC_DECLARE(players_viewing, PLAYER_NUM);
    char strbuf[1024];
};

extern struct ui_data_s ui_data;

#endif
