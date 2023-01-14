#include "config.h"

#include <stdio.h>

#include "game_misc.h"
#include "bits.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_ai.h"
#include "game_aux.h"
#include "game_num.h"
#include "game_shiptech.h"
#include "game_str.h"
#include "game_tech.h"
#include "lib.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "util.h"
#include "util_math.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

void game_update_have_reserve_fuel(struct game_s *g)
{
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        shipresearch_t *srd = &(g->srd[pi]);
        int num = g->eto[pi].shipdesigns_num;
        for (int si = 0; si < NUM_SHIPDESIGNS; ++si) {
            srd->have_reserve_fuel[si] = false;
        }
        for (int si = 0; si < num; ++si) {
            shipdesign_t *sd = &(srd->design[si]);
            if (0
              || (sd->special[0] == SHIP_SPECIAL_RESERVE_FUEL_TANKS)
              || (sd->special[1] == SHIP_SPECIAL_RESERVE_FUEL_TANKS)
              || (sd->special[2] == SHIP_SPECIAL_RESERVE_FUEL_TANKS)
            ) {
                srd->have_reserve_fuel[si] = true;
            }
        }
    }
}

void game_update_maint_costs(struct game_s *g)
{
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        shipsum_t tbl_ships[NUM_SHIPDESIGNS];
        uint32_t totalcost; /* FIXME maybe uint64_t? */
        uint16_t bases;
        shipresearch_t *srd = &(g->srd[pi]);
        empiretechorbit_t *e = &(g->eto[pi]);
        int numsd = e->shipdesigns_num;
        for (int si = 0; si < NUM_SHIPDESIGNS; ++si) {
            tbl_ships[si] = 0;
        }
        for (int si = 0; si < numsd; ++si) {
            for (int i = 0; i < g->galaxy_stars; ++i) {
                tbl_ships[si] += e->orbit[i].ships[si];
            }
        }
        for (int j = 0; j < g->enroute_num; ++j) {
            if (g->enroute[j].owner == pi) {
                for (int si = 0; si < numsd; ++si) {
                    tbl_ships[si] += g->enroute[j].ships[si];
                }
            }
        }
        totalcost = 0;
        for (int si = 0; si < NUM_SHIPDESIGNS; ++si) {
            const shipdesign_t *sd = &(srd->design[si]);
            srd->shipcount[si] = tbl_ships[si];
            totalcost += tbl_ships[si] * sd->cost;
        }
        totalcost = totalcost / 50;
        SETMIN(totalcost, game_num_max_ship_maint);
        bases = 0;
        for (int i = 0; i < g->galaxy_stars; ++i) {
            const planet_t *p = &(g->planet[i]);
            if (p->owner == pi) {
                if (p->have_stargate) {
                    totalcost += game_num_stargate_maint; /* WASBUG MOO1 sums to a int16_t var after limiting to 32000 */
                }
                bases += p->missile_bases;
            }
        }
        SETMIN(totalcost, game_num_max_ship_maint);
        e->ship_maint_bc = totalcost;
        e->bases_maint_bc = (bases * game_get_base_cost(g, pi)) / 50;
    }
}

