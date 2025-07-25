#include "config.h"

#include <stdlib.h> /* abs */

#include "game_diplo.h"
#include "comp.h"
#include "game.h"
#include "game_num.h"
#include "game_planet.h"
#include "game_spy.h"
#include "game_tech.h"
#include "rnd.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const int16_t game_diplo_tbl_reldiff[6] = { -50, -30, -20, 0, 10, 20 };

/* -------------------------------------------------------------------------- */

static int game_diplo_wage_war_fleet_w(struct game_s *g, player_id_t p1, player_id_t p2)
{
    int ratio;
    uint32_t fleetw[2];
    for (int i = 0; i < 2; ++i) {
        player_id_t player;
        empiretechorbit_t *e;
        shipresearch_t *srd;
        player = (i == 0) ? p1 : p2;
        e = &(g->eto[player]);
        srd = &(g->srd[player]);
        fleetw[i] = 0;
        for (int j = 0; j < e->shipdesigns_num; ++j) {
            fleetw[i] += srd->shipcount[j] * game_num_tbl_hull_w[srd->design[j].hull];
        }
        SETRANGE(fleetw[i], 1, 3250000);
    }
    ratio = ((fleetw[0] - fleetw[1]) * 100) / fleetw[1];
    SETRANGE(ratio, -300, 300);
    if (ratio == 0) {
        return 0;
    } else if (ratio < 0) {
        return -rnd_1_n(-ratio, &g->seed);
    } else {
        return rnd_1_n(ratio, &g->seed);
    }
}

static void game_diplo_wage_war_do(struct game_s *g, player_id_t p1, player_id_t p2)
{
    empiretechorbit_t *e1 = &(g->eto[p1]);
    empiretechorbit_t *e2 = &(g->eto[p2]);
    if ((e1->treaty[p2] == TREATY_ALLIANCE) || (e1->treaty[p2] == TREATY_NONAGGRESSION)) {
        if (g->difficulty < DIFFICULTY_AVERAGE) {
            return;
        }
        if (0
          || ((e1->treaty[p2] == TREATY_ALLIANCE) && ((!rnd_0_nm1(4, &g->seed)) || ((!rnd_0_nm1(2, &g->seed)) && (e2->trait2 == TRAIT2_EXPANSIONIST))))
          || ((e1->treaty[p2] == TREATY_NONAGGRESSION) && ((!rnd_0_nm1(2, &g->seed)) || (e2->trait2 == TRAIT2_EXPANSIONIST)))
        ) {
            game_diplo_act(g, -10000, p1, p2, 32, 0, 0);
            game_diplo_break_treaty(g, p2, p1);
            if (e1->relation1[p2] > 30) {
                e1->relation1[p2] = 30;
                e2->relation1[p1] = 30;
            }
        }
    } else if (g->evn.ceasefire[p1][p2] <= 0) {
        game_diplo_act(g, -10000, p1, p2, (e1->relation1[p2] < 0) ? 13 : 60, 0, 0);
        game_diplo_start_war(g, p2, p1);
    }
}

static int game_diplo_wage_war_prod_w(struct game_s *g, player_id_t p1, player_id_t p2)
{
    int ratio;
    uint32_t prod[2];
    prod[0] = g->eto[p1].total_production_bc;
    prod[1] = g->eto[p2].total_production_bc;
    SETRANGE(prod[1], 1, 3250000);  /* FIXME BUG? only p2 prod limited */
    ratio = ((prod[0] - prod[1]) * 100) / prod[1];
    SETRANGE(ratio, -300, 300);
    if (ratio == 0) {
        return 0;
    } else if (ratio < 0) {
        return -rnd_1_n(-ratio, &g->seed);
    } else {
        return rnd_1_n(ratio, &g->seed);
    }
}

/* -------------------------------------------------------------------------- */

