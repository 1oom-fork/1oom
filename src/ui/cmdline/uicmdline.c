#include "config.h"

#include <stdio.h>

#include "ui.h"
#include "game.h"
#include "game_end.h"
#include "game_turn.h"
#include "log.h"
#include "options.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const struct cmdline_options_s ui_cmdline_options[] = {
    { NULL, 0, NULL, NULL, NULL, NULL }
};

/* -------------------------------------------------------------------------- */

const char *idstr_ui = "cmdline";

bool ui_use_audio = false;

/* -------------------------------------------------------------------------- */

int ui_early_init(void)
{
    return 0;
}

int ui_init(void)
{
    return 0;
}

int ui_late_init(void)
{
    return 0;
}

void ui_shutdown(void)
{
}

char *ui_get_strbuf(void)
{
    return NULL;
}

void ui_play_intro(void)
{
    fputs("\n\nMicroProse presents...\n\na Simtex Software production...\n\n*** Master of Orion ***\n\n\n", stdout);
}

void ui_play_ending_good(int race, const char *name)
{
    fputs("\n", stdout);
    fputs(name, stdout);
    fputs(" won! :)\n\n", stdout);
}

void ui_play_ending_tyrant(int race, const char *name)
{
    fputs("\n", stdout);
    fputs(name, stdout);
    fputs(" won... :/\n\n", stdout);
}

void ui_play_ending_funeral(int p0, int p2)
{
    fputs("\nFuneral. :(\n\n", stdout);
}

void ui_play_ending_exile(const char *name)
{
    fputs("\n", stdout);
    fputs(name, stdout);
    fputs(" was exiled. >:I\n\n", stdout);
}

void ui_sound_play_sfx(int sfxi)
{
}

int ui_spy_steal(struct game_s *g, int spy, int target, uint8_t flags_field)
{
    return -1;
}

void ui_spy_stolen(struct game_s *g, int pi, int spy, int field, uint8_t tech)
{
}

ui_sabotage_t ui_spy_sabotage_ask(struct game_s *g, int spy, int target, uint8_t *planetptr)
{
    return UI_SABOTAGE_NONE;
}

int ui_spy_sabotage_done(struct game_s *g, int pi, int spy, int target, ui_sabotage_t act, int other1, int other2, uint8_t planet, int snum)
{
    return PLAYER_NONE;
}

void ui_newtech(struct game_s *g, int pi)
{
}

bool ui_explore(struct game_s *g, int pi, uint8_t planet_i, bool by_scanner, bool flag_colony_ship)
{
    return flag_colony_ship;
}

bool ui_bomb_ask(struct game_s *g, int pi, uint8_t planet_i, int pop_inbound)
{
    return true;
}

void ui_bomb_show(struct game_s *g, int attacker_i, int owner_i, uint8_t planet_i, int popdmg, int factdmg, bool play_music)
{
}

void ui_turn_pre(const struct game_s *g)
{
}

void ui_turn_msg(struct game_s *g, int pi, const char *str)
{
    fputs(str, stdout);
    fputs("\n", stdout);
}

void ui_ground(struct ground_s *gr)
{
}

void ui_news_start(void)
{
}

void ui_news(struct game_s *g, struct news_s *ns)
{
}

void ui_news_end(void)
{
}

void ui_election_start(struct election_s *el)
{
}

void ui_election_show(struct election_s *el)
{
}

void ui_election_delay(struct election_s *el, int delay)
{
}

int ui_election_vote(struct election_s *el, int player_i)
{
    return 0;
}

bool ui_election_accept(struct election_s *el, int player_i)
{
    return true;
}

void ui_election_end(struct election_s *el)
{
}

void ui_audience_start(struct audience_s *au)
{
}

void ui_audience_show1(struct audience_s *au)
{
}

void ui_audience_show2(struct audience_s *au)
{
}

void ui_audience_show3(struct audience_s *au)
{
}

int16_t ui_audience_ask2a(struct audience_s *au)
{
    return -1;
}

int16_t ui_audience_ask2b(struct audience_s *au)
{
    return -1;
}

int16_t ui_audience_ask3(struct audience_s *au)
{
    return -1;
}

int16_t ui_audience_ask4(struct audience_s *au)
{
    return -1;
}

void ui_audience_newtech(struct audience_s *au)
{
}

void ui_audience_end(struct audience_s *au)
{
}

void ui_copyprotection_check(struct game_s *g) {
    copyprot_status = -99;
}

void ui_copyprotection_lose(struct game_s *g, struct game_end_s *ge) {
}

void ui_newships(struct game_s *g, int pi)
{
}