void game_update_production(struct game_s *g)
{
    game_update_maint_costs(g);
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        g->eto[pi].total_production_bc = 0;
    }
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        empiretechorbit_t *e = &(g->eto[pi]);
        for (player_id_t pi2 = PLAYER_0; pi2 < g->players; ++pi2) {
            if ((pi == pi2) || BOOLVEC_IS0(e->contact, pi2)) {
                e->spying[pi2] = 0;
            }
        }
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        player_id_t owner = p->owner;
        if (owner != PLAYER_NONE) {
            empiretechorbit_t *e = &(g->eto[owner]);
            int popx = p->pop;
            int v;
	    /* WASBUG changed to account for leaving transports */
            {
		int r = p->rebels;
		int t = p->trans_num;
		if (t) {           /* see game_send_transport() */
		    popx -= t;
		    SUBSAT0(r, t / 2 + 1);
                    SETMAX(popx, 1);
		}
        if (g->game_mode_extra & GAME_MODE_EXTRA_FACTORY_COST_FIX) {
            v = (popx - r) * p->pop_oper_fact;
        } else {
            v = (popx - r) * e->colonist_oper_factories;
        }
		SETMAX(v, 0);
            }
            {
                uint16_t factories = p->factories;
                int extra;
                SETMIN(factories, v);
                extra = (popx * (e->tech.percent[TECH_FIELD_PLANETOLOGY] * 3 + 50)) / 100;
                if (e->race == RACE_KLACKON) {
                    extra <<= 1;
                }
                v = factories + extra;
            }
            /* AI given a chance to cheat with production */
            if (IS_AI(g, owner)) v = game_ai->production_boost(g, owner, v);
            {
                uint32_t reserve;
                reserve = p->reserve;
                SETMIN(reserve, v);
                v += reserve;
            }
            if (p->unrest == PLANET_UNREST_REBELLION) {
                v = 0;
            }
            SETMAX(v, 0);
            p->prod_after_maint = v;
            p->total_prod = v;
            e->total_production_bc += v;
        }
    }
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        empiretechorbit_t *e = &(g->eto[pi]);
        uint16_t spysum;
        spysum = 0;
        for (player_id_t pi2 = PLAYER_0; pi2 < g->players; ++pi2) {
            spysum += e->spying[pi2];
        }
        {
            uint16_t spymaint;
            spymaint = 0;
            for (player_id_t pi2 = PLAYER_0; pi2 < g->players; ++pi2) {
                spymaint += (e->total_production_bc * e->spying[pi2]) / 1000;
            }
            e->spying_maint_bc = spymaint;
        }
        {
            uint8_t bonus = (e->race == RACE_HUMAN) ? 25 : 0;
            int32_t trade = 0;
            for (player_id_t pi2 = PLAYER_0; pi2 < g->players; ++pi2) {
                trade += (e->trade_bc[pi2] * (e->trade_percent[pi2] + bonus)) / 100;
            }
            e->total_trade_bc = trade;
        }
        {
            int32_t actual_prod;
            actual_prod = (e->total_production_bc * (1000 - e->security - spysum)) / 1000
                        + e->total_trade_bc - e->ship_maint_bc - e->bases_maint_bc
                        - (e->total_production_bc * e->tax) / 1000;
            if (actual_prod < 1) {
                actual_prod = 1;
            }
            e->total_maint_bc = e->total_production_bc - actual_prod;
            e->percent_prod_total_to_actual = e->total_production_bc ? ((actual_prod * 100) / e->total_production_bc) : 0;
        }
        for (int i = 0; i < g->galaxy_stars; ++i) {
            planet_t *p = &(g->planet[i]);
            if (p->owner == pi) {
                int v;
                v = (p->prod_after_maint * e->percent_prod_total_to_actual) / 100;
		/* WASBUG deducting transport cost takes no effect for actual production,
		 * which happens after departure, but leads to wrong display for player.
		 * v -= p->trans_num; */
                p->prod_after_maint = (v > 0) ? v : 0;
            }
        }
    }
}

void game_update_total_research(struct game_s *g)
{
    uint32_t mult;
    mult = game_tech_get_research_mult(g);

    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        g->eto[pi].total_research_bc = 0;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if (1
          && (p->owner != PLAYER_NONE)
          && (!(g->evn.have_plague && (g->evn.plague_planet_i == i)))
          && (!(g->evn.have_nova && (g->evn.nova_planet_i == i)))
        ) {
            empiretechorbit_t *e;
            e = &(g->eto[p->owner]);
            e->total_research_bc += game_get_tech_prod(p->prod_after_maint, p->slider[PLANET_SLIDER_TECH], e->race, p->special) * mult / 4;
        }
    }
}

