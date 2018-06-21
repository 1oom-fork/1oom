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

static void increment_slider(planet_t *p, int slideri)
{
    /* increment */
    int v = p->slider[slideri] + 4;
    SETMIN(v, 100);
    p->slider[slideri] = v;
    /* rebalance */
    game_adjust_slider_group(p->slider, slideri, p->slider[slideri], PLANET_SLIDER_NUM, p->slider_lock);
}
static void set_slider(planet_t *p, int slideri, int16_t value)
{
    /* set to given value. Don't check min/max, as we assume given value was already set and is valid */
    p->slider[slideri] = value;
    /* rebalance */
    game_adjust_slider_group(p->slider, slideri, p->slider[slideri], PLANET_SLIDER_NUM, p->slider_lock);
}

static bool slider_text_equals(const struct game_s *g, planet_t *p, int slideri, const char *string)
{
    char buf[64];

    game_planet_get_slider_text(g, p, p->owner, slideri, buf);

    return strcmp(string, buf) == 0;
}

/* Move eco slider right until "WASTE" disappears */
static void move_eco_min(const struct game_s *g, planet_t *p)
{
    int slideri = PLANET_SLIDER_ECO;
    int previous_allocation;
    do {
        if (!slider_text_equals(g, p, slideri, game_str_sm_ecowaste)) {
            break;
        }
        previous_allocation = p->slider[slideri];
        increment_slider(p, slideri);
    } while (previous_allocation != p->slider[slideri]);
}

/* Move industry slider right until "RESERVE" appears. Then move 1 tick back */
static void move_ind_max(const struct game_s *g, planet_t *p)
{
    int slideri = PLANET_SLIDER_IND;
    int previous_allocation = p->slider[slideri];
    do {
        if (slider_text_equals(g, p, slideri, game_str_sm_indres)) {
            /* restore previous value, to stop going into reserve. */
            set_slider(p, slideri, previous_allocation);
            break;
        }
        if (slider_text_equals(g, p, slideri, game_str_sm_indmax)) {
            /* keep the value that went into "MAX". */
            break;
        }

        previous_allocation = p->slider[slideri];
        increment_slider(p, slideri);
    } while (previous_allocation != p->slider[slideri]);
}
/* Move eco slider right until "CLEAR" or "NONE" disappear. If they never disappear, move slider back */
static void move_eco_max(const struct game_s *g, planet_t *p)
{
    int slideri = PLANET_SLIDER_ECO;
    int original_allocation = p->slider[slideri];
    bool found = false;
    int previous_allocation;
    do {
        if (slider_text_equals(g, p, slideri, game_str_sm_ecoclean) || slider_text_equals(g, p, slideri, game_str_sm_prodnone)) {
            if (found) {
                /* we already moved past "Terraform" back into "Clean", break */
                break;
            }
        } else {
            found = true;
        }
        previous_allocation = p->slider[slideri];
        increment_slider(p, slideri);
    } while (previous_allocation != p->slider[slideri]);

    if (!found) {
        set_slider(p, slideri, original_allocation);
    }
}
/* Move DEF slider, if "UPGRD" or "SHIELD" appears, do that, otherwise move it back */
static void move_def_min(const struct game_s *g, planet_t *p)
{
    int slideri = PLANET_SLIDER_DEF;
    int original_allocation = p->slider[slideri];
    bool found = false;
    int previous_allocation;
    do {
        if (slider_text_equals(g, p, slideri, game_str_sm_defupg) || slider_text_equals(g, p, slideri, game_str_sm_defshld)) {
            found = true;
        } else {
            if (found) {
                /* we already moved past "upgrade"/"shield" back into "build bases", break */
                break;
            }
        }
        previous_allocation = p->slider[slideri];
        increment_slider(p, slideri);
    } while (previous_allocation != p->slider[slideri]);

    if (!found) {
        set_slider(p, slideri, original_allocation);
    }
}

/* Move ship slider right until "1 Y" appears. Then stop moving it */
static void move_ship_1(const struct game_s *g, planet_t *p)
{
    int slideri = PLANET_SLIDER_SHIP;
    int previous_allocation = p->slider[slideri];
    do {
        if (slider_text_equals(g, p, slideri, "1 Y")) {
            break;
        }

        previous_allocation = p->slider[slideri];
        increment_slider(p, slideri);
    } while (previous_allocation != p->slider[slideri]);
}

/* -------------------------------------------------------------------------- */

