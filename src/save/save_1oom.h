#ifndef INC_1OOM_SAVE_1OOM_H
#define INC_1OOM_SAVE_1OOM_H

#include "types.h"

struct game_s;
extern bool libsave_1oom_check_header(const char *filename, int i);
extern int libsave_1oom_do_save(const char *filename, const char *savename, const struct game_s *g, int savei);
extern int libsave_1oom_do_load(const char *filename, struct game_s *g, int savei);

#endif
