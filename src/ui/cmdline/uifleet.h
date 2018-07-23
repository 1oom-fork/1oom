#ifndef INC_1OOM_UIFLEET_H
#define INC_1OOM_UIFLEET_H

#include "types.h"

struct game_s;
struct input_token_s;

extern int ui_cmd_fleet_list(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_fleet_redir(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_fleet_send(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_fleet_scrap(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);

#endif
