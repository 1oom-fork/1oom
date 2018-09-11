#include "config.h"

#include <stdio.h>

#include "game_tech.h"
#include "bits.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_ai.h"
#include "game_aux.h"
#include "game_num.h"
#include "game_shiptech.h"
#include "game_str.h"
#include "game_techtypes.h"
#include "log.h"
#include "rnd.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

const uint8_t tech_reduce_50percent_per_10pts[] = {
    0, 100, 93, 87, 81, 76, 71, 66, 62, 58, 54, 50, 47, 44, 41, 38,
    35, 33, 31, 29, 27, 25, 23, 22, 20, 19, 18, 16, 15, 14, 13, 13,
    12, 11, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 5, 4, 4, 4, 4, 3, 3
};

const uint8_t tech_reduce_25percent_per_10pts[] = {
    100, 97, 94, 92, 89, 87, 84, 82, 79, 77, 75, 73, 71, 69, 67, 65,
    63, 61, 60, 58, 56, 55, 53, 52, 50, 49, 47, 46, 45, 43, 42, 41,
    40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 27, 26,
    25, 24, 24
};

/* -------------------------------------------------------------------------- */

static uint8_t find_byte_in_tbl(uint8_t b, const uint8_t *tbl, uint32_t len)
{
    while (len) {
        if (*tbl++ == b) {
            return b;
        }
        --len;
    }
    return 0;
}

static uint16_t get_base_cost_mod_armor(const struct game_s *g, int player_i, int percent)
{
    uint16_t tech_i = 0;
    uint8_t mult;
    for (int i = 0; i < SHIP_ARMOR_NUM; ++i) {
        if (game_tech_player_has_tech(g, TECH_FIELD_CONSTRUCTION, tbl_shiptech_armor[i].tech_i, player_i)) {
            tech_i = i;
        }
    }
    if (tech_i) {
        --tech_i;
    }
    if (percent < 50) {
        mult = tech_reduce_50percent_per_10pts[percent];
    } else {
        mult = 3;
    }
    return ((tbl_shiptech_armor[tech_i].cost[SHIP_HULL_LARGE] + tbl_shiptech_hull[SHIP_HULL_LARGE].cost) * mult) / 1500;
}

static uint16_t get_base_cost_mod_weap(const struct game_s *g, int tech_i, int percent)
{
    uint16_t mult;
    if (percent < 50) {
        mult = tech_reduce_50percent_per_10pts[percent] * 9;
    } else {
        mult = 3 * 9;
    }
    return (tbl_shiptech_weap[tech_i].cost * mult) / 1000;
}

static uint16_t get_base_cost_mod_shield(const struct game_s *g, int tech_i, int percent)
{
    uint16_t mult;
    if (percent < 50) {
        mult = tech_reduce_50percent_per_10pts[percent];
    } else {
        mult = 3;
    }
    return (tbl_shiptech_shield[tech_i].cost[SHIP_HULL_LARGE] * mult) / 1000 + tbl_shiptech_shield[tech_i].power[SHIP_HULL_LARGE] / 10;
}

static uint16_t get_base_cost_mod_comp(const struct game_s *g, int tech_i, int percent)
{
    uint16_t mult;
    if (percent < 50) {
        mult = tech_reduce_50percent_per_10pts[percent];
    } else {
        mult = 3;
    }
    return (tbl_shiptech_comp[tech_i].cost[SHIP_HULL_LARGE] * mult) / 1000 + tbl_shiptech_comp[tech_i].power[SHIP_HULL_LARGE] / 10;
}

static uint16_t get_base_cost_mod_jammer(const struct game_s *g, int player_i, int percent)
{
    uint16_t tech_i = 0;
    uint8_t mult;
    for (int i = 0; i < SHIP_JAMMER_NUM; ++i) {
        if (game_tech_player_has_tech(g, TECH_FIELD_COMPUTER, tbl_shiptech_jammer[i].tech_i, player_i)) {
            tech_i = i;
        }
    }
    if (percent < 50) {
        mult = tech_reduce_50percent_per_10pts[percent];
    } else {
        mult = 3;
    }
    return (tbl_shiptech_jammer[tech_i].cost[SHIP_HULL_LARGE] * mult) / 1000 + tbl_shiptech_jammer[tech_i].power[SHIP_HULL_LARGE] / 10;
}

