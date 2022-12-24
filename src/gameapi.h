#ifndef INC_1OOM_GAMEAPI_H
#define INC_1OOM_GAMEAPI_H

#include "cfg.h"
#include "types.h"

extern const struct cfg_items_s game_cfg_items[];
extern const struct cfg_items_s game_new_cfg_items[];

extern bool game_opt_enable_bomb_animation;
extern bool game_opt_skip_intro_always;
extern bool game_opt_random_news;

extern bool game_num_patch(const char *numid, const int32_t *patchnums, int first, int num);
extern bool game_str_patch(const char *strid, const char *patchstr, int i);
extern void game_apply_ruleset(void);

#endif