void game_planet_update_eco_on_waste(struct game_s *g, struct planet_s *p, int player_i, bool force_adjust)
{
    empiretechorbit_t *e = &(g->eto[player_i]);
    if (e->race == RACE_SILICOID) {
        return;
    }
    uint16_t v, fact, waste, prod;
    int16_t left;
    fact = game_planet_get_operating_factories(g, p);
    waste = (fact * e->ind_waste_scale) / 10;
    waste = (waste + p->waste) / e->have_eco_restoration_n;
    prod = p->prod_after_maint;
    if (!prod) {
        prod = 1000;
    }
    v = (waste * 100 + prod - 1) / prod;
    SETRANGE(v, 0, 100);
    if ((p->slider[PLANET_SLIDER_ECO] < v) || force_adjust) {
        if (game_num_waste_adjust_fix) {
            int sum;
            sum = p->slider[PLANET_SLIDER_SHIP] + p->slider[PLANET_SLIDER_DEF] + p->slider[PLANET_SLIDER_IND] + p->slider[PLANET_SLIDER_TECH];
            left = sum + p->slider[PLANET_SLIDER_ECO] - v;
            SETMAX(left, 0);
            if (sum > 0) {
                int16_t left2, maxt;
                planet_slider_i_t maxi = 0;
                maxt = -1;
                left2 = left;
                for (planet_slider_i_t si = PLANET_SLIDER_SHIP; si <= PLANET_SLIDER_TECH; ++si) {
                    int16_t t;
                    if (si == PLANET_SLIDER_ECO) {
                        continue;
                    }
                    p->slider[si] = t = (p->slider[si] * left) / sum;
                    left2 -= t;
                    if (maxt < t) {
                        maxt = t;
                        maxi = si;
                    }
                }
                if (left2 > 0) {
                    p->slider[maxi] += left2;
                }
            }
        } else {
            /* WASBUG this can result in the sum of sliders exceeding 100 */
            left = 100 - (v - p->slider[PLANET_SLIDER_ECO]);
            p->slider[PLANET_SLIDER_SHIP] = (p->slider[PLANET_SLIDER_SHIP] * left) / 100;
            p->slider[PLANET_SLIDER_DEF] = (p->slider[PLANET_SLIDER_DEF] * left) / 100;
            p->slider[PLANET_SLIDER_IND] = (p->slider[PLANET_SLIDER_IND] * left) / 100;
        }
        p->slider[PLANET_SLIDER_ECO] = v;
    }
    SETMAX(p->slider[PLANET_SLIDER_SHIP], 0);
    SETMAX(p->slider[PLANET_SLIDER_DEF], 0);
    SETMAX(p->slider[PLANET_SLIDER_IND], 0);
    SETRANGE(p->slider[PLANET_SLIDER_ECO], 0, 100);
    left = 100;
    left -= p->slider[PLANET_SLIDER_SHIP];
    left -= p->slider[PLANET_SLIDER_DEF];
    left -= p->slider[PLANET_SLIDER_IND];
    left -= p->slider[PLANET_SLIDER_ECO];
    SETMAX(left, 0);
    p->slider[PLANET_SLIDER_TECH] = left;
}

void game_update_eco_on_waste(struct game_s *g, int player_i, bool force_adjust)
{
    empiretechorbit_t *e = &(g->eto[player_i]);
    if (e->race == RACE_SILICOID) {
        return;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        if (p->owner == player_i) {
            game_planet_update_eco_on_waste(g, p, player_i, force_adjust);
        }
    }
}