void game_diplo_act(struct game_s *g, int dv, player_id_t pi, player_id_t pi2, int dtype, uint8_t pli1, int16_t dp2)
{
    empiretechorbit_t *e = &(g->eto[pi]);
    empiretechorbit_t *e2 = &(g->eto[pi2]);
    int v1;
    if (g->end == GAME_END_FINAL_WAR) {
        if (IS_AI(g, pi) && (dv < 0) && IS_HUMAN(g, pi2) && (rnd_1_n(100, &g->seed) <= (dv / -10))) {
            player_id_t i = g->winner;
            e2->diplo_val[i] = 100;
            e2->diplo_type[i] = 59;
            e2->diplo_p2[i] = dp2;
            e2->diplo_p1[i] = pli1;
        }
        return;
    }
    if (0
      || (pi >= g->players) || (pi2 >= g->players)
      || (pi == pi2) || (dv == 0) || IS_HUMAN(g, pi2)
      || (e->treaty[pi2] == TREATY_FINAL_WAR)
    ) {
        return;
    }
    if (dv == -10000) {
        if (IS_HUMAN(g, pi) && (dtype != 0)) {
            e->diplo_val[pi2] = 10000;
            e->diplo_type[pi2] = dtype;
            e->diplo_p2[pi2] = dp2;
            e->diplo_p1[pi2] = pli1;
        }
        return;
    }
    if (dv > 0) {
        if (e2->relation1[pi] < 0) {
            dv *= 2;
            if ((e2->relation1[pi] + dv) > 10) {
                dv = 10 - e2->relation1[pi];
            }
        } else {
            int v4 = e2->relation1[pi] / 25 + 1;
            if (v4 == 0) {
                v4 = 1;
            }
            dv /= v4;
        }
    } else {
        if (e2->relation1[pi] > 0) {
            dv *= 2;
        } else {
            int v4 = e2->relation1[pi] / -25 + 1;
            if (v4 == 0) {
                v4 = 1;
            }
            dv /= v4;
        }
    }
    if ((e2->trait1 == TRAIT1_HONORABLE) && (dv < 0)) {
        dv *= 2;
    }
    if ((e2->trait1 == TRAIT1_XENOPHOBIC) && (dv < 0)) {
        dv = (dv * 3) / 2;
    }
    if ((e2->trait1 == TRAIT1_XENOPHOBIC) && (dv > 0)) {
        dv = (dv * 3) / 4;
    }
    v1 = dv;
    if ((e->race == RACE_HUMAN) && (dv > 0)) {
        dv *= 2;
    }
    if ((e2->treaty[pi] == TREATY_WAR) && (dtype >= 4) && (dtype <= 12) && (dv < 0)) {
        if (IS_HUMAN(g, pi)) {
            e2->mood_peace[pi] += dv / 4;
            e->mood_peace[pi2] -= dv / 4;
        }
        dv = 0;
    }
    if (IS_AI(g, pi) && IS_AI(g, pi2) && (g->year > 100)) {
        if (dv > 0) {
            dv *= 2;
        }
        if (dv < 0) {
            dv /= (g->difficulty / 2 + 1);
        }
    }
    if (dv != 0) {
        int v = e2->relation1[pi];
        v += dv;
        SETRANGE(v, -100, 100);
        e2->relation1[pi] = v;
        e2->mood_treaty[pi] += dv;
        e->mood_treaty[pi2] = e2->mood_treaty[pi];
        e2->mood_trade[pi] += dv;
        e->mood_trade[pi2] = e2->mood_trade[pi];
        e2->mood_tech[pi] += dv;
        e->mood_tech[pi2] = e2->mood_tech[pi];
        if (v1 < 0) {
            e->mood_peace[pi2] -= v1 / 4;
            e2->mood_peace[pi] += v1 / 4;
        } else {
            e2->mood_peace[pi] += v1;
        }
        if (IS_HUMAN(g, pi) && (dtype != 0) && (abs(dv) > abs(e->diplo_val[pi2]))) {
            e->diplo_val[pi2] = dv;
            e->diplo_type[pi2] = dtype;
            e->diplo_p2[pi2] = dp2;
            e->diplo_p1[pi2] = pli1;
        }
    }
    if (e2->treaty[pi] >= TREATY_WAR) {
        SETMAX(e2->relation1[pi], -25);
    }
    if (e2->treaty[pi] != TREATY_ALLIANCE) {
        SETMIN(e2->relation1[pi], 65);
    }
    e->relation1[pi2] = e2->relation1[pi];
}

