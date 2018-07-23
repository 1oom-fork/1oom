#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "uifleet.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_num.h"
#include "game_str.h"
#include "uidefs.h"
#include "uiinput.h"
#include "uiplanet.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static uint8_t get_fleet_planet_on(const struct game_s *g, const fleet_enroute_t *r)
{
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const planet_t *p;
        p = &g->planet[i];
        if ((p->x == r->x) && (p->y == r->y)) {
            return i;
        }
    }
    return PLANET_NONE;
}

static void print_fleet_enroute(const struct game_s *g, int api, const fleet_enroute_t *r, uint8_t pon)
{
    char buf[80];
    char pname[20];
    const empiretechorbit_t *e = &(g->eto[api]);
    int eta, sd_num = e->shipdesigns_num;
    const planet_t *p = &(g->planet[r->dest]);
    sprintf(buf, "#F%i (%i,%i) %s [%i] %s", (int)(r - g->enroute), r->x, r->y, game_str_fl_moving, r->dest, ui_planet_str(g, api, r->dest, pname));
    printf("%-40s", buf);
    for (int k = 0; k < sd_num; ++k) {
        printf(" %5i", r->ships[k]);
    }
    if (pon == PLANET_NONE) {
        pon = get_fleet_planet_on(g, r);
    }
    eta = game_calc_eta(g, game_fleet_get_speed(g, r, pon, r->dest), p->x, p->y, r->x, r->y);
    printf("  %s %i %s\n", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
}

static void print_transport_enroute(const struct game_s *g, int api, const transport_t *r)
{
    char buf[80];
    char pname[20];
    const empiretechorbit_t *e = &(g->eto[api]);
    int eta;
    const planet_t *p = &(g->planet[r->dest]);
    sprintf(buf, "#T%i (%i,%i) %s [%i] %s", (int)(r - g->transport), r->x, r->y, game_str_fl_moving, r->dest, ui_planet_str(g, api, r->dest, pname));
    printf("%-40s", buf);
    printf(" %5i", r->pop);
    eta = game_calc_eta(g, e->have_engine, p->x, p->y, r->x, r->y);
    printf("  %s %i %s\n", game_str_sm_eta, eta, (eta == 1) ? game_str_sm_turn : game_str_sm_turns);
}

/* -------------------------------------------------------------------------- */

int ui_cmd_fleet_list(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    char buf[80];
    const empiretechorbit_t *e = &(g->eto[api]);
    int sd_num = e->shipdesigns_num;
    {
        const shipdesign_t *sd = &(g->srd[api].design[0]);
        for (int j = 0; j < sd_num; ++j) {
            printf("%c %s", (j == 0) ? '-' : ',', sd[j].name);
        }
        putchar('\n');
    }
    for (int i = 0; i < g->galaxy_stars; ++i) {
        const fleet_orbit_t *r = &(e->orbit[i]);
        for (int j = 0; j < sd_num; ++j) {
            if (r->ships[j] != 0) {
                const planet_t *p;
                p = &(g->planet[i]);
                sprintf(buf, "%s [%i] (%i,%i) %s", game_str_fl_inorbit, i, p->x, p->y, p->name);
                printf("%-40s", buf);
                for (int k = 0; k < sd_num; ++k) {
                    printf(" %5i", r->ships[k]);
                }
                putchar('\n');
                break;
            }
        }
    }
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        if (r->owner == api) {
            for (int j = 0; j < sd_num; ++j) {
                if (r->ships[j] != 0) {
                    print_fleet_enroute(g, api, r, PLANET_NONE);
                    break;
                }
            }
        }
    }
    return 0;
}

int ui_cmd_fleet_redir(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    const empiretechorbit_t *e = &(g->eto[api]);
    int sd_num = e->shipdesigns_num;
    if (num_param == 0) {
        for (int i = 0; i < g->enroute_num; ++i) {
            const fleet_enroute_t *r = &(g->enroute[i]);
            if (r->owner == api) {
                for (int j = 0; j < sd_num; ++j) {
                    if (r->ships[j] != 0) {
                        uint8_t pon;
                        pon = get_fleet_planet_on(g, r);
                        if (e->have_hyperspace_comm || (pon != PLANET_NONE)) {
                            print_fleet_enroute(g, api, r, pon);
                        }
                        break;
                    }
                }
            }
        }
        if (e->have_hyperspace_comm) {
            for (int i = 0; i < g->transport_num; ++i) {
                const transport_t *r = &(g->transport[i]);
                if (r->owner == api) {
                    print_transport_enroute(g, api, r);
                }
            }
        }
    } else if (num_param == 1) {
        return -1;
    } else {
        bool is_fleet;
        int dist, n = -1;
        uint8_t dest;
        const planet_t *p;
        if (param->str[0] != '#') {
            return -1;
        }
        if (tolower(param->str[1]) == 'f') {
            is_fleet = true;
        } else if (tolower(param->str[1]) == 't') {
            is_fleet = false;
        } else {
            return -1;
        }
        if (0
          || (!util_parse_signed_number(&param->str[2], &n))
          || (n < 0)
          || (is_fleet && (n >= g->enroute_num))
          || ((!is_fleet) && (n >= g->transport_num))
        ) {
            return -1;
        }
        dest = ui_planet_from_param(g, api, &param[1]);
        if (dest == PLANET_NONE) {
            puts("Unknown destination");
            return -1;
        }
        p = &(g->planet[dest]);
        dist = game_get_min_dist(g, api, dest);
        if (is_fleet) {
            fleet_enroute_t *r = &(g->enroute[n]);
            uint8_t pon;
            bool in_range;
            if (r->owner != api) {
                return -1;
            }
            pon = get_fleet_planet_on(g, r);
            if ((!e->have_hyperspace_comm) && (pon == PLANET_NONE)) {
                return -1;
            }
            in_range = false;
            if (p->within_frange[api] == 1) {
                in_range = true;
            } else if (p->within_frange[api] == 2) {
                in_range = true;
                for (int k = 0; k < e->shipdesigns_num; ++k) {
                    if (r->ships[k] && (!g->srd[api].have_reserve_fuel)) {
                        in_range = false;
                        break;
                    }
                }
            }
            if (!in_range) {
                printf("%s %i %s\n", game_str_sm_outsr, dist - e->fuel_range, game_str_sm_parsecs2);
                return -1;
            }
            game_fleet_redirect(g, r, pon, dest);
        } else {
            transport_t *r = &(g->transport[n]);
            if (0
              || (r->owner != api)
              || (!e->have_hyperspace_comm)
            ) {
                return -1;
            }
            if (p->within_frange[api] != 1) {
                printf("%s %i %s\n", game_str_sm_outsr, dist - e->fuel_range, game_str_sm_parsecs2);
                return -1;
            }
            if (!game_transport_dest_ok(g, p, api)) {
                puts("Destination not OK");
                return -1;
            }
            r->dest = dest;
        }
    }
    return 0;
}

