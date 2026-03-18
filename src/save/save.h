#ifndef INC_1OOM_SAVE_H
#define INC_1OOM_SAVE_H

#include "types.h"

#define SAVE_MOO13_LEN  59036
#define TBLLEN(_t_) ((int32_t)(sizeof((_t_)) / sizeof((_t_)[0])))   /* FIXME */

struct game_s;

extern bool libsave_is_moo13(const char *fname);
extern bool libsave_moo13_check(const struct game_s *g);
extern int libsave_moo13_save_do(const char *filename, const struct game_s *g);
extern int libsave_moo13_load_do(const char *filename, struct game_s *g);


#endif