static uint8_t find_best_tech_type(BOOLVEC_PTRPARAMI(tbl), int base, int step, int last)
{
    int best = 0;
    for (int i = base; i <= last; i += step) {
        if (BOOLVEC_IS1(tbl, i)) {
            best = i;
        }
    }
    return (uint8_t)best;
}

static void game_tech_add_newtech(struct game_s *g, player_id_t player, tech_field_t field, uint8_t tech, techsource_t source, int a8, player_id_t stolen_from, bool frame)
{
    newtechs_t *nts = &(g->evn.newtech[player]);
    int num = nts->num;
    if (num < NEWTECH_MAX) {
        newtech_t *nt = &(nts->d[num]);
        nt->field = field;
        nt->tech = tech;
        nt->source = source;
        nt->v06 = a8;
        nt->stolen_from = stolen_from;
        nt->frame = frame;
        nts->num = num + 1;
    }
}

static void game_tech_ai_tech_next(struct game_s *g, player_id_t player, tech_field_t field)
{
    uint8_t tbl[TECH_NEXT_MAX];
    int num = game_tech_get_next_techs(g, player, field, tbl);
    if (num != 0) {
        uint8_t tech = game_ai->tech_next(g, player, field, tbl, num);
        game_tech_start_next(g, player, field, tech);
    }
}

/* -------------------------------------------------------------------------- */

uint8_t game_tech_player_has_tech(const struct game_s *g, int field_i, int tech_i, int player_i)
{
    const uint8_t *p = g->srd[player_i].researchcompleted[field_i];
    uint32_t len = g->eto[player_i].tech.completed[field_i];
    return find_byte_in_tbl(tech_i, p, len);
}

uint8_t game_tech_player_best_tech(const struct game_s *g, int field_i, int tech_i_base, int tech_i_step, int tech_i_max, int player_i)
{
    uint8_t tech_best = 0;
    for (int tech_i = (tech_i_base >= 2) ? tech_i_base : tech_i_step; tech_i < tech_i_max; tech_i += tech_i_step) {
        if (game_tech_player_has_tech(g, field_i, tech_i, player_i)) {
            tech_best = tech_i;
        }
    }
    return tech_best;
}

uint8_t game_tech_player_best_engine(const struct game_s *g, int player_i)
{
    return game_tech_player_best_tech(g, TECH_FIELD_PROPULSION, 0, 6, 50, player_i) + 3;
}

uint16_t game_get_base_cost(const struct game_s *g, int player_i)
{
    const uint8_t *p = g->eto[player_i].tech.percent;
    const empiretechorbit_t *e = &(g->eto[player_i]);
    uint16_t cost;
    cost = get_base_cost_mod_armor(g, player_i, p[TECH_FIELD_CONSTRUCTION]);
    cost += get_base_cost_mod_weap(g, e->base_weapon, p[TECH_FIELD_WEAPON]);
    cost += get_base_cost_mod_shield(g, e->base_shield, p[TECH_FIELD_FORCE_FIELD]);
    cost += get_base_cost_mod_comp(g, e->base_comp, p[TECH_FIELD_COMPUTER]);
    cost += get_base_cost_mod_jammer(g, player_i, p[TECH_FIELD_COMPUTER]);
    cost = (cost * 3) / 5;
    if (BOOLVEC_IS1(g->is_ai, player_i)) {
        switch (g->difficulty) {
            case 1:
                cost = (cost * 9) / 10;
                break;
            case 2:
                cost = (cost * 8) / 10;
                break;
            case 3:
                cost = (cost * 7) / 10;
                break;
            case 4:
                cost /= 2;
                break;
            default:
                break;
        }
    }
    if (cost < 50) {
        cost = 50;
    }
    return cost;
}

uint8_t game_get_base_weapon(const struct game_s *g, player_id_t player_i, int tech_i)
{
    uint8_t r = WEAPON_LASER;   /* BUG? */
    for (int i = 0; i < WEAPON_NUM; ++i) {
        const struct shiptech_weap_s *w = &(tbl_shiptech_weap[i]);
        if (1
          && (w->tech_i <= tech_i)
          && (w->damagemin == w->damagemax)
          && (w->numshots == 2)
          && (w->misstype == 0)
          && (!w->is_bio)
          && (w->nummiss == 1)
          && game_tech_player_has_tech(g, TECH_FIELD_WEAPON, w->tech_i, player_i)
        ) {
            r = i;
        }
    }
    return r;
}

