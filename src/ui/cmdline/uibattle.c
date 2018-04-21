#include "config.h"

#include "game.h"
#include "game_aux.h"
#include "ui.h"

void ui_battle_init(struct battle_s *bt)
{
}
void ui_battle_shutdown(struct battle_s *bt, bool colony_destroyed)
{
}
void ui_battle_draw_planetinfo(const struct battle_s *bt, bool side_r)
{
}
void ui_battle_draw_scan(const struct battle_s *bt, bool side_r)
{
}
void ui_battle_draw_misshield(const struct battle_s *bt, int target_i, int target_x, int target_y, int missile_i)
{
}
void ui_battle_draw_damage(const struct battle_s *bt, int target_i, int target_x, int target_y, uint32_t damage)
{
}
void ui_battle_draw_explos_small(const struct battle_s *bt, int x, int y)
{
}
void ui_battle_draw_basic(const struct battle_s *bt)
{
}
void ui_battle_draw_basic_copy(const struct battle_s *bt)
{
}
void ui_battle_draw_missile(const struct battle_s *bt, int missilei, int x, int y, int tx, int ty)
{
}
void ui_battle_draw_cloaking(const struct battle_s *bt, int from, int to, int sx, int sy)
{
}
void ui_battle_draw_arena(const struct battle_s *bt, int itemi, int a2)
{
}
void ui_battle_draw_item(const struct battle_s *bt, int itemi, int x, int y)
{
}
void ui_battle_draw_bomb_attack(const struct battle_s *bt, int attacker_i, int target_i, ui_battle_bomb_t bombtype)
{
}
void ui_battle_draw_beam_attack(const struct battle_s *bt, int attacker_i, int target_i, int wpni)
{
}
void ui_battle_draw_stasis(const struct battle_s *bt, int attacker_i, int target_i)
{
}
void ui_battle_draw_pulsar(const struct battle_s *bt, int attacker_i, int ptype, const uint32_t *dmgtbl)
{
}
void ui_battle_draw_stream1(const struct battle_s *bt, int attacker_i, int target_i)
{
}
void ui_battle_draw_stream2(const struct battle_s *bt, int attacker_i, int target_i)
{
}
void ui_battle_draw_blackhole(const struct battle_s *bt, int attacker_i, int target_i)
{
}
void ui_battle_draw_technull(const struct battle_s *bt, int attacker_i, int target_i)
{
}
void ui_battle_draw_repulse(const struct battle_s *bt, int attacker_i, int target_i, int sx, int sy)
{
}
void ui_battle_draw_retreat(const struct battle_s *bt)
{
}
void ui_battle_draw_bottom(const struct battle_s *bt)
{
}
void ui_battle_draw_finish(const struct battle_s *bt)
{
}
void ui_battle_area_setup(const struct battle_s *bt)
{
}
void ui_battle_turn_pre(const struct battle_s *bt)
{
}
void ui_battle_turn_post(const struct battle_s *bt)
{
}
ui_battle_action_t ui_battle_turn(const struct battle_s *bt)
{
    return UI_BATTLE_ACT_AUTO;
}
void ui_battle_ai_pre(const struct battle_s *bt)
{
}
bool ui_battle_ai_post(const struct battle_s *bt)
{
    return false;
}
