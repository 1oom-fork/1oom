
#include "config.h"

#include <stdlib.h> /* abs */
#include <string.h>

#include "game.h"
#include "game_ai.h"
#include "game_ai_muxer.h"
#include "game_audience.h"
#include "game_battle.h"
#include "game_election.h"
#include "game_types.h"
#include "types.h"

/* Which player is controlled by what AI (if player is AI controlled) */
static game_ai_id_t game_ai_muxed[PLAYER_NUM] = {
    [PLAYER_0] = GAME_AI_CLASSICPLUS,
    [PLAYER_1] = GAME_AI_STUB,
    [PLAYER_2] = GAME_AI_CLASSICPLUS,
    [PLAYER_3] = GAME_AI_DEFAULT,
    [PLAYER_4] = GAME_AI_CLASSICPLUS,
    [PLAYER_5] = GAME_AI_STUB,
};

/*
The muxer functions all have the pattern
1. Set the game AI id to the one used for that player (see array above)
2. Call the function in that AI
3. Set ID back

This is necessary since the CLASSIC / CLASSICPLUS AIs do different
things depending on what the AI is set to. Maybe elsewhere too..

Some functions like AI-AI combat is handled by AI_DEFAULT (=CLASSICPLUS).
*/

/* -------------------------------------------------------------------------- */

