#include "config.h"

#include <stdio.h>
#include <string.h>

#include "game_planet.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_str.h"
#include "game_tech.h"
#include "rnd.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

static int tenths_2str(char *buf, int num)
{
    int pos;
    pos = sprintf(buf, "%i", num / 10);
    if ((num < 100) && ((num % 10) != 0)) {
        pos += sprintf(&buf[pos], ".%i", num % 10);
    }
    return pos;
}

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
    BOOLVEC_CLEAR(p->finished, FINISHED_NUM);
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

void game_planet_adjust_percent(struct game_s *g, player_id_t owner, planet_slider_i_t si, uint8_t percent, int growth)
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
                if (si != PLANET_SLIDER_DEF) {
                    sum += p->slider[PLANET_SLIDER_DEF];
                }
                if (si != PLANET_SLIDER_IND) {
                    sum += p->slider[PLANET_SLIDER_IND];
                }
                v = (sum * percent) / 100;
                p->slider[PLANET_SLIDER_SHIP] = (p->slider[PLANET_SLIDER_SHIP] * (100 - percent)) / 100;
                if (si == PLANET_SLIDER_DEF) {
                    p->slider[PLANET_SLIDER_DEF] += v;
                } else {
                    p->slider[PLANET_SLIDER_DEF] = (p->slider[PLANET_SLIDER_DEF] * (100 - percent)) / 100;
                }
                if (si == PLANET_SLIDER_IND) {
                    p->slider[PLANET_SLIDER_IND] += v;
                } else {
                    p->slider[PLANET_SLIDER_IND] = (p->slider[PLANET_SLIDER_IND] * (100 - percent)) / 100;
                }
                if (si == PLANET_SLIDER_ECO) {
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

void game_planet_update_home(struct game_s *g)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p = &(g->planet[i]);
        player_id_t pi;
        pi = p->owner;
        if ((pi != PLAYER_NONE) && (g->evn.home[pi] == PLANET_NONE)) {
            g->evn.home[pi] = i;   /* WASBUG? MOO1 sets to 0 which affects rebellion event check */
        }
    }
}

const char *game_planet_get_finished_text(const struct game_s *g, const planet_t *p, planet_finished_t type, char *buf)
{
    int num;
    switch (type) {
        case FINISHED_FACT:
            num = p->max_pop3 * g->eto[p->owner].colonist_oper_factories;
            sprintf(buf, "%s %s %s %i %s. %s", p->name, game_str_sm_hasreached, game_str_sm_indmaxof, num, game_str_sm_factories, game_str_sm_extrares);
            break;
        case FINISHED_POPMAX:
            num = p->max_pop3;
            sprintf(buf, "%s %s %s %i %s. %s", p->name, game_str_sm_hasreached, game_str_sm_popmaxof, num, game_str_sm_colonists, game_str_sm_extrares);
            break;
        case FINISHED_SOILATMOS:
            sprintf(buf, "%s %s %s %s %s%s.", p->name, game_str_sm_hasterraf, game_str_tbl_sm_terraf[p->growth - 1], game_str_sm_envwith, game_str_tbl_sm_envmore[p->growth - 1], game_str_sm_stdgrow);
            break;
        case FINISHED_STARGATE:
            sprintf(buf, "%s %s.", p->name, game_str_sm_hasfsgate);
            break;
        case FINISHED_SHIELD:
            sprintf(buf, "%s %s %s %s.", p->name, game_str_sm_hasfshield, game_str_tbl_roman[p->shield], game_str_sm_planshield);
            break;
        default:
            sprintf(buf, "BUG: invalid finished type %i at '%s'", type, p->name);
            break;
    }
    return buf;
}