void game_update_within_range(struct game_s *g)
{
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        uint8_t tblplanet[PLANETS_MAX];
        uint8_t tblplanet_num;
        empiretechorbit_t *e = &(g->eto[pi]);
        bool tbl_alliance[PLAYER_NUM];
        uint8_t frange, frangep3, srange;
        for (int i = 0; i < PLAYER_NUM; ++i) {
            tbl_alliance[i] = (e->treaty[i] == TREATY_ALLIANCE);
        }
        tbl_alliance[pi] = true;
        frange = e->fuel_range;
        frangep3 = frange + 3;
        srange = e->scanner_range;
        tblplanet_num = 0;
        for (int i = 0; i < g->galaxy_stars; ++i) {
            planet_t *p = &(g->planet[i]);
            if ((p->owner != PLAYER_NONE) && tbl_alliance[p->owner]) {
                tblplanet[tblplanet_num++] = i;
            }
        }
        for (int i = 0; i < g->galaxy_stars; ++i) {
            planet_t *p = &(g->planet[i]);
            if (p->owner == pi) {
                p->within_frange[pi] = 1;
                BOOLVEC_SET1(p->within_srange, pi);
            } else {
                uint16_t dist, mindist1, mindist2;
                mindist1 = 0x2710;
                mindist2 = 0x2710;
                for (int j = 0; (j < tblplanet_num) && ((mindist1 > frange) || (mindist2 > srange)); ++j) {
                    uint8_t planet_i2;
                    planet_i2 = tblplanet[j];
                    dist = g->gaux->star_dist[i][planet_i2];
                    SETMIN(mindist1, dist);
                    if ((dist < mindist2) && (g->planet[planet_i2].owner == pi)) {
                        mindist2 = dist;
                    }
                }
                if (mindist1 <= frange) {
                    p->within_frange[pi] = 1;
                } else if (mindist1 <= frangep3) {
                    p->within_frange[pi] = 2;
                } else {
                    p->within_frange[pi] = 0;
                }
                BOOLVEC_SET(p->within_srange, pi, (mindist2 <= srange));
            }
        }
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
            empiretechorbit_t *e = &(g->eto[pi]);
            uint16_t snum;
            snum = 0;
            for (int j = 0; (j < e->shipdesigns_num) && !snum; ++j) {
                snum += e->orbit[i].ships[j];
            }
            if (!snum) {
                for (player_id_t pi2 = PLAYER_0; pi2 < PLAYER_NUM; ++pi2) {
                    BOOLVEC_CLEAR(g->eto[pi2].orbit[i].visible, PLAYER_NUM); /* WASBUG? only cleared bit 0 */
                }
            } else {
                BOOLVEC_SET1(p->within_srange, pi);
            }
        }
    }
}

void game_update_empire_contact(struct game_s *g)
{
    uint8_t tbl_pnum[PLAYER_NUM];
    uint8_t tbl_planet[PLAYER_NUM][PLANETS_MAX];
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        tbl_pnum[pi] = 0;
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        planet_t *p = &(g->planet[i]);
        player_id_t owner;
        owner = p->owner;
        if (owner != PLAYER_NONE) {
            tbl_planet[owner][tbl_pnum[owner]++] = i;
        }
    }
    for (player_id_t pi1 = PLAYER_0; pi1 < g->players; ++pi1) {
        empiretechorbit_t *e1 = &(g->eto[pi1]);
        for (player_id_t pi2 = pi1 + 1; pi2 < g->players; ++pi2) {
            empiretechorbit_t *e2 = &(g->eto[pi2]);
            uint8_t frange, dist, mindist;
            frange = MAX(e1->fuel_range, e2->fuel_range);
            mindist = 0xff;
            for (int i1 = 0; i1 < tbl_pnum[pi1]; ++i1) {
                for (int i2 = 0; i2 < tbl_pnum[pi2]; ++i2) {
                    dist = g->gaux->star_dist[tbl_planet[pi1][i1]][tbl_planet[pi2][i2]];
                    SETMIN(mindist, dist);
                }
            }
            if (mindist <= frange) {
                BOOLVEC_SET1(e1->contact, pi2);
                BOOLVEC_SET1(e2->contact, pi1);
            } else {
                BOOLVEC_SET0(e1->contact, pi2);
                BOOLVEC_SET0(e2->contact, pi1);
            }
        }
    }
}

bool game_check_coord_is_visible(const struct game_s *g, player_id_t pi, int range, int x, int y)
{
    const empiretechorbit_t *e = &(g->eto[pi]);
    range *= 10;    /* 30, 50, 70, 90 */
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        if ((p->owner == pi) && (util_math_dist_fast(x, y, p->x, p->y) <= range)) {
            return true;
        }
    }
    range = (range - 30) / 2;  /* 0, 10, 20, 30 */
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        for (int j = 0; j < e->shipdesigns_num; ++j) {
            if (e->orbit[i].ships[j]) {
                if ((util_math_dist_fast(x, y, p->x, p->y) <= range)) {
                    return true;
                }
                break;
            }
        }
    }
    for (int ei = 0; ei < g->enroute_num; ++ei) {
        const fleet_enroute_t *r = &(g->enroute[ei]);
        if ((r->owner == pi) && (util_math_dist_fast(x, y, r->x, r->y) <= range)) {
            return true;
        }
    }
    return false;
}