void game_diplo_break_treaty(struct game_s *g, player_id_t pi, player_id_t pi2)
{
    empiretechorbit_t *e, *e2;
    int v2;
    if ((pi >= PLAYER_NUM) || (pi2 >= PLAYER_NUM)) {
        return;
    }
    e = &(g->eto[pi]);
    e2 = &(g->eto[pi2]);
    if (e2->treaty[pi] >= TREATY_WAR) {
        return;
    }
    v2 = 0;
    if (e->treaty[pi2] == TREATY_NONAGGRESSION) {
        v2 = -10;
    }
    if (e->treaty[pi2] == TREATY_ALLIANCE) {
        v2 = -20;
    }
    if (e->trait1 == TRAIT1_HONORABLE) {
        v2 *= 2;
    }
    e->trust[pi2] -= v2;
    if (e->treaty[pi2] == TREATY_ALLIANCE) {
        int v = e->relation2[pi2];
        v -= v2;
        SETMAX(v, -100);
        e->relation2[pi2] = v;
        e2->relation2[pi] = v;
    }
    if (v2 != 0) {
        int v;
        e->broken_treaty[pi2] = e->treaty[pi2];
        e2->broken_treaty[pi] = e->treaty[pi2];
        v = e->relation1[pi2] - rnd_1_n(20, &g->seed);
        SETMAX(v, -100);
        e->relation1[pi2] = v;
        e2->relation2[pi] = v;
    }
    e->treaty[pi2] = TREATY_NONE;
    e2->treaty[pi] = TREATY_NONE;
    e->hated[pi2] = PLAYER_NONE;
    e2->hated[pi] = PLAYER_NONE;
    e->mood_treaty[pi2] = -200;
    e->mood_trade[pi2] = -200;
    e->mood_tech[pi2] = -200;
    e->mood_peace[pi2] = -200;
    e2->mood_treaty[pi] = -200;
    e2->mood_trade[pi] = -200;
    e2->mood_tech[pi] = -200;
    e2->mood_peace[pi] = -200;
}

void game_diplo_start_war(struct game_s *g, player_id_t pi, player_id_t pi2)
{
    empiretechorbit_t *e, *e2;
    if ((pi >= PLAYER_NUM) || (pi2 >= PLAYER_NUM)) {
        return;
    }
    e = &(g->eto[pi]);
    e2 = &(g->eto[pi2]);
    if (IS_HUMAN(g, pi) && (e->have_met[pi2] == 0)) {
        e->have_met[pi2] = 1;
    }
    if (IS_HUMAN(g, pi2) && (e2->have_met[pi] == 0)) {
        e2->have_met[pi] = 1;
    }
    if (e->treaty[pi2] >= TREATY_WAR) {
        return;
    }
    /*6165b*/
    game_diplo_break_trade(g, pi, pi2);
    game_diplo_break_treaty(g, pi, pi2);
    e->relation2[pi2] -= 5;
    e2->relation2[pi] -= 5;
    e2->relation1[pi] = e->relation1[pi2] = -75 - rnd_1_n(25, &g->seed);
    e->treaty[pi2] = TREATY_WAR;
    e2->treaty[pi] = TREATY_WAR;
    e->mood_treaty[pi2] = -200;
    e->mood_trade[pi2] = -200;
    e->mood_tech[pi2] = -200;
    e->mood_peace[pi2] = -130;
    e2->mood_treaty[pi] = -200;
    e2->mood_trade[pi] = -200;
    e2->mood_tech[pi] = -200;
    e2->mood_peace[pi] = -130;
}

