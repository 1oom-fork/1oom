#ifndef INC_1OOM_SAVECONV_H
#define INC_1OOM_SAVECONV_H

#include "types.h"

struct game_s;

extern int saveconv_init(void);
extern void saveconv_shutdown(void);

extern bool saveconv_is_moo13(struct game_s *g, const char *fname);
extern int saveconv_de_moo13(struct game_s *g, const char *fname);
extern int saveconv_en_moo13(struct game_s *g, const char *fname);
extern int saveconv_de_1oom0(struct game_s *g, const char *fname);
extern int saveconv_en_1oom0(struct game_s *g, const char *fname);
extern bool saveconv_is_text(struct game_s *g, const char *fname);
extern int saveconv_de_text(struct game_s *g, const char *fname);
extern int saveconv_en_text(struct game_s *g, const char *fname);

#endif