void game_update_visibility(struct game_s *g)
{
    for (int ei = 0; ei < g->enroute_num; ++ei) {
        fleet_enroute_t *r = &(g->enroute[ei]);
        for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
            if (r->owner != pi) {
                BOOLVEC_SET(r->visible, pi, game_check_coord_is_visible(g, pi, g->eto[pi].scanner_range, r->x, r->y));
            } else {
                BOOLVEC_SET1(r->visible, pi);
            }
        }
    }
    for (int i = 0; i < g->transport_num; ++i) {
        transport_t *t = &(g->transport[i]);
        for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
            if (t->owner != pi) {
                BOOLVEC_SET(t->visible, pi, game_check_coord_is_visible(g, pi, g->eto[pi].scanner_range, t->x, t->y));
            } else {
                BOOLVEC_SET1(t->visible, pi);
            }
        }
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
            bool visible = BOOLVEC_IS1(p->within_srange, pi);
            for (player_id_t pi2 = PLAYER_0; pi2 < g->players; ++pi2) {
                BOOLVEC_SET(g->eto[pi2].orbit[i].visible, pi, visible);
            }
            BOOLVEC_SET1(g->eto[pi].orbit[i].visible, pi);
        }
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
            fleet_orbit_t *o = &(g->eto[pi].orbit[i]);
            bool any_ships;
            any_ships = false;
            for (int j = 0; j < NUM_SHIPDESIGNS; ++j) {
                if (o->ships[j] != 0) {
                    any_ships = true;
                    break;
                }
            }
            if (!any_ships) {
                BOOLVEC_CLEAR(o->visible, PLAYER_NUM);
            }
        }
    }
}

void game_adjust_slider_group(int16_t *slidertbl, int slideri, int16_t value, int num, const uint16_t *locktbl)
{
    bool have_unlocked = false;
    int first_unlocked_i = 0;
    int last_unlocked_i = 0;
    int left = 100;
    for (int i = 0; i < num; ++i) {
        if (locktbl[i]) {
            left -= slidertbl[i];
        } else {
            last_unlocked_i = i;
            if (!have_unlocked) {
                have_unlocked = true;
                first_unlocked_i = i;
            }
        }
    }
    SETMIN(value, left);
    slidertbl[slideri] = value;
    left -= value;
    for (int i = 0; i < num; ++i) {
        if ((i != slideri) && (!locktbl[i])) {
            if (slidertbl[i] <= left) {
                left -= slidertbl[i];
            } else {
                slidertbl[i] = left;
                left = 0;
            }
        }
    }
    if ((left > 0) && (have_unlocked)) {
        int j;
        if (slideri != last_unlocked_i) {
            j = last_unlocked_i;
        } else {
            j = first_unlocked_i;
        }
        slidertbl[j] += left;
    }
}

void game_equalize_slider_group(int16_t *slidertbl, int num, const uint16_t *locktbl)
{
    int total = 0;
    int num_unlocked = 0;
    for (int i = 0; i < num; ++i) {
        if (!locktbl[i]) {
            total += slidertbl[i];
            ++num_unlocked;
        }
    }
    if (num_unlocked > 0) {
        int n = total / num_unlocked;
        int o = total % num_unlocked;
        for (int i = 0; i < num; ++i) {
            if (!locktbl[i]) {
                int v;
                v = n;
                if (o) {
                    ++v;
                    --o;
                }
                slidertbl[i] = v;
            }
        }
    }
}

int game_planet_get_operating_factories(const struct game_s *g, const struct planet_s *p)
{
    const empiretechorbit_t *e = &g->eto[p->owner];
    int oper_fact;
    if (g->game_mode_extra & GAME_MODE_EXTRA_FACTORY_COST_FIX) {
        oper_fact = (p->pop - p->trans_num) * p->pop_oper_fact;
    } else {
        oper_fact = (p->pop - p->trans_num) * e->colonist_oper_factories;
    }
    SETMIN(oper_fact, p->factories);
    return oper_fact;
}