void game_diplo_break_trade(struct game_s *g, player_id_t pi, player_id_t pi2)
{
    empiretechorbit_t *e, *e2;
    if ((pi >= PLAYER_NUM) || (pi2 >= PLAYER_NUM)) {
        return;
    }
    e = &(g->eto[pi]);
    e2 = &(g->eto[pi2]);
    e->trade_bc[pi2] = 0;
    e2->trade_bc[pi] = 0;
    e->trade_established_bc[pi2] = 0;
    e2->trade_established_bc[pi] = 0;
    e->trade_percent[pi2] = 0;
    e2->trade_percent[pi] = 0;
    e->hated[pi2] = PLAYER_NONE;
    e2->hated[pi] = PLAYER_NONE;
    SETMAX(e->relation1[pi2], -100);
    e2->relation1[pi] = e->relation1[pi2];
    if (e->treaty[pi2] >= TREATY_WAR) {
        return;
    }
#if 0
    if (e->trade_bc[pi2] != 0) { /* never true */
        /* ignored */
    }
#endif
}

void game_diplo_annoy(struct game_s *g, player_id_t p1, player_id_t p2, int n)
{
    empiretechorbit_t *e1 = &(g->eto[p1]);
    n *= 10;
    e1->mood_trade[p2] -= n;
    e1->mood_treaty[p2] -= n;
    e1->mood_tech[p2] -= n;
    e1->mood_peace[p2] -= n;
}

void game_diplo_wage_war(struct game_s *g, player_id_t p1, player_id_t p2)
{
    if (g->end != GAME_END_NONE) {
        /* FIXME multiplayer ; gang up against rejecting player(s) */
        for (p1 = PLAYER_0; p1 < g->players; ++p1) {
            empiretechorbit_t *e1 = &(g->eto[p1]);
            if (IS_HUMAN(g, p1)) {
                continue;
            }
            for (p2 = PLAYER_0; p2 < g->players; ++p2) {
                if ((p1 == p2) || IS_HUMAN(g, p2)) {
                    continue;
                }
                e1->treaty[p2] = TREATY_ALLIANCE;
            }
        }
    } else {
        empiretechorbit_t *e1 = &(g->eto[p1]);
        empiretechorbit_t *e2 = &(g->eto[p2]);
        if ((e1->treaty[p2] >= TREATY_WAR) || BOOLVEC_IS0(e1->within_frange, p2) || (!IS_ALIVE(g, p1))) {
            return;
        }
        if (1
          && (e2->trait1 == TRAIT1_ERRATIC)
          && (rnd_1_n(300, &g->seed) <= g->difficulty)
          && (IS_AI(g, p1) || (g->evn.ceasefire[p1][p2] < 1))
        ) {
            e1->diplo_type[p2] = 61;
            e1->diplo_val[p2] = 2000;
            game_diplo_start_war(g, p2, p1);
        } else {
            if (1
              && (!rnd_0_nm1(20, &g->seed))
              && ((e1->trait2 == TRAIT2_MILITARIST) || (e1->trait2 == TRAIT2_EXPANSIONIST))
              && (e2->trait1 != TRAIT1_HONORABLE)
              && (IS_AI(g, p1) || (g->evn.ceasefire[p1][p2] < 1))
            ) {
                int v;
                v = game_diplo_wage_war_fleet_w(g, p1, p2);
                v = e1->relation1[p2] - v + game_diplo_tbl_reldiff[e2->trait1] + e1->trust[p2];
                if (v < -150) {
                    game_diplo_wage_war_do(g, p1, p2);
                }
            }
            /*1679f*/
            if (1
              && (!rnd_0_nm1(20, &g->seed))
              && (IS_AI(g, p1) || (g->evn.ceasefire[p1][p2] < 1))
            ) {
                int v;
                v = game_diplo_wage_war_prod_w(g, p1, p2);
                v = e1->relation1[p2] - v + game_diplo_tbl_reldiff[e2->trait1] + e1->trust[p2];
                if (v < -150) {
                    game_diplo_wage_war_do(g, p1, p2);
                }
            }
        }
        /*16829*/
        for (player_id_t p3 = PLAYER_0; p3 < g->players; ++p3) {
            if (IS_HUMAN(g, p3)) {
                continue;
            }
            for (player_id_t p4 = PLAYER_0; p4 < g->players; ++p4) {
                empiretechorbit_t *e4 = &(g->eto[p4]);
                if (IS_AI(g, p4)) {
                    continue;
                }
                if (1
                  && (e4->treaty[p3] == TREATY_WAR)
                  && (e2->treaty[p3] == TREATY_ALLIANCE)
                  && (g->evn.ceasefire[p4][p2] <= 0)
                  && (!rnd_0_nm1(10, &g->seed))
                  && (e2->treaty[p1] == TREATY_ALLIANCE)
                ) {
                    game_diplo_wage_war_do(g, p1, p2);
                }
            }
        }
        if (IS_HUMAN(g, p1) && (g->difficulty >= DIFFICULTY_AVERAGE)) {
            int num = 0;
            for (player_id_t p3 = PLAYER_0; p3 < g->players; ++p3) {
                if (e1->treaty[p3] >= TREATY_WAR) {
                    ++num;
                }
            }
            if (num < g->difficulty) {
                /* MOO1 does unused buggy count of planets ; overwrites local variable at tbl[-1] (which is also unused) */
                int v = e1->relation1[p2];
                if (v < -30) {
                    v = (-v) / 10;
                    if (rnd_1_n(10, &g->seed) <= v) {
                        game_diplo_wage_war_do(g, p1, p2);
                    }
                }
            }
        }
    }
}

