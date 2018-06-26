#include "config.h"

#ifdef FEATURE_MODEBUG

#include "game_debug.h"
#include "comp.h"
#include "game.h"
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

#endif /* FEATURE_MODEBUG */
