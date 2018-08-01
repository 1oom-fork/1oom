#ifndef INC_1OOM_UIFINISHED_H
#define INC_1OOM_UIFINISHED_H

#include "types.h"

struct game_s;
struct input_token_s;

extern void ui_finished_print_all(struct game_s *g, int api);
extern int ui_cmd_msg_filter(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);

#endif