void game_diplo_battle_finish(struct game_s *g, int def, int att, int popdiff, uint32_t app_def, uint16_t biodamage, uint32_t app_att, uint8_t planet_i)
{
    /* FIXME multiplayer */
    player_id_t claim, side_z, side_ai;
    uint32_t app_ai;
    planet_t *p;
    int offense, extra;
    if ((att >= PLAYER_NUM) || (def >= PLAYER_NUM)) {
        return;
    }
    p = &(g->planet[planet_i]);
    claim = p->claim;
    if ((claim == PLAYER_NONE) || ((claim != att) && (claim != def))) {
        if (IS_AI(g, att)) {
            side_z = def;
            side_ai = att;
            app_ai = app_att;
        } else {
            side_z = att;
            side_ai = def;
            app_ai = app_def;
        }
    } else {
        if (claim == att) {
            side_z = def;
            side_ai = att;
            app_ai = app_att;
        } else {
            side_z = att;
            side_ai = def;
            app_ai = app_def;
        }
    }
    if (biodamage) {
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            if ((i != side_z) && IS_AI(g, i)) {
                game_diplo_act(g, -10, side_z, i, 11, planet_i, 0);
            }
        }
    }
    popdiff *= 5;
    SETMIN(app_ai, 50);
    biodamage *= 10;
    offense = MAX(popdiff, app_ai);
    SETMAX(offense, biodamage);
    if (biodamage > 0) {
        extra = 11;
    } else if (popdiff > 0) {
        extra = 10;
    } else {
        extra = 9;
    }
    game_diplo_act(g, -offense, side_z, side_ai, extra, planet_i, 0);
    if ((offense >= 30) && IS_HUMAN(g, side_z)) {
        empiretechorbit_t *ea = &(g->eto[side_ai]);
        empiretechorbit_t *ez = &(g->eto[side_z]);
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            if ((i != side_z) && (i != side_ai) && ((ea->treaty[i] >= TREATY_WAR) || (ez->mutual_enemy[i] == side_ai))) {
                if (ez->hated[i] == side_ai) {
                    ez->mutual_enemy[i] = i;
                } else if (game_planet_get_random(g, side_ai) == PLANET_NONE) {
                    game_diplo_act(g, offense, side_z, i, 3, planet_i, 0);
                }
            }
        }
    }
}