int game_planet_get_factory_adj_cost(const struct game_s *g, const struct planet_s *p)
{
    const empiretechorbit_t *e = &(g->eto[p->owner]);
    int cost;
    if (e->race == RACE_MEKLAR) {
        cost = e->factory_cost;
    } else {
        if (g->game_mode_extra & GAME_MODE_EXTRA_FACTORY_COST_FIX) {
            cost = (e->factory_cost * p->pop_oper_fact) / 2;
        } else {
            cost = (e->factory_cost * e->colonist_oper_factories) / 2;
        }
    }
    SETMAX(cost, e->factory_cost);
    return cost;
}

int game_get_min_dist(const struct game_s *g, int player_i, int planet_i)
{
    int dist, mindist = 255;
    for (int i = 0; i < g->galaxy_stars; ++i) {
        if ((i != planet_i) && (g->planet[i].owner == player_i)) {
            dist = g->gaux->star_dist[planet_i][i];
            SETMIN(mindist, dist);
        }
    }
    return mindist;
}

int game_adjust_prod_by_special(int prod, planet_special_t special)
{
    switch (special) {
        case PLANET_SPECIAL_ULTRA_POOR:
            prod /= 3;
            break;
        case PLANET_SPECIAL_POOR:
            prod /= 2;
            break;
        case PLANET_SPECIAL_RICH:
            prod *= 2;
            break;
        case PLANET_SPECIAL_ULTRA_RICH:
            prod *= 3;
            break;
        default:
            break;
    }
    return prod;
}

int game_get_pop_growth_max(const struct game_s *g, const planet_t *p, int max_pop3)
{
    const empiretechorbit_t *e = &(g->eto[p->owner]);
    int v, retval;
    if (e->race == RACE_SILICOID) {
        v = (100 - (p->pop * 100) / max_pop3) / 2;
    } else {
        v = 100 - ((p->pop + p->waste) * 100) / max_pop3;
        switch (p->growth) {
            case PLANET_GROWTH_HOSTILE:
                if (v >= 0) {
                    v /= 2;
                } else {
                    v *= 2;
                }
                break;
            case PLANET_GROWTH_NORMAL:
            default:
                break;
            case PLANET_GROWTH_FERTILE:
                if (v >= 0) {
                    v += v / 2;
                } else {
                    v = (v * 2) / 3;
                }
                break;
            case PLANET_GROWTH_GAIA:
                if (v >= 0) {
                    v *= 2;
                } else {
                    v /= 2;
                }
                break;
        }
        if (e->race == RACE_SAKKRA) {
            v *= 2;
        }
    }
    if (g->game_mode_extra & GAME_MODE_EXTRA_POPULATION_GROWTH_FIX) {
        retval = (v * p->pop + 5) / 100;
    } else {
        retval = (v * p->pop + 5) / 100 + p->pop_tenths;
    }
    if ((v > 0) && (retval < 1)) {
        retval = 1;
    }
    if ((p->pop + retval / 10) < 1) {
        retval = 1;
    }
    return retval;
}

int game_get_pop_growth_for_eco(const struct game_s *g, const planet_t *p, int eco)
{
    const empiretechorbit_t *e = &(g->eto[p->owner]);
    int v, vmax;
    v = (eco * 10) / e->inc_pop_cost;
    if (e->race == RACE_SAKKRA) {
        v *= 2;
    } else if (e->race == RACE_SILICOID) {
        v /= 2;
    }
    vmax = (p->pop * 10) / 4;
    return MIN(v, vmax);
}

int game_get_tech_prod(int prod, int slider, race_t race, planet_special_t special)
{
    int v = (prod * slider) / 100;
    if (race == RACE_PSILON) {
        v += v / 2;
    }
    switch (special) {
        case PLANET_SPECIAL_ARTIFACTS:
            v *= 2;
            break;
        case PLANET_SPECIAL_4XTECH:
            v *= 4;
            break;
        default:
            break;
    }
    return MIN(v, 0x7fff);
}

void game_print_prod_of_total(const struct game_s *g, player_id_t pi, int prod, char *buf, size_t bufsize)
{
    int v = g->eto[pi].total_production_bc;
    if (v == 0) {
        lib_strcpy(buf, "0%%", bufsize);
    } else {
        v = (prod * 1000) / v;
        lib_sprintf(buf, bufsize, "%i.%i%%", v / 10, v % 10);
    }
}

