#include "config.h"

#include <stdio.h>
#include <string.h>

#include "game_ground.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_diplo.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_shiptech.h"
#include "game_spy.h"
#include "game_str.h"
#include "game_tech.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "ui.h"
#include "util.h"
#include "util_math.h"

/* -------------------------------------------------------------------------- */

static void game_ground_init(struct ground_s *gr)
{
    struct game_s *g = gr->g;
    char buf[0x32];
    for (int i = 0; i < 2; ++i) {
        const empiretechorbit_t *e = &(g->eto[gr->s[i].player]);
        const shipresearch_t *srd = &(g->srd[gr->s[i].player]);
        const uint8_t *rc;
        uint8_t bestarmor, bestsuit, bestshield, bestweap, besti;
        gr->s[i].human = IS_HUMAN(g, gr->s[i].player);
        gr->s[i].force = 0;
        gr->s[i].strnum = 1;
        bestarmor = 0;
        bestsuit = 0;
        rc = &(srd->researchcompleted[TECH_FIELD_CONSTRUCTION][0]);
        for (int j = 0; j < e->tech.completed[TECH_FIELD_CONSTRUCTION]; ++j) {
            const uint8_t *r;
            r = RESEARCH_D0_PTR(g->gaux, TECH_FIELD_CONSTRUCTION, rc[j]);
            if (r[0] == 7) {
                bestarmor = r[1];
            } else if (r[0] == 0xf) {
                bestsuit = r[1];
                besti = rc[j];
            }
        }
        gr->s[i].force += bestarmor * 5 + bestsuit * 10;
        strcpy(buf, *tbl_shiptech_armor[bestarmor * 2].nameptr);
        util_str_tolower(&buf[1]);
        sprintf(gr->s[i].str[0], "%s ", buf);
        if (bestsuit == 0) {
            strcat(gr->s[i].str[0], game_str_gr_carmor);
        } else {
            game_tech_get_name(g->gaux, TECH_FIELD_CONSTRUCTION, besti, buf);
            strcat(gr->s[i].str[0], buf);
        }
        bestshield = 0;
        rc = &(srd->researchcompleted[TECH_FIELD_FORCE_FIELD][0]);
        for (int j = 0; j < e->tech.completed[TECH_FIELD_FORCE_FIELD]; ++j) {
            const uint8_t *r;
            r = RESEARCH_D0_PTR(g->gaux, TECH_FIELD_FORCE_FIELD, rc[j]);
            if (r[0] == 0x10) {
                bestshield = r[1];
                besti = rc[j];
            }
        }
        if (bestshield != 0) {
            gr->s[i].force += bestshield * 10;
            game_tech_get_name(g->gaux, TECH_FIELD_FORCE_FIELD, besti, gr->s[i].str[1]);
            gr->s[i].strnum = 2;
        }
        bestweap = 0;
        rc = &(srd->researchcompleted[TECH_FIELD_WEAPON][0]);
        for (int j = 0; j < e->tech.completed[TECH_FIELD_WEAPON]; ++j) {
            const uint8_t *r;
            r = RESEARCH_D0_PTR(g->gaux, TECH_FIELD_WEAPON, rc[j]);
            if (r[0] == 0x15) {
                bestweap = r[1];
                besti = rc[j];
            }
        }
        if (bestweap != 0) {
            if (bestweap > 2) {
                ++bestweap;
            }
            gr->s[i].force += bestweap * 5;
            game_tech_get_name(g->gaux, TECH_FIELD_WEAPON, besti, gr->s[i].str[gr->s[i].strnum++]);
        }
        if (e->race == RACE_BULRATHI) {
            gr->s[i].force += 25;
        }
    }
    gr->s[gr->flag_swap ? 0 : 1].force += 5;
}

/* -------------------------------------------------------------------------- */

void game_ground_kill(struct ground_s *gr)
{
    int v1, v2, death;
    v1 = rnd_1_n(100, &gr->g->seed) + gr->s[0].force;
    v2 = rnd_1_n(100, &gr->g->seed) + gr->s[1].force;
    if (v1 <= v2) {
        death = 0;
    } else {
        death = 1;
    }
    gr->death = death;
    --gr->s[death].pop1;
}

void game_ground_finish(struct ground_s *gr)
{
    planet_t *p = &(gr->g->planet[gr->planet_i]);
    struct game_s *g = gr->g;
    struct spy_esp_s *s = gr->steal;
    gr->techchance = 0;
    if (gr->s[0].pop1 > 0) {
        if (gr->flag_rebel) {
            p->unrest = PLANET_UNREST_RESOLVED;
            p->pop = p->pop - p->rebels + gr->s[0].pop1;
            p->rebels = 0;
        } else {
            int fact, chance, num;
            gr->fact = fact = p->factories;
            chance = 0;
            for (int i = 0; i < fact; ++i) {
                if (!rnd_0_nm1(50, &g->seed)) {
                    ++chance;
                }
            }
            gr->techchance = chance;
            s->target = gr->s[1].player;
            s->spy = gr->s[0].player;
            num = game_spy_esp_sub1(g, s, 0, 0);
            SETMIN(num, chance);
            s->tnum = num;
            for (int i = 0; i < num; ++i) {
                game_tech_get_new(g, gr->s[0].player, s->tbl_field[i], s->tbl_tech2[i], TECHSOURCE_FOUND, gr->planet_i, gr->s[1].player, false);
            }
            gr->techchance = num;
            game_planet_destroy(g, gr->planet_i, gr->s[0].player);
            p->owner = gr->s[0].player;
            p->pop = gr->s[0].pop1;
            p->bc_to_refit = 0;
            p->pop_oper_fact = 1;
        }
    } else {
        if (gr->flag_rebel) {
            p->rebels = gr->s[1].pop1;
            if (p->rebels == 0) {
                p->unrest = PLANET_UNREST_RESOLVED;
            }
        } else {
            p->pop = gr->s[1].pop1;
        }
    }
    SETMIN(p->pop, p->max_pop3);
}