uint8_t game_get_base_weapon_2(const struct game_s *g, player_id_t player_i, int tech_i, uint8_t r)
{
    for (int i = 0; i < WEAPON_PLASMA_TORPEDO; ++i) {
        const struct shiptech_weap_s *w = &(tbl_shiptech_weap[i]);
        if (1
          && (w->nummiss > 1)
          && game_tech_player_has_tech(g, TECH_FIELD_WEAPON, w->tech_i, player_i)
        ) {
            r = i;
        }
    }
    return r;
}

uint8_t game_get_best_shield(struct game_s *g, player_id_t player_i, int tech_i)
{
    uint8_t r = 0;
    for (int i = 0; i < SHIP_SHIELD_NUM; ++i) {
        const struct shiptech_shield_s *w = &(tbl_shiptech_shield[i]);
        if (1
          && (w->tech_i <= tech_i)
          && game_tech_player_has_tech(g, TECH_FIELD_FORCE_FIELD, w->tech_i, player_i)
        ) {
            r = i;
        }
    }
    return r;
}

uint8_t game_get_best_comp(struct game_s *g, player_id_t player_i, int tech_i)
{
    uint8_t r = 0;
    for (int i = 0; i < SHIP_COMP_NUM; ++i) {
        const struct shiptech_comp_s *w = &(tbl_shiptech_comp[i]);
        if (1
          && (w->tech_i <= tech_i)
          && game_tech_player_has_tech(g, TECH_FIELD_COMPUTER, w->tech_i, player_i)
        ) {
            r = i;
        }
    }
    return r;
}

uint8_t game_get_best_jammer(const struct game_s *g, player_id_t player_i, int tech_i)
{
    uint8_t r = 0;
    for (int i = 0; i < SHIP_JAMMER_NUM; ++i) {
        const struct shiptech_jammer_s *w = &(tbl_shiptech_jammer[i]);
        if (1
          && (w->tech_i <= tech_i)
          && game_tech_player_has_tech(g, TECH_FIELD_COMPUTER, w->tech_i, player_i)
        ) {
            r = i;
        }
    }
    return r;
}

