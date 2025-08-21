#ifndef INC_1OOM_SAVE_H
#define INC_1OOM_SAVE_H

#include "types.h"

struct game_s;
extern void *libsave_1oom_open_check_header(const char *filename, char *savename);
extern bool libsave_is_1oom(const char *filename);
extern int libsave_1oom_save_do(const char *filename, const char *savename, const struct game_s *g);
extern int libsave_1oom_load_do(const char *filename, struct game_s *g);

#endif