void game_diplo_set_treaty(struct game_s *g, player_id_t p1, player_id_t p2, treaty_t treaty)
{
    empiretechorbit_t *e1 = &(g->eto[p1]);
    empiretechorbit_t *e2 = &(g->eto[p2]);
    e1->treaty[p2] = treaty;
    e2->treaty[p1] = treaty;
}

void game_diplo_set_trade(struct game_s *g, player_id_t p1, player_id_t p2, int bc)
{
    empiretechorbit_t *e1 = &(g->eto[p1]);
    empiretechorbit_t *e2 = &(g->eto[p2]);
    int v;
    if (bc == 0) {
        return;
    }
    if (e1->trade_established_bc[p2] < bc) {
        v = ((e1->trade_percent[p2] * e1->trade_established_bc[p2]) - (bc * 30)) / (e1->trade_established_bc[p2] + bc);
    } else {
        v = e1->trade_percent[p2];
    }
    SETMIN(v, 100);
    e1->trade_bc[p2] = bc;
    e1->trade_established_bc[p2] = bc;  /* FIXME BUG? should be < bc to make the variable have a purpose */
    e1->trade_percent[p2] = v;
    e2->trade_bc[p1] = bc;
    e2->trade_established_bc[p1] = bc;
    e2->trade_percent[p1] = v;
}

void game_diplo_stop_war(struct game_s *g, player_id_t p1, player_id_t p2)
{
    empiretechorbit_t *e1 = &(g->eto[p1]);
    empiretechorbit_t *e2 = &(g->eto[p2]);
    game_diplo_set_treaty(g, p1, p2, TREATY_NONE);
    ADDSATT(e1->relation1[p2], 40, 100);
    e2->relation1[p1] = e1->relation1[p2];
    if (IS_HUMAN(g, p1) /* && IS_AI(g, p2) */) {
        g->evn.ceasefire[p1][p2] = rnd_0_nm1(6, &g->seed) + 8;
    }
    if (IS_HUMAN(g, p2) /* && IS_AI(g, p1) */) {
        g->evn.ceasefire[p2][p1] = rnd_0_nm1(6, &g->seed) + 8;
    }
}

void game_diplo_limit_mood_treaty(struct game_s *g)
{
    for (player_id_t p1 = PLAYER_0; p1 < g->players; ++p1) {
        empiretechorbit_t *e = &(g->eto[p1]);
        for (player_id_t p2 = PLAYER_0; p2 < g->players; ++p2) {
            if (p1 != p2) {
                SETRANGE(e->mood_treaty[p2], -200, 120);
            }
        }
    }
}

void game_diplo_mood_relax(struct game_s *g)
{
    for (player_id_t p1 = PLAYER_0; p1 < g->players; ++p1) {
        empiretechorbit_t *e = &(g->eto[p1]);
        for (player_id_t p2 = PLAYER_0; p2 < g->players; ++p2) {
            if (p1 != p2) {
                if (e->mood_treaty[p2] < 100) {
                    e->mood_treaty[p2] += 10;
                }
                if (e->mood_trade[p2] < 100) {
                    e->mood_trade[p2] += 10;
                }
                if (e->mood_tech[p2] < 100) {
                    e->mood_tech[p2] += 10;
                }
                if (e->mood_peace[p2] < 100) {
                    e->mood_peace[p2] += 10;
                }
            }
        }
    }
}

int16_t game_diplo_get_mood(struct game_s *g, player_id_t p1, player_id_t p2)
{
    if (g->end == GAME_END_FINAL_WAR) {
        return 0;
    } else {
        empiretechorbit_t *e = &(g->eto[p1]);
        int16_t vmin, v;
        vmin = e->mood_treaty[p2];
        v = e->mood_trade[p2];
        SETMIN(vmin, v);
        v = e->mood_tech[p2];
        SETMIN(vmin, v);
        v = e->mood_peace[p2];
        SETMIN(vmin, v);
        return vmin;
    }
}
