#include "config.h"

#include "game_stat.h"
#include "comp.h"
#include "game.h"
#include "game_num.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

int game_stat_fleet(const struct game_s *g, player_id_t pi)
{
    const empiretechorbit_t *e = &(g->eto[pi]);
    const shipresearch_t *srd = &(g->srd[pi]);
    int v2, v4, va, ve = 0, sum;
    sum = 0;
    for (int i = 0; i < e->shipdesigns_num; ++i) {
        sum += srd->shipcount[i] * game_num_tbl_hull_w[srd->design[i].hull];
    }
    SETMIN(sum, 0x319750);
    v4 = 0;
    va = 125;
    while (sum > va) {
        sum -= va;
        ++v4;
        ve = va;
        va <<= 1;
    }
    v2 = (sum * 10) / (va - ve) + v4 * 10;
    SETRANGE(v2, 0, 100);
    return v2;
}

int game_stat_tech(const struct game_s *g, player_id_t pi)
{
    const uint8_t *p = g->eto[pi].tech.percent;
    int sum = 0;
    for (tech_field_t f = TECH_FIELD_COMPUTER; f < TECH_FIELD_NUM; ++f) {
        sum += p[f];
    }
    return sum;
}

int game_stat_prod(const struct game_s *g, player_id_t pi)
{
    uint32_t va, ve = 0, prod = g->eto[pi].total_production_bc;
    int si = 0;
    va = 100;
    while (prod > va) {
        prod -= va;
        ++si;
        ve = va;
        va <<= 1;
    }
    return (prod * 10) / (va - ve) + si * 10;
}

int game_stat_pop(const struct game_s *g, player_id_t pi)
{
    int sum = 0;
    for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if (p->owner == pi) {
            sum += p->pop;
        }
    }
    return sum / ((g->galaxy_stars * 3) / 2 + 1);
}

int game_stat_planets(const struct game_s *g, player_id_t pi)
{
    int sum = 0;
    for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if (p->owner == pi) {
            ++sum;
        }
    }
    return (sum * 100) / (g->galaxy_stars + 1);
}
