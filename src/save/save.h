#ifndef INC_1OOM_SAVE_H
#define INC_1OOM_SAVE_H

#include "types.h"

struct game_s;
extern void *game_save_open_check_header(const char *filename, char *savename);
extern bool game_save_is_1oom(const char *filename);
extern int game_save_do_save_do(const char *filename, const char *savename, const struct game_s *g);
extern int game_save_do_load_do(const char *filename, struct game_s *g);

#endif
