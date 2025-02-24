#include "config.h"

#include "game_planet.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "rnd.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

void game_planet_destroy(struct game_s *g, uint8_t planet_i, player_id_t attacker)
{
    planet_t *p = &(g->planet[planet_i]);
    player_id_t owner = p->owner;
    if (IS_HUMAN(g, owner)) {
        g->seen[owner][planet_i].owner = PLAYER_NONE;
        g->seen[owner][planet_i].pop = 0;
        g->seen[owner][planet_i].bases = 0;
        g->seen[owner][planet_i].factories = p->factories;
    }
    if (IS_HUMAN(g, owner)) {
        /* WASBUG there was an unreliable mess here */
        g->gaux->human_killer = attacker;
    }
    p->rebels = 0;
    p->unrest = 0;
    p->reserve = 0;
    p->prev_owner = owner;
    p->owner = PLAYER_NONE;
    p->bc_to_ecoproj = 0;
    p->pop = 0;
    p->pop_prev = 0;
    p->prod_after_maint = 0;
    p->bc_to_ship = 0;
    for (int i = 0; i < PLAYER_NUM; ++i) {
        p->inbound[i] = 0;
    }
    p->buildship = 0;
    for (int i = 0; i < PLANET_SLIDER_NUM; ++i) {
        p->slider[i] = 0;
        p->slider_lock[i] = 0;
    }
    p->slider[PLANET_SLIDER_IND] = 100;
    p->reloc = planet_i;
    p->missile_bases = 0;
    p->bc_upgrade_base = 0;
    p->bc_to_base = 0;
    p->trans_num = 0;
    p->bc_to_ship = 0;
    p->bc_to_factory = 0;
    p->have_stargate = false;
    p->shield = 0;
    p->bc_to_shield = 0;
    for (int i = 0; i < g->galaxy_stars; ++i) {
        p = &(g->planet[i]);
        if (p->reloc == planet_i) {
            p->reloc = i;
        }
    }
}

uint8_t game_planet_get_random(struct game_s *g, player_id_t owner)
{
    uint8_t tbl[PLANETS_MAX];
    int num = 0;
    for (int i = 0; i < g->galaxy_stars; ++i) {
        if (g->planet[i].owner == owner) {
            tbl[num++] = i;
        }
    }
    if (num == 0) {
        return PLANET_NONE;
    } else {
        return tbl[rnd_0_nm1(num, &g->seed)];
    }
}

void game_planet_adjust_percent(struct game_s *g, player_id_t owner, int a0, uint8_t percent, int growth)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if (p->owner == owner) {
            if (0
              || (growth == 0)
              || ((growth == 1) && (p->growth > PLANET_GROWTH_HOSTILE))
              || ((growth == 2) && (p->growth == PLANET_GROWTH_HOSTILE))
            ) {
                int sum, v;
                sum = p->slider[PLANET_SLIDER_SHIP] + p->slider[PLANET_SLIDER_TECH];
                if (a0 != 2) {
                    sum += p->slider[PLANET_SLIDER_DEF];
                }
                if (a0 != 0) {
                    sum += p->slider[PLANET_SLIDER_IND];
                }
                v = (sum * percent) / 100;
                p->slider[PLANET_SLIDER_SHIP] = (p->slider[PLANET_SLIDER_SHIP] * (100 - percent)) / 100;
                if (a0 == 2) {
                    p->slider[PLANET_SLIDER_DEF] += v;
                } else {
                    p->slider[PLANET_SLIDER_DEF] = (p->slider[PLANET_SLIDER_DEF] * (100 - percent)) / 100;
                }
                if (a0 == 0) {
                    p->slider[PLANET_SLIDER_IND] += v;
                } else {
                    p->slider[PLANET_SLIDER_IND] = (p->slider[PLANET_SLIDER_IND] * (100 - percent)) / 100;
                }
                if (a0 == 3) {
                    p->slider[PLANET_SLIDER_ECO] += v;
                }
                p->slider[PLANET_SLIDER_TECH] = 100 - p->slider[PLANET_SLIDER_SHIP] - p->slider[PLANET_SLIDER_ECO] - p->slider[PLANET_SLIDER_DEF] - p->slider[PLANET_SLIDER_IND];
                SETMAX(p->slider[PLANET_SLIDER_TECH], 0);
            }
        }
    }
}

int game_planet_get_w1(const struct game_s *g, uint8_t planet_i)
{
    const planet_t *p = &(g->planet[planet_i]);
    const empiretechorbit_t *e;
    int w, fact, waste, prod;
    if (p->owner == PLAYER_NONE) {
        return 0;
    }
    e = &(g->eto[p->owner]);
    fact = p->factories;
    SETMIN(fact, p->pop * e->colonist_oper_factories);
    waste = (e->race == RACE_SILICOID) ? 0 : (((fact * e->ind_waste_scale) / 10 + p->waste) / e->have_eco_restoration_n);
    prod = p->prod_after_maint;
    if (prod == 0) {
        prod = 1000;
    }
    w = ((waste * 100) + prod - 1) / prod;
    SETRANGE(w, 0, 100);
    return w;
}

void game_planet_update_home(struct game_s *g)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        player_id_t pi;
        pi = p->owner;
        if ((pi != PLAYER_NONE) && IS_AI(g, pi) && (g->evn.home[pi] == PLANET_NONE)) { /* FIXME multiplayer */
            g->evn.home[pi] = 0;   /* FIXME why not "= i"? */
        }
    }
}
