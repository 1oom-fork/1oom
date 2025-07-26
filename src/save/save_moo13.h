#ifndef INC_1OOM_SAVE_MOO13_H
#define INC_1OOM_SAVE_MOO13_H

#include "types.h"

struct game_s;

extern bool libsave_moo13_check(struct game_s *g, const char *fname);
extern int libsave_moo13_do_load(const char *filename, struct game_s *g, int savei);

#endif