void game_update_tech_util(struct game_s *g)
{
    BOOLVEC_TBL_DECLARE(tbl_techcompl, TECH_FIELD_NUM, 0x96);
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        empiretechorbit_t *e = &(g->eto[pi]);
        uint8_t b, tech_i;
        BOOLVEC_TBL_CLEAR(tbl_techcompl, TECH_FIELD_NUM, 0x96); /* NOTE: MOO1 clears 41h */
        for (tech_field_t field_i = TECH_FIELD_COMPUTER; field_i < TECH_FIELD_NUM; ++field_i) {
            uint8_t *p = g->srd[pi].researchcompleted[field_i];
            uint32_t len = e->tech.completed[field_i];
            while (len--) {
                tech_i = *p++;
                BOOLVEC_TBL_SET1(tbl_techcompl, field_i, tech_i);
            }
        }
        e->have_colony_for = PLANET_TYPE_MINIMAL;
        for (int i = 0; i < 6; ++i) {
            if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, i * 3 + 3)) {
                e->have_colony_for = PLANET_TYPE_BARREN - i;
            }
        }
        if (e->race == RACE_SILICOID) {
            e->have_colony_for = PLANET_TYPE_RADIATED;
        }
        e->have_adv_soil_enrich = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 30);
        e->have_atmos_terra = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 22);
        e->have_soil_enrich = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 16);
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 42)) {
            e->inc_pop_cost = 5;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 21)) {
            e->inc_pop_cost = 10;
        } else {
            e->inc_pop_cost = 20;
        }
        if (e->race == RACE_SAKKRA) {
            e->inc_pop_cost = (e->inc_pop_cost * 2) / 3;
        }
        tech_i = find_best_tech_type(BOOLVEC_TBL_PTRPARAMM(tbl_techcompl, TECH_FIELD_PLANETOLOGY), 2, 6, 50);
        b = (tech_i > 0) ? (((tech_i - 2) / 6 + 1) * 10) : 0;
        if (b == 90) {
            b = 120;
        } else if (b == 80) {
            b = 100;
        } else if (b == 70) {
            b = 80;
        }
        e->have_terraform_n = b;
        b = (tech_i > 0) ? (5 - ((tech_i - 2) / 12)) : 5;
        if (b < 2) {
            b = 2;
        }
        e->terraform_cost_per_inc = b;
        e->have_combat_transporter = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 45);
        b = 2;
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 5)) {
            b = 3;
        }
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 13)) {
            b = 5;
        }
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 24)) {
            b = 10;
        }
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 34)) {
            b = 20;
        }
        e->have_eco_restoration_n = b;
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_COMPUTER, 23)) {
            b = 9;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_COMPUTER, 13)) {
            b = 7;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_COMPUTER, 4)) {
            b = 5;
        } else {
            b = 3;
        }
        e->scanner_range = b;
        e->have_stargates = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 27);
        e->have_hyperspace_comm = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_COMPUTER, 34);
        e->have_ia_scanner = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_COMPUTER, 13) || BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_COMPUTER, 23);
        e->have_adv_scanner = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_COMPUTER, 23);
        tech_i = find_best_tech_type(BOOLVEC_TBL_PTRPARAMM(tbl_techcompl, TECH_FIELD_COMPUTER), 8, 10, 50);
        b = (tech_i > 0) ? ((tech_i + 12) / 10 + 1) : 2;
        if (e->race == RACE_MEKLAR) {
            b += 2;
        }
        e->colonist_oper_factories = b;
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 38)) {
            b = 2;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 33)) {
            b = 3;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 28)) {
            b = 4;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 23)) {
            b = 5;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 18)) {
            b = 6;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 13)) {
            b = 7;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 8)) {
            b = 8;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 3)) {
            b = 9;
        } else {
            b = 10;
        }
        e->factory_cost = b;
        if (e->race != RACE_MEKLAR) {
            b = (b * e->colonist_oper_factories) / 2;
        }
        e->factory_adj_cost = b;
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 45)) {
            b = 0;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 35)) {
            b = 2;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 25)) {
            b = 4;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 15)) {
            b = 6;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_CONSTRUCTION, 5)) {
            b = 8;
        } else {
            b = 10;
        }
        e->ind_waste_scale = b;
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 41)) {
            b = 30;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 29)) {
            b = 10;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 23)) {
            b = 9;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 19)) {
            b = 8;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 14)) {
            b = 7;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 9)) {
            b = 6;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 5)) {
            b = 5;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 3)) {
            b = 4;
        } else {
            b = 3;
        }
        e->fuel_range = b;
        tech_i = find_best_tech_type(BOOLVEC_TBL_PTRPARAMM(tbl_techcompl, TECH_FIELD_FORCE_FIELD), 2, 10, 50);
        if (tech_i < 10) {
            tech_i = 0;
        }
        b = (tech_i > 0) ? (5 * ((tech_i - 2) / 10)) : 0;
        e->have_planet_shield = b;
        e->planet_shield_cost = game_num_pshield_cost[b / 5];
        b = 1;
        for (int i = 6; i < 50; i += 6) {
            if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, i)) {
                b = (i - 6) / 6 + 1;
            }
        }
        e->have_engine = b;
        e->have_sub_space_int = BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PROPULSION, 43);
        if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 36)) {
            b = 2;
        } else if (BOOLVEC_TBL_IS1(tbl_techcompl, TECH_FIELD_PLANETOLOGY, 17)) {
            b = 1;
        } else {
            b = 0;
        }
        e->antidote = b;
        if (g->gaux->flag_cheat_galaxy) {
            e->scanner_range = 30;
        }
    }
}

const char *game_tech_get_name(const struct game_aux_s *gaux, tech_field_t field, int tech, char *buf)
{
    if (tech == 0) {
        sprintf(buf, "%s %s", game_str_tbl_te_field[field], game_str_te_techno);
    } else if (tech == -2) {
        strcpy(buf, game_str_tbl_st_weap[WEAPON_NUCLEAR_MISSILE_2 - 1]);
    } else if (tech == -1) {
        strcpy(buf, game_str_tbl_st_weap[WEAPON_NUCLEAR_BOMB - 1]);
    } else if (tech > 50) {
        sprintf(buf, "%s %s %s %s", game_str_te_adv, game_str_tbl_te_field[field], game_str_te_tech, game_str_tbl_roman[(tech - 50) / 5]);
    } else {
        const uint8_t *p = RESEARCH_D0_PTR(gaux, field, tech);
        if (p[0] != 0xff) {
            int offs = GET_LE_16(&p[4]);
            strcpy(buf, &(gaux->research.names[offs]));
        } else {
            buf[0] = '\0';
        }
    }
    return buf;
}

