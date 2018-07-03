#include "config.h"

#include <stdio.h>
#include <string.h>

#include "uifleet.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_fleet.h"
#include "game_misc.h"
#include "game_str.h"
#include "uidefs.h"
#include "uiinput.h"
#include "uiplanet.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

int ui_cmd_fleet_send(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    uint8_t pti, pfi = g->planet_focus_i[api];
    empiretechorbit_t *e = &(g->eto[api]);
    fleet_orbit_t *o = &(e->orbit[pfi]);
    const planet_t *pt;
    shipcount_t ships[NUM_SHIPDESIGNS];
    pti = ui_planet_from_param(g, api, param);
    if (pti == PLANET_NONE) {
        printf("Unknown destination\n");
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
            printf("No ships on orbit\n");
            return -1;
        }
    }
    {
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
        for (; i < e->shipdesigns_num; ++i) {
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
            printf("Not sending empty fleet\n");
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
            printf("Out of range\n");
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