bool game_xy_is_in_nebula(const struct game_s *g, int x, int y)
{
    for (int n = 0; n < g->nebula_num; ++n) {
        for (int i = 0; i < 4; ++i) {
            if (1
              && (x >= g->nebula_x0[n][i])
              && (x <= g->nebula_x1[n][i])
              && (y >= g->nebula_y0[n][i])
              && (y <= g->nebula_y1[n][i])
            ) {
                return true;
            }
        }
    }
    return false;
}

int game_calc_eta_ship(const struct game_s *g, int speed, int x0, int y0, int x1, int y1)
{
    int x, y, num = 0;
    x = x0;
    y = y0;
    while ((x != x1) || (y != y1)) {
        int v;
        bool first;
        v = speed;
        first = true;
        do {
            /* Inside nebulas, ships move on frame 0 and then on every odd frame. */
            if (first || (!game_xy_is_in_nebula(g, x, y))) {
                util_math_go_line_dist(&x, &y, x1, y1, 5);
            }
            util_math_go_line_dist(&x, &y, x1, y1, 6);
            first = false;
            --v;
        } while (v > 0);
        ++num;
    }
    return num;
}

int game_calc_eta_trans(const struct game_s *g, int speed, int x0, int y0, int x1, int y1)
{
    int x = x0, y = y0, num = 0;
    while ((x != x1) || (y != y1)) {
        int v = speed;
        do {
            /* Inside nebulas, transports move on every even frame. */
            util_math_go_line_dist(&x, &y, x1, y1, 5);
            if (!game_xy_is_in_nebula(g, x, y)) {
                util_math_go_line_dist(&x, &y, x1, y1, 6);
            }
            --v;
        } while (v > 0);
        ++num;
    }
    return num;
}

bool game_transport_dest_ok(const struct game_s *g, const planet_t *p, player_id_t pi)
{
    return (p->within_frange[pi] == 1)
        && BOOLVEC_IS1(p->explored, pi)
        && (p->type >= g->eto[pi].have_colony_for)
        && (p->owner != PLAYER_NONE)
        ;
}

void game_rng_step(struct game_s *g)
{
    /* TODO disable for multiplayer */
    if (!game_num_deterministic) {
        rnd_0_nm1(2, &g->seed);
    }
}

int game_get_maxpop(const struct game_s *g, const planet_t *p)
{
    player_id_t owner = p->owner;
    const empiretechorbit_t *e = e = &(g->eto[owner]);
    if (owner == PLAYER_NONE) return 0;
    int type = p->type, growth = p->growth;
    int v, p1 = p->max_pop1, p2 = p-> max_pop2;
    if (e->have_atmos_terra && (p->growth == PLANET_GROWTH_HOSTILE)) {
         if (type < PLANET_TYPE_DEAD) {
             v = 20;
         } else if (type < PLANET_TYPE_BARREN) {
             v = 10;
         } else {
             v = 0;
         }
        type = PLANET_TYPE_MINIMAL;
        p2 += v; p1 += v;
        growth = PLANET_GROWTH_NORMAL;
    }
    if (e->have_soil_enrich && (growth == PLANET_GROWTH_NORMAL)) {
        growth = PLANET_GROWTH_FERTILE;
        if (game_num_soil_rounding_fix) {
            v = ((p1 + 19) / 20) * 5;
        } else {
            v = (p1 / 20) * 5;
            if ((p1 / 20) % 5) {
                v += 5;
            }
        }
        SETMAX(v, 5);
        p2 = p1 + v;
    }
    if (e->have_adv_soil_enrich && (growth < PLANET_GROWTH_GAIA) && (growth > PLANET_GROWTH_HOSTILE)) {
        growth = PLANET_GROWTH_GAIA;
        if (game_num_soil_rounding_fix) {
            v = ((p1 + 9) / 10) * 5;
        } else {
            v = (p1 / 10) * 5;
            if ((p1 / 10) % 5) {
                v += 5;
            }
        }
        SETMAX(v, 5);
        p2 = p1 + v;
    }
    v = p2 + e->have_terraform_n;
    if (!game_num_reset_tform_to_max) SETMAX(v, p2 + p->max_pop3 - p->max_pop2);
    SETMIN(v, game_num_max_pop);
    return v;
}

