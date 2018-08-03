#ifndef INC_1OOM_UIGOVERN_H
#define INC_1OOM_UIGOVERN_H

#include "types.h"

struct game_s;
struct input_token_s;

extern int ui_cmd_govern_toggle(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_govern_readjust(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_govern_readjust_all(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_govern_bases(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_govern_rest(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_govern_sg_toggle(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_govern_eco_mode(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);
extern int ui_cmd_govern_opts(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var);

#endif