int game_planet_get_slider_text(const struct game_s *g, uint8_t planet_i, player_id_t player, planet_slider_i_t si, char *buf)
{
    const planet_t *p = &(g->planet[planet_i]);
    const empiretechorbit_t *e = &(g->eto[player]);
    int retval = -1;
    switch (si) {
        case PLANET_SLIDER_SHIP:
            {
                struct planet_prod_s prod;
                int cost;
                game_planet_get_ship_prod(p, &prod, false);
                if (p->buildship == BUILDSHIP_STARGATE) {
                    cost = game_num_stargate_cost;
                } else {
                    cost = g->srd[player].design[p->buildship].cost;
                }
                if ((prod.vtotal < cost) || (p->buildship == BUILDSHIP_STARGATE)) {
                    if (prod.vthis < 1) {
                        strcpy(buf, game_str_sm_prodnone);
                        retval = 0;
                    } else {
                        int num = 0, over;
                        over = cost - p->bc_to_ship;
                        while (over > 0) {
                            over -= prod.vthis;
                            ++num;
                        }
                        SETMAX(num, 1);
                        sprintf(buf, "%i %s", num, game_str_sm_prod_y);
                        if (p->buildship != BUILDSHIP_STARGATE) {
                            retval = 1;
                        }
                    }
                } else {
                    int num;
                    /* TODO this adjusted sd->cost directly! */
                    SETMAX(cost, 1);
                    num = prod.vtotal / cost;
                    sprintf(buf, "1 %s", game_str_sm_prod_y);
                    retval = num;
                }
            }
            break;
        case PLANET_SLIDER_DEF:
            {
                struct planet_prod_s prod;
                int cost, v8, va;
                cost = game_get_base_cost(g, player);
                game_planet_get_def_prod(p, &prod);
                v8 = p->bc_upgrade_base;
                if (prod.vthis == 0) {
                    strcpy(buf, game_str_sm_prodnone);
                } else if (prod.vtotal <= v8) {
                    strcpy(buf, game_str_sm_defupg);
                } else {
                    prod.vtotal -= v8;
                    SETMAX(prod.vtotal, 0);
                    va = e->planet_shield_cost - p->bc_to_shield;
                    if (p->battlebg == 0) {
                        va = 0;
                    }
                    SETMAX(va, 0);
                    if (prod.vtotal <= va) {
                        strcpy(buf, game_str_sm_defshld);
                    } else {
                        int num, over;
                        prod.vtotal -= va;
                        SETMAX(prod.vtotal, 0);
                        if ((cost * 2) > prod.vtotal) {
                            num = 1;
                            over = cost - prod.vtotal;
                            while (over > 0) {
                                over -= prod.vthis;
                                ++num;
                            }
                            sprintf(buf, "%i %s", num, game_str_sm_prod_y);
                        } else {
                            num = prod.vtotal / cost;
                            sprintf(buf, "%i/%s", num, game_str_sm_prod_y);
                        }
                    }
                }
            }
            break;
        case PLANET_SLIDER_IND:
            {
                const char *str = 0;
                struct planet_prod_s prod;
                int cost;
                cost = e->factory_adj_cost;
                game_planet_get_ind_prod(p, &prod);
                if (prod.vthis != 0) {
                    int v20;
                    v20 = (prod.vthis * 10) / cost;
                    if ((v20 / 10 + p->factories) >= ((p->pop - p->trans_num) * p->pop_oper_fact)) {
                        if (p->pop_oper_fact < e->colonist_oper_factories) {
                            if (e->race != RACE_MEKLAR) {
                                str = game_str_sm_refit;
                            } else {
                                int pos = tenths_2str(buf, v20);
                                sprintf(&buf[pos], "/%s", game_str_sm_prod_y);
                            }
                        } else {
                            str = game_str_sm_indres;
                            if (p->factories < (p->max_pop3 * e->colonist_oper_factories)) {
                                str = game_str_sm_indmax;
                            }
                        }
                    } else {
                        int pos = tenths_2str(buf, v20);
                        sprintf(&buf[pos], "/%s", game_str_sm_prod_y);
                    }
                } else {
                    str = game_str_sm_prodnone;
                }
                if (str) {
                    strcpy(buf, str);
                }
            }
            break;
        case PLANET_SLIDER_ECO:
            retval = game_planet_get_slider_text_eco(g, planet_i, player, false, buf);
            break;
        case PLANET_SLIDER_TECH:
            {
                int v = game_get_tech_prod(p->prod_after_maint, p->slider[PLANET_SLIDER_TECH], e->race, p->special);
                sprintf(buf, "%i", v);
                retval = v;
            }
            break;
        default:
            *buf = '\0';
            break;
    }
    return retval;
}