const char *game_tech_get_descr(const struct game_aux_s *gaux, tech_field_t field, int tech, char *buf)
{
    if (tech == 0) {
        buf[0] = '\0';
    } else if (tech > 50) {
        sprintf(buf, "%s%s %s.", game_str_te_genimp, game_str_tbl_te_field[field], game_str_te_techno2);
    } else if (tech == -2) {
        strcpy(buf, game_str_te_nmis);
    } else if (tech == -1) {
        strcpy(buf, game_str_te_nbomb);
    } else {
        strcpy(buf, &(gaux->research.descr[(field * 50 + tech - 1) * RESEARCH_DESCR_LEN]));
    }
    return buf;
}

int game_tech_current_research_percent1(struct empiretechorbit_s *e, tech_field_t field)
{
    uint32_t invest, cost;
    int slider, t1, t3, percent;
    cost = e->tech.cost[field];
    slider = e->tech.slider[field];
    if ((cost == 0) || (slider == 0)) {
        return 0;
    }
    invest = e->tech.investment[field];
    t1 = (invest * 3) / 20;
    t3 = (slider * e->total_research_bc) / 100;
    SETMIN(t1, t3 * 2);
    invest += t3 + t1;
    percent = (invest * 100) / cost;
    return percent;
}

int game_tech_current_research_percent2(struct empiretechorbit_s *e, tech_field_t field)
{
    uint32_t invest, cost;
    int slider, t1, t3;
    cost = e->tech.cost[field];
    slider = e->tech.slider[field];
    if ((cost == 0) || (slider == 0)) {
        return 0;
    }
    invest = e->tech.investment[field];
    t1 = (invest * 3) / 20;
    t3 = (slider * e->total_research_bc) / 100;
    SETMIN(t1, t3 * 2);
    invest += t3 + t1;
    if (invest <= cost) {
        return 0;
    } else {
        int percent = (((invest - cost) * 50) / cost) / 2;
        SETRANGE(percent, 0, 99);
        return percent;
    }
}

void game_tech_get_new(struct game_s *g, player_id_t player, tech_field_t field, uint8_t tech, techsource_t source, int a8, player_id_t stolen_from, bool flag_frame)
{
    empiretechorbit_t *e = &(g->eto[player]);
    shipresearch_t *srd = &(g->srd[player]);
    uint8_t *rc = srd->researchcompleted[field];
    /*di*/int tc = e->tech.completed[field];
    bool have_tech = false;
    for (int i = 0; i < tc; ++i) {
        if (rc[i] == tech) {
            have_tech = true;
            break;
        }
    }
    if (have_tech || (tech == 0)) {
        /*6365a*/
        if (e->tech.project[field] == tech) {
            if (IS_HUMAN(g, player)) {
                e->tech.project[field] = 0;
                if (source == 4) {
                    game_tech_add_newtech(g, player, field, tech, source, a8, stolen_from, flag_frame);
                }
            } else {
                /*6374b*/
                game_tech_ai_tech_next(g, player, field);
            }
        }
    } else {
        /*6375b*/
        rc[tc++] = tech;
        e->tech.completed[field] = tc;
        for (int loops = 0; loops < tc; ++loops) {
            for (int i = 0; i < (tc - 1); ++i) {
                uint8_t t0, t1;
                t0 = rc[i];
                t1 = rc[i + 1];
                if (t0 > t1) {
                    rc[i + 1] = t0;
                    rc[i] = t1;
                }
            }
        }
        if (IS_HUMAN(g, player)) {
            game_tech_add_newtech(g, player, field, tech, source, a8, stolen_from, flag_frame);
        }
        /*63899*/
        if (e->tech.project[field] == tech) {
            e->tech.project[field] = 0;
            e->tech.investment[field] = 1;
        }
        /*63967*/
        if ((e->tech.project[field] == 0) && IS_AI(g, player)) {
            game_tech_ai_tech_next(g, player, field);
        }
    }
}