int ui_cmd_fleet_send(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    uint8_t pti, pfi = g->planet_focus_i[api];
    empiretechorbit_t *e = &(g->eto[api]);
    fleet_orbit_t *o = &(e->orbit[pfi]);
    const planet_t *pt;
    shipcount_t ships[NUM_SHIPDESIGNS];
    pti = ui_planet_from_param(g, api, param);
    if (pti == PLANET_NONE) {
        puts("Unknown destination");
        return -1;
    }
    if (pti == pfi) {
        puts("Not sending fleet to planet it is orbiting");
        return -1;
    }
    pt = &(g->planet[pti]);
    {
        bool any_ships = false;
        int i;
        for (i = 0; i < e->shipdesigns_num; ++i) {
            ships[i] = o->ships[i];
            if (ships[i] != 0) {
                any_ships = true;
            }
        }
        for (; i < NUM_SHIPDESIGNS; ++i) {
            ships[i] = 0;
        }
        if (!any_ships) {
            puts("No ships on orbit");
            return -1;
        }
    }
    if (num_param > 1) {
        int i;
        for (i = 0; (i < e->shipdesigns_num) && (i < (num_param - 1)); ++i) {
            shipcount_t n;
            if (param[1 + i].type != INPUT_TOKEN_NUMBER) {
                printf("Invalid param %i\n", 1 + i);
                return -1;
            }
            n = param[1 + i].data.num;
            if (n > ships[i]) {
                printf("Trying to send more ships than you have (%i > %i)\n", n, ships[i]);
                return -1;
            }
            ships[i] = n;
        }
        for (; i < NUM_SHIPDESIGNS; ++i) {
            ships[i] = 0;
        }
    }
    {
        bool any_ships = false;
        for (int i = 0; i < e->shipdesigns_num; ++i) {
            if (ships[i] != 0) {
                any_ships = true;
                break;
            }
        }
        if (!any_ships) {
            puts("Not sending empty fleet");
            return -1;
        }
    }
    {
        bool have_reserve_fuel = true;
        bool *hrf = &(g->srd[api].have_reserve_fuel[0]);
        for (int i = 0; i < e->shipdesigns_num; ++i) {
            if (ships[i] && (!hrf[i])) {
                have_reserve_fuel = false;
                break;
            }
        }
        if (!((pt->within_frange[api] == 1) || ((pt->within_frange[api] == 2) && have_reserve_fuel))) {
            puts("Out of range");
            return -1;
        }
    }
    {
        const uint8_t shiptypes[NUM_SHIPDESIGNS] = { 0, 1, 2, 3, 4, 5 };
        game_send_fleet_from_orbit(g, api, pfi, pti, ships, shiptypes, NUM_SHIPDESIGNS);
    }
    return 0;
}

int ui_cmd_fleet_scrap(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    const empiretechorbit_t *e = &(g->eto[api]);
    const shipresearch_t *srd = &(g->srd[api]);
    int n = -1;
    if (e->shipdesigns_num <= 1) {
        return -1;
    }
    if (num_param == 0) {
        struct input_list_s rl_in[] = {
            { 0, "1", NULL, NULL },
            { 1, "2", NULL, NULL },
            { 2, "3", NULL, NULL },
            { 3, "4", NULL, NULL },
            { 4, "5", NULL, NULL },
            { 5, "6", NULL, NULL },
            { 0, NULL, NULL, NULL },
            { 0, NULL, NULL, NULL }
        };
        int i;
        for (i = 0; i < e->shipdesigns_num; ++i) {
            rl_in[i].display = srd->design[i].name;
        }
        rl_in[i].value = -1;
        rl_in[i].key = "Q";
        rl_in[i].str = "q";
        rl_in[i].display = "(quit)";
        ++i;
        rl_in[i].value = 0;
        rl_in[i].key = NULL;
        rl_in[i].str = NULL;
        rl_in[i].display = NULL;
        n = ui_input_list("Scrap", "> ", rl_in);
    } else {
        if (param->type == INPUT_TOKEN_NUMBER) {
            n = param->data.num;
            if ((n < 1) || (n > e->shipdesigns_num)) {
                return -1;
            }
        } else {
            for (int i = 0; i < e->shipdesigns_num; ++i) {
                if (strcasecmp(srd->design[i].name, param->str) == 0) {
                    n = i;
                    break;
                }
            }
            if (n == -1) {
                return -1;
            }
        }
    }
    if ((n >= 0) && (n < e->shipdesigns_num)) {
        game_design_scrap(g, api, n, var != NULL);
    }
    return 0;
}
