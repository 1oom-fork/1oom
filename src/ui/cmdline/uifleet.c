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
