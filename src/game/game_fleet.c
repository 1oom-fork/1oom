#include "config.h"

#include <stdio.h>

#include "game_fleet.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_str.h"
#include "game_util.h"
#include "log.h"
#include "types.h"
#include "ui.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

static bool game_send_fleet_do(struct game_s *g, player_id_t owner, uint8_t from, uint8_t dest, const shipcount_t ships[NUM_SHIPDESIGNS], const uint8_t shiptypes[NUM_SHIPDESIGNS], uint8_t numtypes)
{
    fleet_enroute_t *r;
    const planet_t *pf, *pt;
    {
        bool found = false;
        for (int i = 0; i < numtypes; ++i) {
            if (ships[i] != 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    if (g->enroute_num == FLEET_ENROUTE_MAX) {
        log_warning("fleet enroute table (size %i) full!\n", FLEET_ENROUTE_MAX);
        return false;
    }
    pf = (&g->planet[from]);
    pt = (&g->planet[dest]);
    r = (&g->enroute[g->enroute_num]);
    r->owner = owner;
    r->x = pf->x;
    r->y = pf->y;
    r->dest = dest;
    for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
        r->ships[i] = 0;
    }
    {
        uint8_t speed = 100;
        shipdesign_t *sd = (&g->srd[owner].design[0]);
        for (int i = 0; i < numtypes; ++i) {
            if (ships[i] > 0) {
                uint8_t s, st;
                st = shiptypes[i];
                r->ships[st] = ships[i];
                s = sd[st].engine + 1;
                SETMIN(speed, s);
            }
        }
        if ((pt->owner == owner) && (pf->owner == owner) && pt->have_stargate && pf->have_stargate) {
            speed = FLEET_SPEED_STARGATE;
        }
        r->speed = speed;
    }
    BOOLVEC_CLEAR(r->visible, PLAYER_NUM);
    BOOLVEC_SET1(r->visible, owner);
    ++g->enroute_num;
    return true;
}

/* -------------------------------------------------------------------------- */

bool game_send_fleet_from_orbit(struct game_s *g, player_id_t owner, uint8_t from, uint8_t dest, const shipcount_t ships[NUM_SHIPDESIGNS], const uint8_t shiptypes[NUM_SHIPDESIGNS], uint8_t numtypes)
{
    if (!game_send_fleet_do(g, owner, from, dest, ships, shiptypes, numtypes)) {
        return false;
    }
    {
        fleet_orbit_t *o = &(g->eto[owner].orbit[from]);
        bool found = false;
        for (int i = 0; i < numtypes; ++i) {
            o->ships[shiptypes[i]] -= ships[i];
        }
        for (int i = 0; i < NUM_SHIPDESIGNS; ++i) {
            if (o->ships[i] > 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            BOOLVEC_CLEAR(o->visible, PLAYER_NUM);
        }
    }
    return true;
}

bool game_send_fleet_reloc(struct game_s *g, player_id_t owner, uint8_t from, uint8_t dest, uint8_t si, shipcount_t shipnum)
{
    shipcount_t ships[NUM_SHIPDESIGNS];
    uint8_t shiptypes[NUM_SHIPDESIGNS];
    ships[0] = shipnum;
    shiptypes[0] = si;
    return game_send_fleet_do(g, owner, from, dest, ships, shiptypes, 1);
}

bool game_send_transport(struct game_s *g, struct planet_s *pf)
{
    transport_t *r;
    const planet_t *pt;
    player_id_t owner;
    if (g->transport_num == TRANSPORT_MAX) {
        log_warning("transport table (size %i) full, could not send!\n", TRANSPORT_MAX);
        return false;
    }
    pt = &(g->planet[pf->trans_dest]);
    owner = pf->owner;
    r = (&g->transport[g->transport_num]);
    r->owner = owner;
    r->x = pf->x;
    r->y = pf->y;
    r->dest = pf->trans_dest;
    {
        uint8_t speed = g->eto[owner].have_engine;
        if ((pt->owner == owner) && pt->have_stargate && pf->have_stargate) {
            speed = FLEET_SPEED_STARGATE;
        }
        r->speed = speed;
    }
    {
        uint16_t n = pf->trans_num;
        r->pop = n;
        pf->pop -= n;
        SETMAX(pf->pop, 1);
        SUBSAT0(pf->rebels, n / 2 + 1);
    }
    pf->trans_num = 0;
    ++g->transport_num;
    return true;
}

void game_remove_empty_fleets(struct game_s *g)
{
    for (int i = 0; i < g->enroute_num; ++i) {
        fleet_enroute_t *r = &(g->enroute[i]);
        bool is_empty;
        is_empty = true;
        for (int j = 0; j < NUM_SHIPDESIGNS; ++j) {
            if (r->ships[j] != 0) {
                is_empty = false;
                break;
            }
        }
        if (is_empty) {
            util_table_remove_item_any_order(i, g->enroute, sizeof(fleet_enroute_t), g->enroute_num);
            --g->enroute_num;
            --i;
        }
    }
    for (int i = 0; i < g->transport_num; ++i) {
        transport_t *r = &(g->transport[i]);
        if (r->pop == 0) {
            util_table_remove_item_any_order(i, g->transport, sizeof(transport_t), g->transport_num);
            --g->transport_num;
            --i;
        }
    }
}

void game_remove_player_fleets(struct game_s *g, player_id_t owner)
{
    for (int i = 0; i < g->enroute_num; ++i) {
        fleet_enroute_t *r = &(g->enroute[i]);
        if (r->owner == owner) {
            util_table_remove_item_any_order(i, g->enroute, sizeof(fleet_enroute_t), g->enroute_num);
            --g->enroute_num;
            --i;
        }
    }
    for (int i = 0; i < g->transport_num; ++i) {
        transport_t *r = &(g->transport[i]);
        if (r->owner == owner) {
            util_table_remove_item_any_order(i, g->transport, sizeof(transport_t), g->transport_num);
            --g->transport_num;
            --i;
        }
    }
    memset(g->eto[owner].orbit, 0, sizeof(g->eto[owner].orbit));
}

bool game_fleet_any_dest_player(const struct game_s *g, player_id_t owner, player_id_t target)
{
    for (int i = 0; i < g->enroute_num; ++i) {
        const fleet_enroute_t *r = &(g->enroute[i]);
        if ((r->owner == owner) && (g->planet[r->dest].owner == target)) {
            return true;
        }
    }
    return false;
}

void game_fleet_unrefuel(struct game_s *g)
{
    for (player_id_t pi = PLAYER_0; pi < g->players; ++pi) {
        const shipresearch_t *srd = &(g->srd[pi]);
        empiretechorbit_t *e = &(g->eto[pi]);
        for (uint8_t pli = 0; pli < g->galaxy_stars; ++pli) {
            const planet_t *p = &(g->planet[pli]);
            uint8_t wr;
            wr = p->within_frange[pi];
            if (wr != 1) {
                fleet_orbit_t *r = &(e->orbit[pli]);
                bool unfueled;
                unfueled = false;
                for (int si = 0; si < e->shipdesigns_num; ++si) {
                    if (1
                      && (r->ships[si] != 0)
                      && ((wr == 0) || ((wr == 2) && (!srd->have_reserve_fuel[si])))
                    ) {
                        unfueled = true;
                        r->ships[si] = 0;
                    }
                }
                if (unfueled && IS_HUMAN(g, pi)) {
                    char *buf, *b, *q;
                    buf = b = ui_get_strbuf();
                    b += sprintf(buf, "%s ", game_str_tr_fuel1);
                    q = b;
                    b += sprintf(b, "%s", p->name);
                    util_str_tolower(q + 1);
                    sprintf(b, " %s", game_str_tr_fuel2);
                    g->planet_focus_i[pi] = pli;
                    ui_turn_msg(g, pi, buf);
                    /*temp_turn_hmm3 = 0;*/
                }
            }
        }
    }
}
