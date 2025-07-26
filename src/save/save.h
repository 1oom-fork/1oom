#ifndef INC_1OOM_SAVE_H
#define INC_1OOM_SAVE_H

#include "save_1oom.h"
#include "save_moo13.h"

#include "types.h"

#define NUM_SAVES   6
#define NUM_ALL_SAVES   (NUM_SAVES + 1/*continue game*/ + 1/*undo*/)
#define SAVE_NAME_LEN   20

#define GAME_SAVE_I_CONTINUE    (7 - 1)
#define GAME_SAVE_I_UNDO        (8 - 1)

extern bool game_save_tbl_have_save[NUM_ALL_SAVES];
extern char game_save_tbl_name[NUM_ALL_SAVES][SAVE_NAME_LEN];

extern void libsave_init(void);
extern void libsave_shutdown(void);

extern const char *game_save_get_slot_fname(int i);
extern int game_save_check_saves(void);

struct game_s;
extern int game_save_do_load_fname(const char *filename, char *savename, struct game_s *g);
extern int game_save_do_load_i(int savei/*0..NUM_ALL_SAVES-1*/, struct game_s *g);
extern int game_save_do_save_i(int savei/*0..NUM_ALL_SAVES-1*/, const char *savename, const struct game_s *g);

#endif