int game_tech_get_next_techs(const struct game_s *g, player_id_t player, tech_field_t field, uint8_t *tbl)
{
    const uint8_t *rc = g->srd[player].researchcompleted[field];
    int num, maxtier, len = g->eto[player].tech.completed[field];
    const uint8_t (*rl)[3] = g->srd[player].researchlist[field];
    uint8_t tmax = 0;
    for (int i = 0; i < len; ++i) {
        uint8_t t;
        t = rc[i];
        SETMAX(tmax, t);
    }
    if (len == 1) {
        maxtier = 1;
    } else {
        maxtier = (tmax - 1) / 5 + 2;
        SETMIN(maxtier, 10);
    }
    num = 0;
    for (int tier = 0; tier < maxtier; ++tier) {
        for (int l = 0; l < 3; ++l) {
            uint8_t t;
            t = rl[tier][l];
            if (t != 0) {
                bool have;
                have = false;
                for (int i = 0; i < len; ++i) {
                    if (rc[i] == t) {
                        have = true;
                        break;
                    }
                }
                if ((!have) && (num < TECH_NEXT_MAX)) {
                    tbl[num++] = t;
                }
            }
        }
    }
    if ((num == 0) && (tmax < 100)) {
        tbl[num++] = (tmax <= 50) ? 55 : (tmax + 5);
    }
    return num;
}

uint32_t game_tech_get_next_rp(const struct game_s *g, player_id_t player, tech_field_t field, uint8_t tech)
{
    uint32_t cost;
    cost = tech * tech;
    cost *= game_num_tech_costmulr[g->eto[player].race][field];
    if (IS_AI(g, player)) {
        cost *= game_num_tech_costmuld[0];
        cost /= 100;
    } else {
        cost *= game_num_tech_costmuld[g->difficulty];
        cost /= 1000;
        cost *= 10;
    }
    return cost;
}

void game_tech_start_next(struct game_s *g, player_id_t player, tech_field_t field, uint8_t tech)
{
    techdata_t *td = &(g->eto[player].tech);
    if (td->project[field] != 0) {
        td->investment[field] = 0;
    }
    td->project[field] = tech;
    td->cost[field] = game_tech_get_next_rp(g, player, field, tech);
}

int game_tech_get_field_percent(const struct game_s *g, player_id_t player, tech_field_t field)
{
    const uint8_t *rc = g->srd[player].researchcompleted[field];
    int v, len = g->eto[player].tech.completed[field];
    uint8_t tmax = 0;
    for (int i = 0; i < len; ++i) {
        uint8_t t;
        t = rc[i];
        SETMAX(tmax, t);
    }
    if (len == 1) {
        v = 1;
    } else {
        v = (tmax - 1) / 5 + 2;
    }
    v = tmax + len - v;
    SETRANGE(v, 1, 99);
    return v;
}

void game_tech_research(struct game_s *g)
{
    for (player_id_t player = PLAYER_0; player < g->players; ++player) {
        empiretechorbit_t *e = &(g->eto[player]);
        techdata_t *td = &(e->tech);
        uint32_t total_research;
        total_research = e->total_research_bc;
        for (tech_field_t field = 0; field < TECH_FIELD_NUM; ++field) {
            int slider, t1, t3;
            uint32_t invest, cost;
            invest = td->investment[field];
            slider = td->slider[field];
            t1 = (invest * 3) / 20;
            t3 = (slider * total_research) / 100;
            SETMIN(t1, t3 * 2);
            invest += t3 + t1;
            td->investment[field] = invest;
            td->percent[field] = game_tech_get_field_percent(g, player, field);
            cost = td->cost[field];
            if ((cost != 0) && (slider != 0) && (total_research != 0)) {
                if (cost < invest) {
                    int v;
                    v = ((invest - cost) * 250) / cost;
                    if (rnd_1_n(500, &g->seed) <= v) {
                        game_tech_get_new(g, player, field, td->project[field], TECHSOURCE_RESEARCH, game_planet_get_random(g, player), 0, false);
                    }
                }
            } else {
                td->investment[field] = (invest * 9) / 10;
            }
        }
    }
    game_update_tech_util(g);
}