int game_planet_get_slider_text_eco(const struct game_s *g, uint8_t planet_i, player_id_t player, bool flag_tenths, char *buf)
{
    const planet_t *p = &(g->planet[planet_i]);
    const empiretechorbit_t *e = &(g->eto[player]);
    int retval = -1;
    const char *str = NULL;
    int vthis, factoper, waste, adjwaste;
    bool flag_ecoproj = false;
    vthis = (p->prod_after_maint * p->slider[PLANET_SLIDER_ECO]) / 100;
    factoper = (p->pop - p->trans_num) * e->colonist_oper_factories;
    SETMIN(factoper, p->factories);
    waste = (factoper * e->ind_waste_scale) / 10;
    if (e->race == RACE_SILICOID) {
        adjwaste = 0;
    } else {
        adjwaste = (waste + p->waste) / e->have_eco_restoration_n;
    }
    if ((vthis < adjwaste) || (vthis == 0)) {
        str = (vthis < adjwaste) ? game_str_sm_ecowaste : game_str_sm_prodnone;
    } else {
        SUBSAT0(vthis, adjwaste);
        if ((vthis > 0) && e->have_atmos_terra && (p->growth == PLANET_GROWTH_HOSTILE)) {
            vthis -= game_num_atmos_cost - p->bc_to_ecoproj;
            if (vthis < 0) {
                flag_ecoproj = true;
                str = game_str_sm_ecoatmos;
            }
        }
        if ((vthis > 0) && e->have_soil_enrich && (p->growth == PLANET_GROWTH_NORMAL)) {
            vthis -= game_num_soil_cost - p->bc_to_ecoproj;
            if (vthis < 0) {
                flag_ecoproj = true;
                str = game_str_sm_ecosoil;
            }
        }
        if ((vthis > 0) && e->have_adv_soil_enrich && ((p->growth == PLANET_GROWTH_NORMAL) || (p->growth == PLANET_GROWTH_FERTILE))) {
           vthis -= game_num_adv_soil_cost - p->bc_to_ecoproj;
           if (vthis < 0) {
                flag_ecoproj = true;
                str = game_str_sm_ecogaia;
            }
        }
        if (vthis > 0) {
            if (p->max_pop3 < game_num_max_pop) {
                adjwaste = (p->max_pop2 + (e->have_terraform_n - p->max_pop3)) * e->terraform_cost_per_inc;
            } else {
                adjwaste = 0;
            }
            if (vthis < adjwaste) {
                str = game_str_sm_ecotform;
            } else {
                int growth, growth2, max_pop;
                bool flag_tform = false;
                max_pop = p->max_pop3;
                if (adjwaste > 0) {
                    if (vthis < adjwaste) {
                        flag_tform = true;
                    } else if (flag_tenths) {   /* keep same +N as MOO1 for developing planets on -nouiextra */
                        max_pop = p->max_pop2 + e->have_terraform_n;
                    }
                }
                vthis -= adjwaste;
                growth = game_get_pop_growth_max(g, planet_i, max_pop) + p->pop_tenths;
                if (!flag_tenths) {
                    if (((p->pop - p->trans_num) + (growth / 10)) > max_pop) {
                        growth = (max_pop - (p->pop - p->trans_num)) * 10;
                    }
                    growth2 = game_get_pop_growth_for_eco(g, planet_i, vthis) + growth;
                    if (((p->pop - p->trans_num) + (growth2 / 10)) > max_pop) {
                        growth2 = (max_pop - (p->pop - p->trans_num)) * 10;
                    }
                    growth = growth2 / 10 - growth / 10;
                } else {
                    if (((p->pop - p->trans_num) * 10 + growth) > (max_pop * 10)) {
                        growth = (max_pop - (p->pop - p->trans_num)) * 10 + p->pop_tenths;
                    }
                    growth2 = game_get_pop_growth_for_eco(g, planet_i, vthis) + growth;
                    if (((p->pop - p->trans_num) * 10 + growth2) > (max_pop * 10)) {
                        growth2 = (max_pop - (p->pop - p->trans_num)) * 10 + p->pop_tenths;
                    }
                    growth = growth2 - growth;
                }
                if (growth <= 0) {
                    str = flag_tform ? game_str_sm_ecotform : game_str_sm_ecoclean;
                } else {
                    retval = growth;
                    str = game_str_sm_ecopop;
                }
            }
        } else {
            if ((flag_ecoproj == false) && (e->race != RACE_SILICOID)) {
                str = game_str_sm_ecoclean;
            }
        }
    }
    if (str) {
        strcpy(buf, str);
    } else {
        *buf = '\0';
    }
    return retval;
}
