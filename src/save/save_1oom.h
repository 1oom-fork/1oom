#ifndef INC_1OOM_SAVE_1OOM_H
#define INC_1OOM_SAVE_1OOM_H

#include "types.h"

struct game_s;
extern bool game_save_check_header(const char *filename, int i);
extern int game_save_do_save_do(const char *filename, const char *savename, const struct game_s *g, int savei);
extern int game_save_do_load_do(const char *filename, struct game_s *g, int savei);

#endif