static void game_ai_muxer_new_game_init(struct game_s *g, player_id_t player, uint8_t home)
{
    game_ai_id_t temp = g->ai_id;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->new_game_init(g, player, home);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

static void game_ai_muxer_new_game_tech(struct game_s *g, player_id_t player)
{
    game_ai_id_t temp = g->ai_id;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->new_game_tech(g, player);
    g->ai_id = temp;
}

/* ========================================================================== */

static void game_ai_muxer_turn_p1(struct game_s *g, player_id_t player)
{
    game_ai_id_t temp = g->ai_id;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->turn_p1(g, player);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

static void game_ai_muxer_turn_p2(struct game_s *g, player_id_t player)
{
    game_ai_id_t temp = g->ai_id;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->turn_p2(g, player);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

static void game_ai_muxer_turn_p3(struct game_s *g, player_id_t player)
{
    game_ai_id_t temp = g->ai_id;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->turn_p3(g, player);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

uint32_t game_ai_muxer_production_boost(const struct game_s *g, player_id_t player, uint32_t prod)
{
    return game_ais[game_ai_muxed[player]]->production_boost(g, player, prod);
}

/* -------------------------------------------------------------------------- */

uint8_t game_ai_muxer_tech_cost(const struct game_s *g, player_id_t player)
{
    return game_ais[game_ai_muxed[player]]->tech_cost(g, player);
}

/* -------------------------------------------------------------------------- */

uint16_t game_ai_muxer_base_cost_reduce(const struct game_s *g, player_id_t player, uint16_t cost)
{
   return game_ais[game_ai_muxed[player]]->base_cost_reduce(g, player, cost);
}

/* ========================================================================== */

/* Fake a battle between AI fleets? */

bool game_ai_muxer_battle_ai_ai_resolve(struct battle_s *bt)
{
    struct game_s *g = bt->g;
    game_ai_id_t temp = g->ai_id;
    bool ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->battle_ai_ai_resolve(bt);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

void game_ai_muxer_battle_ai_turn(struct battle_s *bt)
{
    struct game_s *g = bt->g;
    game_ai_id_t temp = g->ai_id;

    g->ai_id = GAME_AI_DEFAULT;
    game_ais[g->ai_id]->battle_ai_turn(bt);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

bool game_ai_muxer_battle_ai_retreat(struct battle_s *bt)
{
    struct game_s *g = bt->g;
    game_ai_id_t temp = g->ai_id;
    bool ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->battle_ai_retreat(bt);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

uint8_t game_ai_muxer_tech_next(struct game_s *g, player_id_t player, tech_field_t field, uint8_t *tbl, int num) {
    game_ai_id_t temp = g->ai_id;
    uint8_t ret;

    g->ai_id = game_ai_muxed[player];
    ret = game_ais[g->ai_id]->tech_next(g, player, field, tbl, num);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

bool game_ai_muxer_bomb(struct game_s *g, player_id_t player, uint8_t planet, int pop_inbound) {
    game_ai_id_t temp = g->ai_id;
    bool ret;

    g->ai_id = game_ai_muxed[player];
    ret = game_ais[g->ai_id]->bomb(g, player, planet, pop_inbound);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

void game_ai_muxer_ground(struct game_s *g, player_id_t def, player_id_t att, uint8_t planet, int pop_killed, bool owner_changed) {
    game_ai_id_t temp = g->ai_id;
    g->ai_id = GAME_AI_DEFAULT;
    game_ais[g->ai_id]->ground(g, def, att, planet, pop_killed, owner_changed);
    g->ai_id = temp;
}

/* ========================================================================== */

void game_ai_muxer_plague(struct game_s *g, uint8_t planet) {
    game_ai_id_t temp = g->ai_id;
    player_id_t player = g->planet[planet].owner;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->plague(g, planet);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

void game_ai_muxer_nova(struct game_s *g, uint8_t planet) {
    game_ai_id_t temp = g->ai_id;
    player_id_t player = g->planet[planet].owner;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->nova(g, planet);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

void game_ai_muxer_comet(struct game_s *g, uint8_t planet) {
    game_ai_id_t temp = g->ai_id;
    player_id_t player = g->planet[planet].owner;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->comet(g, planet);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

void game_ai_muxer_pirates(struct game_s *g, uint8_t planet) {
    game_ai_id_t temp = g->ai_id;
    player_id_t player = g->planet[planet].owner;
    g->ai_id = game_ai_muxed[player];
    game_ais[g->ai_id]->pirates(g, planet);
    g->ai_id = temp;
}

/* ========================================================================== */

int game_ai_muxer_vote(struct election_s *el, player_id_t player) {
    struct game_s *g = el->g;
    game_ai_id_t temp = g->ai_id;
    int ret;

    g->ai_id = game_ai_muxed[player];
    ret = game_ais[g->ai_id]->vote(el, player);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

void game_ai_muxer_turn_diplo_p1(struct game_s *g) {
    game_ai_id_t temp = g->ai_id;
    g->ai_id = GAME_AI_DEFAULT;
    game_ais[g->ai_id]->turn_diplo_p1(g);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

void game_ai_muxer_turn_diplo_p2(struct game_s *g) {
    game_ai_id_t temp = g->ai_id;
    g->ai_id = GAME_AI_DEFAULT;
    game_ais[g->ai_id]->turn_diplo_p2(g);
    g->ai_id = temp;
}

/* ========================================================================== */

void game_ai_muxer_aud_start_human(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    g->ai_id = GAME_AI_DEFAULT;
    game_ais[g->ai_id]->aud_start_human(au);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

int game_ai_muxer_aud_treaty_nap(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    int ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_treaty_nap(au);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

int game_ai_muxer_aud_treaty_alliance(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    int ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_treaty_alliance(au);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

int game_ai_muxer_aud_treaty_peace(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    int ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_treaty_peace(au);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

int game_ai_muxer_aud_treaty_declare_war(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    int ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_treaty_declare_war(au);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

int game_ai_muxer_aud_treaty_break_alliance(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    int ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_treaty_break_alliance(au);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

int game_ai_muxer_aud_trade(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    int ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_trade(au);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

bool game_ai_muxer_aud_sweeten(struct audience_s *au, int *bcptr, tech_field_t *fieldptr, uint8_t *techptr) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    bool ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_sweeten(au, bcptr, fieldptr, techptr);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

uint8_t game_ai_muxer_aud_threaten(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    uint8_t ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_threaten(au);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

void game_ai_muxer_aud_tribute_bc(struct audience_s *au, int selected, int bc) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;

    g->ai_id = GAME_AI_DEFAULT;
    game_ais[g->ai_id]->aud_tribute_bc(au, selected, bc);
    g->ai_id = temp;
}


/* -------------------------------------------------------------------------- */

void game_ai_muxer_aud_tribute_tech(struct audience_s *au, int selected, tech_field_t field, uint8_t tech) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;

    g->ai_id = GAME_AI_DEFAULT;
    game_ais[g->ai_id]->aud_tribute_tech(au, selected, field, tech);
    g->ai_id = temp;
}

/* -------------------------------------------------------------------------- */

int game_ai_muxer_aud_tech_scale(struct audience_s *au) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    int ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_tech_scale(au);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

uint8_t game_ai_muxer_aud_get_dtype(struct audience_s *au, uint8_t dtype, int a2) {
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    uint8_t ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_get_dtype(au, dtype, a2);
    g->ai_id = temp;

    return ret;
}

/* -------------------------------------------------------------------------- */

static bool game_ai_muxer_aud_later(struct audience_s *au)
{
    struct game_s *g = au->g;
    game_ai_id_t temp = g->ai_id;
    bool ret;

    g->ai_id = GAME_AI_DEFAULT;
    ret = game_ais[g->ai_id]->aud_later(au);
    g->ai_id = temp;

    return ret;
}

/* ========================================================================== */

const struct game_ai_s game_ai_muxer = {
    GAME_AI_MUXER,
    "MixerMuxer",
    "Provides a mix of different AIs",
    game_ai_muxer_new_game_init,
    game_ai_muxer_new_game_tech,
    game_ai_muxer_turn_p1,
    game_ai_muxer_turn_p2,
    game_ai_muxer_turn_p3,
    game_ai_muxer_production_boost,
    game_ai_muxer_tech_cost,
    game_ai_muxer_base_cost_reduce,
    game_ai_muxer_battle_ai_ai_resolve,
    game_ai_muxer_battle_ai_turn,
    game_ai_muxer_battle_ai_retreat,
    game_ai_muxer_tech_next,
    game_ai_muxer_bomb,
    game_ai_muxer_ground,
    game_ai_muxer_plague,
    game_ai_muxer_nova,
    game_ai_muxer_comet,
    game_ai_muxer_pirates,
    game_ai_muxer_vote,
    game_ai_muxer_turn_diplo_p1,
    game_ai_muxer_turn_diplo_p2,
    game_ai_muxer_aud_start_human,
    game_ai_muxer_aud_treaty_nap,
    game_ai_muxer_aud_treaty_alliance,
    game_ai_muxer_aud_treaty_peace,
    game_ai_muxer_aud_treaty_declare_war,
    game_ai_muxer_aud_treaty_break_alliance,
    game_ai_muxer_aud_trade,
    game_ai_muxer_aud_sweeten,
    game_ai_muxer_aud_threaten,
    game_ai_muxer_aud_tribute_bc,
    game_ai_muxer_aud_tribute_tech,
    game_ai_muxer_aud_tech_scale,
    game_ai_muxer_aud_get_dtype,
    game_ai_muxer_aud_later
};