void game_turn_ground(struct game_s *g)
{
    struct ground_s gr[1];
    struct spy_esp_s steal;
    gr->g = g;
    gr->steal = &steal;
    for (int pli = 0; pli < g->galaxy_stars; ++pli) {
        planet_t *p = &(g->planet[pli]);
        player_id_t powner;
        uint16_t inbound[PLAYER_NUM];
        powner = p->owner;
        for (player_id_t i = 0; i < g->players; ++i) {
            inbound[i] = p->inbound[i];
        }
        for (player_id_t i = 0; i < g->players; ++i) {
            if ((i != powner) || (p->unrest == PLANET_UNREST_REBELLION)) {
                powner = p->owner;  /* FIXME redundant */
                p->inbound[i] = inbound[i]; /* FIXME why ? */
                if ((powner != PLAYER_NONE) && (p->inbound[i] > 0)) {
                    int pop_planet;
                    gr->flag_rebel = (p->unrest == PLANET_UNREST_REBELLION) && IS_HUMAN(g, i) && (p->owner == i);
                    gr->inbound = p->inbound[i];
                    gr->total_inbound = p->total_inbound[i];
                    gr->s[0].pop2 = gr->s[0].pop1 = p->inbound[i];
                    pop_planet = gr->s[1].pop2 = gr->s[1].pop1 = gr->flag_rebel ? p->rebels : p->pop;
                    gr->s[0].player = i;
                    gr->s[1].player = powner;
                    gr->planet_i = pli;
                    gr->flag_swap = false;
                    if (IS_HUMAN(g, i) || IS_HUMAN(g, powner)) {
                        int t;
                        gr->flag_swap = true;
                        t = gr->s[0].pop2; gr->s[0].pop2 = gr->s[1].pop2; gr->s[1].pop2 = t;
                        t = gr->s[0].pop1; gr->s[0].pop1 = gr->s[1].pop1; gr->s[1].pop1 = t;
                        t = gr->s[0].player; gr->s[0].player = gr->s[1].player; gr->s[1].player = t;
                    }
                    game_ground_init(gr);
                    if ((gr->s[0].pop1 != 0) && (gr->s[1].pop1 != 0)) {
                        if (gr->s[0].human || gr->s[1].human) {
                            /*e8a1*/
                            ui_ground(gr);
                            if (gr->flag_swap) {
                                int t;
                                t = gr->s[0].pop1; gr->s[0].pop1 = gr->s[1].pop1; gr->s[1].pop1 = t;
                                t = gr->s[0].player; gr->s[0].player = gr->s[1].player; gr->s[1].player = t;
                            }
                            /*e8cb*/
                            pop_planet -= gr->s[1].pop1;
                            SETMAX(pop_planet, 1);
                            if ((p->owner != powner) && IS_HUMAN(g, gr->s[0].player)) {
                                if (g->eto[gr->s[0].player].treaty[gr->s[1].player] < TREATY_WAR) {
                                    game_diplo_act(g, -50 - rnd_1_n(50, &g->seed), gr->s[0].player, gr->s[1].player, 0xd, pli, 0);
                                    game_diplo_start_war(g, gr->s[1].player, gr->s[0].player);
                                } else {
                                    /*e93f*/
                                    game_diplo_act(g, -50 - rnd_1_n(50, &g->seed), gr->s[0].player, gr->s[1].player, 0xa, pli, 0);
                                }
                            } else {
                                /*e969*/
                                game_diplo_act(g, -((rnd_1_n(5, &g->seed) + 5) * pop_planet), gr->s[0].player, gr->s[1].player, 0xa, pli, 0);
                            }
                        } else {
                            /*e996*/
                            pop_planet -= gr->s[1].pop1;
                            SETMAX(pop_planet, 1);
                            while ((gr->s[0].pop1 != 0) && (gr->s[1].pop1 != 0)) {
                                game_ground_kill(gr);
                            }
                            game_ground_finish(gr);
                            /*e9c4*/
                            game_diplo_act(g, -((rnd_1_n(4, &g->seed) + 4) * pop_planet), gr->s[0].player, gr->s[1].player, 0xa, pli, 0);
#if 0
                            if (gr->s[0].human) {   /* FIXME no test on MOO1, never true */
                                ui_newtech(g, gr->s[0].player); /* FIXME why is this here? only AI present */
                            }
                            g->evn.newtech[gr->s[0].player].num = 0;
#endif
                        }
                    }
                }
                powner = p->owner;
            }
        }
    }
}