void game_planet_destroy(struct game_s *g, uint8_t planet_i, player_id_t attacker)
{
    planet_t *p = &(g->planet[planet_i]);
    player_id_t owner = p->owner;
    if (1/*IS_HUMAN(g, owner)*/) {
        g->seen[owner][planet_i].owner = PLAYER_NONE;
        g->seen[owner][planet_i].pop = 0;
        g->seen[owner][planet_i].bases = 0;
        g->seen[owner][planet_i].factories = 0;
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
    /* stop governor for this planet */
    BOOLVEC_SET0(p->extras, PLANET_EXTRAS_GOVERNOR);
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
                    p->slider[PLANET_SLIDER_DEF] = v;
                } else {
                    p->slider[PLANET_SLIDER_DEF] = (p->slider[PLANET_SLIDER_DEF] * (100 - percent)) / 100;
                }
                if (si == PLANET_SLIDER_IND) {
                    p->slider[PLANET_SLIDER_IND] = v;
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

int game_planet_get_slider_text(const struct game_s *g, const planet_t *p, player_id_t player, planet_slider_i_t si, char *buf)
{
    const empiretechorbit_t *e = &(g->eto[player]);
    int retval = -1;
    switch (si) {
        case PLANET_SLIDER_SHIP:
            {
                int vthis, vtotal, cost;
                vthis = game_adjust_prod_by_special((p->prod_after_maint * p->slider[PLANET_SLIDER_SHIP]) / 100, p->special);
                vtotal = vthis + p->bc_to_ship;
                if (p->buildship == BUILDSHIP_STARGATE) {
                    cost = game_num_stargate_cost;
                } else {
                    cost = g->srd[player].design[p->buildship].cost;
                }
                if ((vtotal < cost) || (p->buildship == BUILDSHIP_STARGATE)) {
                    if (vthis < 1) {
                        strcpy(buf, game_str_sm_prodnone);
                        retval = 0;
                    } else {
                        int num = 0, over;
                        over = cost - p->bc_to_ship;
                        while (over > 0) {
                            over -= vthis;
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
                    num = vtotal / cost;
                    sprintf(buf, "1 %s", game_str_sm_prod_y);
                    retval = num;
                }
            }
            break;
        case PLANET_SLIDER_DEF:
            {
                int vthis, vtotal, cost, v8, va;
                cost = game_get_base_cost(g, player);
                vthis = game_adjust_prod_by_special((p->prod_after_maint * p->slider[PLANET_SLIDER_DEF]) / 100, p->special);
                vtotal = vthis + p->bc_to_base;
                v8 = p->bc_upgrade_base;
                if (vthis == 0) {
                    strcpy(buf, game_str_sm_prodnone);
                } else if (vtotal <= v8) {
                    strcpy(buf, game_str_sm_defupg);
                } else {
                    vtotal -= v8;
                    SETMAX(vtotal, 0);
                    va = e->planet_shield_cost - p->bc_to_shield;
                    if (p->battlebg == 0) {
                        va = 0;
                    }
                    SETMAX(va, 0);
                    if (vtotal <= va) {
                        strcpy(buf, game_str_sm_defshld);
                    } else {
                        int num, over;
                        vtotal -= va;
                        SETMAX(vtotal, 0);
                        if ((cost * 2) > vtotal) {
                            num = 1;
                            over = cost - vtotal;
                            while (over > 0) {
                                over -= vthis;
                                ++num;
                            }
                            sprintf(buf, "%i %s", num, game_str_sm_prod_y);
                        } else {
                            num = vtotal / cost;
                            sprintf(buf, "%i/%s", num, game_str_sm_prod_y);
                        }
                    }
                }
            }
            break;
        case PLANET_SLIDER_IND:
            {
                const char *str = 0;
                int vthis, cost;
                cost = e->factory_adj_cost;
                vthis = game_adjust_prod_by_special((p->prod_after_maint * p->slider[PLANET_SLIDER_IND]) / 100, p->special);
                if (vthis != 0) {
                    int v20;
                    v20 = (vthis * 10) / cost;
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
            {
                const char *str = NULL;
                int vthis, factoper, waste, adjwaste;
                bool flag_tform = false, flag_ecoproj = false;
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
                    if (e->race != RACE_SILICOID) {
                        vthis -= adjwaste;
                        SETMAX(vthis, 0);
                        if ((vthis > 0) && (e->have_atmos_terra) && (p->growth == PLANET_GROWTH_HOSTILE)) {
                            vthis -= game_num_atmos_cost - p->bc_to_ecoproj;
                            if (vthis < 0) {
                                flag_ecoproj = true;
                                str = game_str_sm_ecoatmos;
                            }
                        }
                        if ((vthis > 0) && (e->have_soil_enrich) && (p->growth == PLANET_GROWTH_NORMAL)) {
                            vthis -= game_num_soil_cost - p->bc_to_ecoproj;
                            if (vthis < 0) {
                                flag_ecoproj = true;
                                str = game_str_sm_ecosoil;
                            }
                        }
                        if ((vthis > 0) && (e->have_adv_soil_enrich) && ((p->growth == PLANET_GROWTH_NORMAL) || (p->growth == PLANET_GROWTH_FERTILE))) {
                            vthis -= game_num_adv_soil_cost - p->bc_to_ecoproj;
                            if (vthis < 0) {
                                flag_ecoproj = true;
                                str = game_str_sm_ecogaia;
                            }
                        }
                    }
                    flag_tform = false;
                    if (vthis > 0) {
                        if (p->max_pop3 < game_num_max_pop) {
                            adjwaste = (p->max_pop2 + (e->have_terraform_n - p->max_pop3)) * e->terraform_cost_per_inc;
                        } else {
                            adjwaste = 0;
                        }
                        if (vthis < adjwaste) {
                            str = game_str_sm_ecotform;
                        } else {
                            int growth, growth2;
                            if (adjwaste > 0) {
                                flag_tform = true;
                            }
                            vthis -= adjwaste;
                            growth = game_get_pop_growth_max(g, p, p->max_pop3) + p->pop_tenths;
                            if (((p->pop - p->trans_num) + (growth / 10)) > p->max_pop3) {
                                growth = (p->max_pop3 - (p->pop - p->trans_num)) * 10;
                            }
                            growth2 = game_get_pop_growth_for_eco(g, p, vthis) + growth;
                            if (((p->pop - p->trans_num) + (growth2 / 10)) > p->max_pop3) {
                                growth2 = (p->max_pop3 - (p->pop - p->trans_num)) * 10;
                            }
                            growth = growth2 / 10 - growth / 10;
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
            }
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

/**
 * Govern the colony.
 * - First, set ecology to minimum that avoids waste.
 * - Then move industry slider until maximum or reserve achieved.
 * - Then move ecology slider if can terraform or grow population until "clean" is achieved.
 * - Then set defense to upgrade bases or build shield. Will not build new bases.
 * - Then set shipbuilding to minimum required to build stargate if technology is present.
 * - If all of the above are finished, do research
 *
 * This works by moving slider by 1 tick (4%) until desired results happen. Implemented
 * this way to avoid duplication of planet production logic.
 *
 */

void game_planet_govern(const struct game_s *g, planet_t *p)
{
    player_id_t player = p->owner;
    const empiretechorbit_t *e = &(g->eto[player]);
    /* unlock all sliders */
    for (planet_slider_i_t i = 0; i < PLANET_SLIDER_NUM; ++i) {
        p->slider_lock[i] = false;
    }
    /* clear spending- all goes to research */
    for (planet_slider_i_t i = 0; i < PLANET_SLIDER_NUM; ++i) {
        if (i != PLANET_SLIDER_TECH) {
            p->slider[i] = 0;
            game_adjust_slider_group(p->slider, i, p->slider[i], PLANET_SLIDER_NUM, p->slider_lock);
        } else {
            p->slider[i] = 10000;
            game_adjust_slider_group(p->slider, i, p->slider[i], PLANET_SLIDER_NUM, p->slider_lock);
        }
    }
    /* set eco to minimum & lock it, keep increasing until "WASTE" disappears */
    move_eco_min(g, p);
    p->slider_lock[PLANET_SLIDER_ECO] = true;
    /* Add maximum industry if factories would actually get used, until we get MAX or RESERVE */
    move_ind_max(g, p);
    p->slider_lock[PLANET_SLIDER_IND] = true;
    /* Add ecology for terraforming/pop growth, until "CLEAN" is displayed */
    p->slider_lock[PLANET_SLIDER_ECO] = false;
    move_eco_max(g, p);
    p->slider_lock[PLANET_SLIDER_ECO] = true;
    /* For defense, since we don't have a limit to missile base number, only do upgrades/shields.
       Click right until we get a number, if we get a number, move back */
    move_def_min(g, p);
    if (e->have_stargates && !p->have_stargate) {
        /* Lock def not to use it up for stargate construction */
        p->slider_lock[PLANET_SLIDER_DEF] = true;
        /* build stargate */
        p->buildship = BUILDSHIP_STARGATE;
        /* move ship slider right until stargate can be built in "1 y" */
        move_ship_1(g, p);
        /* unlock def */
        p->slider_lock[PLANET_SLIDER_DEF] = false;
    }
    /* at the end of govern, keep only eco slider locked to allow easy one-turn manual tweaks */
    p->slider_lock[PLANET_SLIDER_IND] = false;
}
