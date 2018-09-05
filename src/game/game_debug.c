#include "config.h"

#ifdef FEATURE_MODEBUG

#include "game_debug.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_str.h"
#include "log.h"
#include "options.h"

/* -------------------------------------------------------------------------- */

void game_debug_dump_sliders(const struct game_s *g, bool force)
{
    if (!force && (opt_modebug < 4)) {
        return;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if (p->owner != PLAYER_NONE) {
            int sum;
            sum = 0;
            LOG_DEBUG((0, "%s: pl:%-3i'%-12s' o:%i sl:", __func__, i, p->name, p->owner));
            for (int j = 0; j < PLANET_SLIDER_NUM; ++j) {
                LOG_DEBUG((0, " %-3i", p->slider[j]));
                sum += p->slider[j];
            }
            LOG_DEBUG((0, " sum:%i\n", sum));
        }
    }
}

void game_debug_dump_race_techs(struct game_s *g, bool force)
{
    if (!force) {
        if (opt_modebug < 4) {
            return;
        }
    } else {
        game_update_total_research(g);
        game_update_production(g);
    }
    for (int pi = 0; pi < g->players; ++pi) {
        const empiretechorbit_t *e = &(g->eto[pi]);
        const techdata_t *t = &(e->tech);
        if (!IS_ALIVE(g, pi)) {
            continue;
        }
        {
            uint32_t res, prod, r;
            res = e->total_research_bc;
            prod = e->total_production_bc;
            SETMAX(prod, 1);
            r = (res * 1000) / prod;
            LOG_DEBUG((0, "%s: p:%i total_research:%u (%u.%u%%)\n", __func__, pi, res, r / 10, r % 10));
        }
        for (tech_field_t f = 0; f < TECH_FIELD_NUM; ++f) {
            LOG_DEBUG((0, "  f:%i %%:%-2u p:%-2u d:%-2u s:%-3i inv:%u c:%u\n", f, t->percent[f], t->project[f], t->completed[f], t->slider[f], t->investment[f], t->cost[f]));
        }
    }
}

void game_debug_dump_race_spending(struct game_s *g, bool force)
{
    if (!force) {
        if (opt_modebug < 4) {
            return;
        }
    } else {
        game_update_total_research(g);
        game_update_production(g);
    }
    for (int pi = 0; pi < g->players; ++pi) {
        const empiretechorbit_t *e = &(g->eto[pi]);
        uint32_t prod, v, r;
        if (!IS_ALIVE(g, pi)) {
            continue;
        }
        prod = e->total_production_bc;
        LOG_DEBUG((0, "%s: p:%i %s", __func__, pi, game_str_tbl_race[e->race]));
        LOG_DEBUG((0, " prod:%u", prod));
        SETMAX(prod, 1);
        v = e->total_research_bc;
        r = (v * 1000) / prod;
        LOG_DEBUG((0, " res:%u (%u.%u%%)", v, r / 10, r % 10));
        v = 0;
        for (player_id_t i = PLAYER_0; i < PLAYER_NUM; ++i) {
            if (i != pi) {
                v += e->spying[i];
            }
        }
        r = (v * 1000) / prod;
        LOG_DEBUG((0, " spy:%u (%u.%u%%)", v, r / 10, r % 10));
        r = e->security;
        LOG_DEBUG((0, " sec:%u.%u%%", r / 10, r % 10));
        v = e->ship_maint_bc;
        r = (v * 1000) / prod;
        LOG_DEBUG((0, " ship:%u (%u.%u%%)", v, r / 10, r % 10));
        v = e->bases_maint_bc;
        r = (v * 1000) / prod;
        LOG_DEBUG((0, " base:%u (%u.%u%%)", v, r / 10, r % 10));
        r = e->tax;
        LOG_DEBUG((0, " tax:%u.%u%%", r / 10, r % 10));
        LOG_DEBUG((0, " reserve:%u\n", e->reserve_bc));
    }
}

void game_debug_dump_race_waste(struct game_s *g, bool force)
{
    if (!force) {
        if (opt_modebug < 4) {
            return;
        }
    }
    for (int pi = 0; pi < g->players; ++pi) {
        int num_planets, no_waste, no_fact, sum_waste;
        if (!IS_ALIVE(g, pi)) {
            continue;
        }
        num_planets = 0;
        no_waste = 0;
        no_fact = 0;
        sum_waste = 0;
        for (int i = 0; i < g->galaxy_stars; ++i) {
            const planet_t *p = &(g->planet[i]);
            if (p->owner == pi) {
                ++num_planets;
                if (p->waste == 0) {
                    ++no_waste;
                    if (p->factories == 0) {
                        ++no_fact;
                    }
                } else {
                    sum_waste += (((int)p->waste) * 1000) / p->max_pop2;
                }
            }
        }
        if (num_planets > no_fact) {
            sum_waste /= num_planets - no_fact;
        } else {
            sum_waste /= num_planets;
        }
        LOG_DEBUG((0, "%s: p:%i no waste:%i/%i, no fact:%i, avg:%u.%u%%\n", __func__, pi, no_waste, num_planets, no_fact, sum_waste / 10, sum_waste % 10));
    }
}

#endif /* FEATURE_MODEBUG */
