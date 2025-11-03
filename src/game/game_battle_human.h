#ifndef INC_1OOM_GAME_BATTLE_HUMAN_H
#define INC_1OOM_GAME_BATTLE_HUMAN_H

#include "types.h"
#include "game_types.h"

#define BATTLE_XY_SET(_x_, _y_) (((_x_) << 3) + (_y_))
#define BATTLE_XY_GET_X(_v_)    (((_v_) >> 3) & 0xf)
#define BATTLE_XY_GET_Y(_v_)    ((_v_) & 7)
#define BATTLE_XY_INVALID       0xff

struct battle_s;

extern void game_battle_area_setup(struct battle_s *bt);
extern int game_battle_area_check_line_ok(struct battle_s *bt, int *tblx, int *tbly, int len);
extern void game_battle_item_move(struct battle_s *bt, battle_item_id_t itemi, int sx, int sy);
extern int game_battle_get_xy_notsame(const struct battle_s *bt, battle_item_id_t item1, battle_item_id_t item2, int *x_notsame);
extern bool game_battle_attack(struct battle_s *bt, battle_item_id_t attacker_i, battle_item_id_t target_i, bool retaliate);

extern bool game_battle_with_human(struct battle_s *bt); /* true if right side won */

#endif
