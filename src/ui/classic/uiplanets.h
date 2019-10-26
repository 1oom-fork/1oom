#ifndef INC_1OOM_UIPLANETS_H
#define INC_1OOM_UIPLANETS_H

#include "game_types.h"

struct game_s;

extern const char *planets_get_notes_str(const struct game_s *g, uint8_t pli, bool *flag_normal_ptr, char *buf, size_t bufsize);
extern void ui_planets(struct game_s *g, player_id_t pi);

#endif