void game_tech_get_orion_loot(struct game_s *g, player_id_t player)
{
    empiretechorbit_t *e = &(g->eto[player]);
    techdata_t *td = &(e->tech);
    uint8_t percent[TECH_FIELD_NUM];
    game_tech_get_new(g, player, TECH_FIELD_WEAPON, TECH_WEAP_DEATH_RAY, TECHSOURCE_FOUND, -PLANETS_MAX, 0, false);    /* HACK */
    for (tech_field_t f = 0; f < TECH_FIELD_NUM; ++f) {
        percent[f] = MIN(td->percent[f] + 25, 50);
    }
    for (int n = 0; n < 3; ++n) {
        for (int loops = 0; loops < 200; ++loops) {
            tech_field_t field;
            uint8_t tech;
            const uint8_t *p;
            field = rnd_0_nm1(TECH_FIELD_NUM, &g->seed);
            tech = rnd_1_n(31, &g->seed) + 19;
            p = RESEARCH_D0_PTR(g->gaux, field, tech);
            if ((percent[field] >= tech) && (p[0] != 0xff)) {
                const uint8_t *rc;
                bool have_tech;
                rc = &(g->srd[player].researchcompleted[field][0]);
                have_tech = false;
                for (int i = 0; i < td->completed[field]; ++i) {
                    if (rc[i] == tech) {
                        have_tech = true;
                        break;
                    }
                }
                if (!have_tech) {
                    game_tech_get_new(g, player, field, tech, TECHSOURCE_FOUND, NEWTECH_V06_ORION, 0, false);
                    break;
                }
            }
        }
    }
}

void game_tech_get_artifact_loot(struct game_s *g, uint8_t planet, player_id_t player)
{
    empiretechorbit_t *e = &(g->eto[player]);
    techdata_t *td = &(e->tech);
    uint8_t percent[TECH_FIELD_NUM];
    if (IS_AI(g, player)) {
        return;
    }
    for (tech_field_t f = 0; f < TECH_FIELD_NUM; ++f) {
        percent[f] = MIN(td->percent[f] + 10, 50);
    }
    for (int n = 0; n < 1; ++n) {
        for (int loops = 0; loops < 200; ++loops) {
            tech_field_t field;
            uint8_t tech;
            const uint8_t *p;
            field = rnd_0_nm1(TECH_FIELD_NUM, &g->seed);
            tech = rnd_1_n(30, &g->seed);
            p = RESEARCH_D0_PTR(g->gaux, field, tech);
            if ((percent[field] >= tech) && (p[0] != 0xff)) {
                const uint8_t *rc;
                bool have_tech;
                rc = &(g->srd[player].researchcompleted[field][0]);
                have_tech = false;
                for (int i = 0; i < td->completed[field]; ++i) {
                    if (rc[i] == tech) {
                        have_tech = true;
                        break;
                    }
                }
                if (!have_tech) {
                    game_tech_get_new(g, player, field, tech, TECHSOURCE_FOUND, -(planet + 1), 0, false);  /* HACK */
                    break;
                }
            }
        }
    }
}

void game_tech_final_war_share(struct game_s *g)
{
    for (tech_field_t field_i = TECH_FIELD_COMPUTER; field_i < TECH_FIELD_NUM; ++field_i) {
        BOOLVEC_DECLARE(tbl_techcompl, 0x96);
        BOOLVEC_CLEAR(tbl_techcompl, 0x96); /* NOTE: MOO1 clears 60 */
        for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
            if (IS_AI(g, pi)) {
                uint8_t *p = g->srd[pi].researchcompleted[field_i];
                uint32_t len = g->eto[pi].tech.completed[field_i];
                while (len--) {
                    uint8_t tech_i;
                    tech_i = *p++;
                    BOOLVEC_SET1(tbl_techcompl, tech_i);
                }
            }
        }
        for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
            if (IS_AI(g, pi)) {
                uint8_t *p = g->srd[pi].researchcompleted[field_i];
                int n;
                n = 0;
                for (uint8_t tech_i = 0; tech_i < 60; ++tech_i) {
                    if (BOOLVEC_IS1(tbl_techcompl, tech_i)) {
                        p[n++] = tech_i;
                    }
                }
                g->eto[pi].tech.completed[field_i] = n;
            }
        }
    }
}
