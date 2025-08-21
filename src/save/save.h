#ifndef INC_1OOM_SAVE_H
#define INC_1OOM_SAVE_H

#include "types.h"

#define SAVE_MOO13_LEN  59036
#define TBLLEN(_t_) ((int32_t)(sizeof((_t_)) / sizeof((_t_)[0])))   /* FIXME */

struct game_s;
extern void *libsave_1oom_open_check_header(const char *filename, char *savename, uint32_t *version);
extern int libsave_1oom_save_do(const char *filename, const char *savename, const struct game_s *g);
extern int libsave_1oom_load_do(const char *filename, struct game_s *g, char *savename);

extern bool libsave_is_moo13(const char *fname);
extern bool libsave_moo13_check(const struct game_s *g);
extern int libsave_moo13_save_do(const char *filename, const struct game_s *g);
extern int libsave_moo13_load_do(const char *filename, struct game_s *g);


#endif