int game_get_maxprod(const struct game_s *g, const planet_t *p)
{
    player_id_t owner = p->owner;
    const empiretechorbit_t *e = e = &(g->eto[owner]);
    int pop = game_get_maxpop(g, p);
    int ind = pop * e->colonist_oper_factories;
    int man = pop * (e->tech.percent[TECH_FIELD_PLANETOLOGY] * 3 + 50) / 100;
    if (e->race == RACE_KLACKON) man <<= 1;
    return ind + man;
}

/* Max. population values: Each planet has three max_pop variables which are
 * affected by the ECO projects in different ways.
 *
 * The base size, max_pop1, is the size of the planet at the start of the game.
 * It is used to determine the increase in max. population from (advanced) soil
 * enrichment; max_pop1 itself is increased only when applying atmospheric
 * terraforming to some types of hostile planets.
 *
 * max_pop2 is max_pop1 modified by (advanced) soil enrichment. Note that for
 * planets that are Fertile or Gaia at the start of the game,
 * max_pop1 == max_pop2. They are only different when the planets were made
 * Fertile or Gaia by a player (human or AI).
 *
 * max_pop3 is the actual population maximum. It equals max_pop2 modified by
 * terraforming and/or by bioweapon damage.
 */
void game_atmos_tform(struct planet_s *p) {
    int max_pop_increase;
    if (p->type < PLANET_TYPE_DEAD) {
        max_pop_increase = 20;
    } else if (p->type < PLANET_TYPE_BARREN) {
        max_pop_increase = 10;
    } else {
        max_pop_increase = 0;
    }
    /* WASBUG max_pop += moved from outside if (bc >= cost) */
    p->type = PLANET_TYPE_MINIMAL;
    p->max_pop1 += max_pop_increase;
    p->max_pop2 += max_pop_increase;
    p->max_pop3 += max_pop_increase;
    p->growth = PLANET_GROWTH_NORMAL;
}

void game_soil_enrich(struct planet_s *p, int best_tform, bool advanced) {
    int max_pop_increase = 0;
    int16_t old_max_pop2 = 0;
    p->growth = advanced ? PLANET_GROWTH_GAIA : PLANET_GROWTH_FERTILE;
    if (advanced) {
        if (game_num_soil_rounding_fix) {
            max_pop_increase = ((p->max_pop1 + 9) / 10) * 5;
        } else {
            max_pop_increase = (p->max_pop1 / 10) * 5;
            /* BUG? If we want to calculate 50% of the base size, rounded up
            * to the next multiple of 5, we'd check if p->max_pop1 % 10 != 0
            * below. Instead, we add an additional +5 unless the base size is
            * in the range 50-59 or 100-109. Penalizing these specific sizes
            * seems weird and arbitrary. */
            if ((p->max_pop1 / 10) % 5) {
                max_pop_increase += 5;
            }
        }
    } else {
        if (game_num_soil_rounding_fix) {
            max_pop_increase = ((p->max_pop1 + 19) / 20) * 5;
        } else {
            max_pop_increase = (p->max_pop1 / 20) * 5;
            /* BUG? See above. */
            if ((p->max_pop1 / 20) % 5) {
                max_pop_increase += 5;
            }
        }
    }
    SETMAX(max_pop_increase, 5);
    old_max_pop2 = p->max_pop2;
    p->max_pop2 = p->max_pop1 + max_pop_increase;
    p->max_pop3 += p->max_pop2 - old_max_pop2;
    if (game_num_reset_tform_to_max) {
        /* BUG? Having p->max_pop3 > (best_tform + p->max_pop2) seems
         * only possible when conquering a planet from a race with better
         * terraforming tech. Is it intended that the player loses the
         * more advanced terraforming here, or is this a sanity check
         * with unintended consequences? */
        SETMIN(p->max_pop3, (best_tform + p->max_pop2));
    }
    SETMIN(p->max_pop3, game_num_max_pop);
}

