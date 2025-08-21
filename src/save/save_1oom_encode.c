/* NOTE!
   If the save format changes, increase LIBSAVE_1OOM_VERSION and implement a converter in 1oom_saveconv.
   The format is not to be changed without a very good reason.
*/
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "game_save.h"
#include "game.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "save.h"
#include "save_1oom_encode.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

static int libsave_1oom_encode_planet(uint8_t *buf, int pos, const planet_t *p, player_id_t pnum)
{
    SG_1OOM_EN_TBL_U8(p->name, PLANET_NAME_LEN);
    SG_1OOM_EN_U16(p->x);
    SG_1OOM_EN_U16(p->y);
    SG_1OOM_EN_U8(p->star_type);
    SG_1OOM_EN_U8(p->look);
    SG_1OOM_EN_U8(p->frame);
    SG_1OOM_EN_U8(p->rocks);
    SG_1OOM_EN_U16(p->max_pop1);
    SG_1OOM_EN_U16(p->max_pop2);
    SG_1OOM_EN_U16(p->max_pop3);
    SG_1OOM_EN_U8(p->type);
    SG_1OOM_EN_U8(p->battlebg);
    SG_1OOM_EN_U8(p->infogfx);
    SG_1OOM_EN_U8(p->growth);
    SG_1OOM_EN_U8(p->special);
    SG_1OOM_EN_U16(p->bc_to_ecoproj);
    SG_1OOM_EN_U16(p->bc_to_ship);
    SG_1OOM_EN_U16(p->bc_to_factory);
    SG_1OOM_EN_U32(p->reserve);
    SG_1OOM_EN_U16(p->waste);
    SG_1OOM_EN_PLAYER_ID(p->owner);
    SG_1OOM_EN_PLAYER_ID(p->prev_owner);
    SG_1OOM_EN_PLAYER_ID(p->claim);
    SG_1OOM_EN_U16(p->pop);
    SG_1OOM_EN_U16(p->pop_prev);
    SG_1OOM_EN_U16(p->factories);
    SG_1OOM_EN_TBL_U16(p->slider, PLANET_SLIDER_NUM);
    SG_1OOM_EN_TBL_U8(p->slider_lock, PLANET_SLIDER_NUM);
    SG_1OOM_EN_U8(p->buildship);
    SG_1OOM_EN_PLANET_ID(p->reloc);
    SG_1OOM_EN_U16(p->missile_bases);
    SG_1OOM_EN_U16(p->bc_to_base);
    SG_1OOM_EN_U16(p->bc_upgrade_base);
    SG_1OOM_EN_U8(p->have_stargate);
    SG_1OOM_EN_U8(p->shield);
    SG_1OOM_EN_U16(p->bc_to_shield);
    SG_1OOM_EN_U16(p->trans_num);
    SG_1OOM_EN_PLANET_ID(p->trans_dest);
    SG_1OOM_EN_U8(p->pop_tenths);
    SG_1OOM_EN_BV(p->explored, PLAYER_NUM);
    SG_1OOM_EN_DUMMY_V0(5);
    SG_1OOM_EN_U8(p->pop_oper_fact);
    SG_1OOM_EN_U16(p->bc_to_refit);
    SG_1OOM_EN_U16(p->rebels);
    SG_1OOM_EN_U8(p->unrest);
    SG_1OOM_EN_U8(p->unrest_reported);
    SG_1OOM_EN_BV(p->finished, FINISHED_NUM);
    SG_1OOM_EN_DUMMY_V0(5);
    return pos;
}

static int libsave_1oom_encode_enroute(uint8_t *buf, int pos, const fleet_enroute_t *r)
{
    SG_1OOM_EN_PLAYER_ID(r->owner);
    SG_1OOM_EN_U16(r->x);
    SG_1OOM_EN_U16(r->y);
    SG_1OOM_EN_PLANET_ID(r->dest);
    SG_1OOM_EN_U8(r->speed);
    SG_1OOM_EN_TBL_U16(r->ships, NUM_SHIPDESIGNS);
    return pos;
}

static int libsave_1oom_encode_transport(uint8_t *buf, int pos, const transport_t *r)
{
    SG_1OOM_EN_PLAYER_ID(r->owner);
    SG_1OOM_EN_U16(r->x);
    SG_1OOM_EN_U16(r->y);
    SG_1OOM_EN_PLANET_ID(r->dest);
    SG_1OOM_EN_U8(r->speed);
    SG_1OOM_EN_U16(r->pop);
    return pos;
}

static int libsave_1oom_encode_orbits(uint8_t *buf, int pos, const fleet_orbit_t *o, planet_id_t planets)
{
    for (planet_id_t i = PLANET_0; i < planets; ++i) {
        bool any_ships;
        any_ships = false;
        for (shipdesign_id_t j = SHIPDESIGN_0; j < NUM_SHIPDESIGNS; ++j) {
            if (o[i].ships[j] != 0) {
                any_ships = true;
                break;
            }
        }
        if (any_ships) {
            SG_1OOM_EN_PLANET_ID(i);
            SG_1OOM_EN_TBL_U16(o[i].ships, NUM_SHIPDESIGNS);
        }
    }
    SG_1OOM_EN_PLANET_ID(PLANET_NONE);
    return pos;
}

static int libsave_1oom_encode_eto(uint8_t *buf, int pos, const empiretechorbit_t *e, player_id_t pnum, planet_id_t planets)
{
    SG_1OOM_EN_U8(e->race);
    SG_1OOM_EN_U8(e->banner);
    SG_1OOM_EN_U8(e->trait1);
    SG_1OOM_EN_U8(e->trait2);
    SG_1OOM_EN_U8(e->ai_p3_countdown);
    SG_1OOM_EN_U8(e->ai_p2_countdown);
    SG_1OOM_EN_BV(e->contact, PLAYER_NUM);
    SG_1OOM_EN_DUMMY_V0(5);
    SG_1OOM_EN_TBL_U16(e->relation1, pnum);
    SG_1OOM_EN_TBL_U16(e->relation2, pnum);
    SG_1OOM_EN_TBL_U8(e->diplo_type, pnum);
    SG_1OOM_EN_TBL_U16(e->diplo_val, pnum);
    SG_1OOM_EN_TBL_U16(e->diplo_p1, pnum);
    SG_1OOM_EN_TBL_U16(e->diplo_p2, pnum);
    SG_1OOM_EN_TBL_U16(e->trust, pnum);
    SG_1OOM_EN_TBL_U8(e->broken_treaty, pnum);
    SG_1OOM_EN_TBL_U16(e->blunder, pnum);
    SG_1OOM_EN_TBL_U8(e->tribute_field, pnum);
    SG_1OOM_EN_TBL_U8(e->tribute_tech, pnum);
    SG_1OOM_EN_TBL_U16(e->mood_treaty, pnum);
    SG_1OOM_EN_TBL_U16(e->mood_trade, pnum);
    SG_1OOM_EN_TBL_U16(e->mood_tech, pnum);
    SG_1OOM_EN_TBL_U16(e->mood_peace, pnum);
    SG_1OOM_EN_TBL_U8(e->treaty, pnum);
    SG_1OOM_EN_TBL_U16(e->trade_bc, pnum);
    SG_1OOM_EN_TBL_U16(e->trade_percent, pnum);
    SG_1OOM_EN_TBL_U8(e->spymode_next, pnum);
    SG_1OOM_EN_TBL_U8(e->offer_field, pnum);
    SG_1OOM_EN_TBL_U8(e->offer_tech, pnum);
    SG_1OOM_EN_TBL_U16(e->offer_bc, pnum);
    for (player_id_t i = PLAYER_0; i < pnum; ++i) {
        SG_1OOM_EN_HATED_ID(e->hated[i]);
    }
    for (player_id_t i = PLAYER_0; i < pnum; ++i) {
        SG_1OOM_EN_HATED_ID(e->mutual_enemy[i]);
    }
    SG_1OOM_EN_TBL_U16(e->hatred, pnum);
    SG_1OOM_EN_TBL_U16(e->have_met, pnum);
    SG_1OOM_EN_TBL_U16(e->trade_established_bc, pnum);
    SG_1OOM_EN_TBL_U16(e->spying, pnum);
    SG_1OOM_EN_TBL_U16(e->spyfund, pnum);
    SG_1OOM_EN_TBL_U8(e->spymode, pnum);
    SG_1OOM_EN_U16(e->security);
    SG_1OOM_EN_TBL_U16(e->spies, pnum);
    SG_1OOM_EN_U32(e->reserve_bc);
    SG_1OOM_EN_U16(e->tax);
    SG_1OOM_EN_U8(e->base_shield);
    SG_1OOM_EN_U8(e->base_comp);
    SG_1OOM_EN_U8(e->base_weapon);
    SG_1OOM_EN_U8(e->colonist_oper_factories);
    SG_1OOM_EN_TBL_U8(e->tech.percent, TECH_FIELD_NUM);
    SG_1OOM_EN_TBL_U16(e->tech.slider, TECH_FIELD_NUM);
    SG_1OOM_EN_TBL_U8(e->tech.slider_lock, TECH_FIELD_NUM);
    SG_1OOM_EN_TBL_U32(e->tech.investment, TECH_FIELD_NUM);
    SG_1OOM_EN_TBL_U8(e->tech.project, TECH_FIELD_NUM);
    SG_1OOM_EN_TBL_U32(e->tech.cost, TECH_FIELD_NUM);
    SG_1OOM_EN_TBL_U16(e->tech.completed, TECH_FIELD_NUM);
    SG_1OOM_EN_U8(e->shipdesigns_num);
    pos = libsave_1oom_encode_orbits(buf, pos, e->orbit, planets);
    SG_1OOM_EN_TBL_TBL_U8(e->spyreportfield, pnum, TECH_FIELD_NUM);
    SG_1OOM_EN_TBL_U16(e->spyreportyear, pnum);
    SG_1OOM_EN_U8(e->shipi_colony);
    SG_1OOM_EN_U8(e->shipi_bomber);
    return pos;
}

static int libsave_1oom_encode_sd(uint8_t *buf, int pos, const shipdesign_t *sd)
{
    SG_1OOM_EN_TBL_U8(sd->name, SHIP_NAME_LEN);
    SG_1OOM_EN_U16(sd->cost);
    SG_1OOM_EN_U16(sd->space);
    SG_1OOM_EN_U8(sd->hull);
    SG_1OOM_EN_U8(sd->look);
    SG_1OOM_EN_TBL_U8(sd->wpnt, WEAPON_SLOT_NUM);
    SG_1OOM_EN_TBL_U8(sd->wpnn, WEAPON_SLOT_NUM);
    SG_1OOM_EN_U8(sd->engine);
    SG_1OOM_EN_U32(sd->engines);
    SG_1OOM_EN_TBL_U8(sd->special, SPECIAL_SLOT_NUM);
    SG_1OOM_EN_U8(sd->shield);
    SG_1OOM_EN_U8(sd->jammer);
    SG_1OOM_EN_U8(sd->comp);
    SG_1OOM_EN_U8(sd->armor);
    SG_1OOM_EN_U8(sd->man);
    SG_1OOM_EN_U16(sd->hp);
    return pos;
}

static int libsave_1oom_encode_srd(uint8_t *buf, int pos, const shipresearch_t *srd, shipdesign_id_t sdnum)
{
    for (shipdesign_id_t i = SHIPDESIGN_0; i < sdnum; ++i) {
        pos = libsave_1oom_encode_sd(buf, pos, &(srd->design[i]));
    }
    for (tech_field_t f = TECH_FIELD_COMPUTER; f < TECH_FIELD_NUM; ++f) {
        SG_1OOM_EN_TBL_TBL_U8(srd->researchlist[f], TECH_TIER_NUM, 3);
    }
    SG_1OOM_EN_TBL_TBL_U8(srd->researchcompleted, TECH_FIELD_NUM, TECH_PER_FIELD);
    SG_1OOM_EN_TBL_U8(srd->have_reserve_fuel, NUM_SHIPDESIGNS);
    SG_1OOM_EN_TBL_U16(srd->year, NUM_SHIPDESIGNS);
    SG_1OOM_EN_TBL_U32(srd->shipcount, NUM_SHIPDESIGNS);
    return pos;
}

static int libsave_1oom_encode_monster(uint8_t *buf, int pos, const monster_t *m)
{
    SG_1OOM_EN_U8(m->exists);
    SG_1OOM_EN_U16(m->x);
    SG_1OOM_EN_U16(m->y);
    SG_1OOM_EN_PLAYER_ID(m->killer);
    SG_1OOM_EN_PLANET_ID(m->dest);
    SG_1OOM_EN_U8(m->counter);
    SG_1OOM_EN_U8(m->nuked);
    return pos;
}

static int libsave_1oom_encode_evn(uint8_t *buf, int pos, const gameevents_t *ev, player_id_t pnum)
{
    SG_1OOM_EN_U16(ev->year);
    SG_1OOM_EN_BV(ev->done, GAME_EVENT_TBL_NUM);
    SG_1OOM_EN_DUMMY_V0(17);
    SG_1OOM_EN_U8(ev->have_plague);
    SG_1OOM_EN_PLAYER_ID(ev->plague_player);
    SG_1OOM_EN_PLANET_ID(ev->plague_planet_i);
    SG_1OOM_EN_U32(ev->plague_val);
    SG_1OOM_EN_U8(ev->have_nova);
    SG_1OOM_EN_PLAYER_ID(ev->nova_player);
    SG_1OOM_EN_PLANET_ID(ev->nova_planet_i);
    SG_1OOM_EN_U8(ev->nova_years);
    SG_1OOM_EN_U32(ev->nova_val);
    SG_1OOM_EN_U8(ev->have_accident);
    SG_1OOM_EN_PLANET_ID(ev->accident_planet_i);
    SG_1OOM_EN_U8(ev->have_comet);
    SG_1OOM_EN_PLAYER_ID(ev->comet_player);
    SG_1OOM_EN_PLANET_ID(ev->comet_planet_i);
    SG_1OOM_EN_U8(ev->comet_years);
    SG_1OOM_EN_U16(ev->comet_hp);
    SG_1OOM_EN_U16(ev->comet_dmg);
    SG_1OOM_EN_U8(ev->have_pirates);
    SG_1OOM_EN_PLANET_ID(ev->pirates_planet_i);
    SG_1OOM_EN_U16(ev->pirates_hp);
    pos = libsave_1oom_encode_monster(buf, pos, &(ev->crystal));
    pos = libsave_1oom_encode_monster(buf, pos, &(ev->amoeba));
    SG_1OOM_EN_PLANET_ID(ev->planet_orion_i);
    SG_1OOM_EN_U8(ev->have_guardian);
    for (player_id_t i = PLAYER_0; i < pnum; ++i) {
        SG_1OOM_EN_PLANET_ID(ev->home[i]);
    }
    SG_1OOM_EN_U8(ev->report_stars);
    SG_1OOM_EN_TBL_TBL_U32(ev->new_ships, pnum, NUM_SHIPDESIGNS);
    SG_1OOM_EN_TBL_TBL_U16(ev->spies_caught, pnum, pnum);
    SG_1OOM_EN_TBL_TBL_U16(ev->ceasefire, pnum, pnum);
    for (player_id_t i = PLAYER_0; i < pnum; ++i) {
        SG_1OOM_EN_BV(ev->help_shown[i], HELP_SHOWN_NUM);
        SG_1OOM_EN_DUMMY_V0(14);
    }
    SG_1OOM_EN_TBL_U16(ev->build_finished_num, pnum);
    for (player_id_t i = PLAYER_0; i < pnum; ++i) {
        SG_1OOM_EN_PLAYER_ID(ev->voted[i]);
    }
    SG_1OOM_EN_TBL_U8(ev->best_ecorestore, pnum);
    SG_1OOM_EN_TBL_U8(ev->best_wastereduce, pnum);
    SG_1OOM_EN_TBL_U8(ev->best_roboctrl, pnum);
    SG_1OOM_EN_TBL_U8(ev->best_terraform, pnum);
    return pos;
}

static int libsave_1oom_encode(uint8_t *buf, size_t buflen, const struct game_s *g)
{
    int pos = 0;
    if (buflen < sizeof(*g)) {
        log_error("Save: BUG: encode expected len > %i, got %i\n", sizeof(*g), buflen);
        return -1;
    }
    if (LIBSAVE_1OOM_VERSION == 0) {
        SG_1OOM_EN_U8(g->players);
    } else {
        SG_1OOM_EN_U16(g->players);
    }
    SG_1OOM_EN_BV(g->is_ai, PLAYER_NUM);
    SG_1OOM_EN_DUMMY_V0(5);
    SG_1OOM_EN_PLAYER_ID(g->active_player);
    SG_1OOM_EN_U8(g->difficulty);
    SG_1OOM_EN_U8(g->galaxy_size);
    SG_1OOM_EN_U8(g->galaxy_w);
    SG_1OOM_EN_U8(g->galaxy_h);
    if (LIBSAVE_1OOM_VERSION == 0) {
        SG_1OOM_EN_U8(g->galaxy_stars);
    } else {
        SG_1OOM_EN_U16(g->galaxy_stars);
    }
    SG_1OOM_EN_U16(g->galaxy_maxx);
    SG_1OOM_EN_U16(g->galaxy_maxy);
    SG_1OOM_EN_U32(g->galaxy_seed);
    SG_1OOM_EN_DUMMY_V0(4);
    SG_1OOM_EN_U16(g->year);
    SG_1OOM_EN_U16(g->enroute_num);
    SG_1OOM_EN_U16(g->transport_num);
    SG_1OOM_EN_U8(g->end);
    SG_1OOM_EN_PLAYER_ID(g->winner);
    SG_1OOM_EN_U8(g->election_held);
    SG_1OOM_EN_U8(g->nebula_num);
    if (LIBSAVE_1OOM_VERSION != 0) {
        SG_1OOM_EN_TBL_U16(g->nebula_type, g->nebula_num);
    }
    SG_1OOM_EN_TBL_U16(g->nebula_x, g->nebula_num);
    SG_1OOM_EN_TBL_U16(g->nebula_y, g->nebula_num);
    SG_1OOM_EN_TBL_TBL_U16(g->nebula_x0, g->nebula_num, 4);
    SG_1OOM_EN_TBL_TBL_U16(g->nebula_x1, g->nebula_num, 4);
    SG_1OOM_EN_TBL_TBL_U16(g->nebula_y0, g->nebula_num, 4);
    SG_1OOM_EN_TBL_TBL_U16(g->nebula_y1, g->nebula_num, 4);
    SG_1OOM_EN_TBL_TBL_U8(g->emperor_names, g->players, EMPEROR_NAME_LEN);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        SG_1OOM_EN_PLANET_ID(g->planet_focus_i[i]);
    }
    for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
        pos = libsave_1oom_encode_planet(buf, pos, &(g->planet[i]), g->players);
    }
    for (player_id_t j = PLAYER_0; j < g->players; ++j) {
        for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
            const seen_t *s = &(g->seen[j][i]);
            SG_1OOM_EN_PLAYER_ID(s->owner);
            SG_1OOM_EN_U16(s->pop);
            SG_1OOM_EN_U16(s->bases);
            SG_1OOM_EN_U16(s->factories);
        }
    }
    for (fleet_enroute_id_t i = FLEET_ENROUTE_0; i < g->enroute_num; ++i) {
        pos = libsave_1oom_encode_enroute(buf, pos, &(g->enroute[i]));
    }
    for (transport_id_t i = TRANSPORT_0; i < g->transport_num; ++i) {
        pos = libsave_1oom_encode_transport(buf, pos, &(g->transport[i]));
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        pos = libsave_1oom_encode_eto(buf, pos, &(g->eto[i]), g->players, g->galaxy_stars);
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        pos = libsave_1oom_encode_srd(buf, pos, &(g->srd[i]), g->eto[i].shipdesigns_num);
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        pos = libsave_1oom_encode_sd(buf, pos, &(g->current_design[i]));
    }
    pos = libsave_1oom_encode_evn(buf, pos, &(g->evn), g->players);
    SG_1OOM_EN_U32(LIBSAVE_1OOM_END);
    return pos;
}

static void libsave_1oom_make_header(uint8_t *buf, const char *savename)
{
    memset(buf, 0, LIBSAVE_1OOM_HDR_SIZE);
    memcpy(buf, (const uint8_t *)LIBSAVE_1OOM_MAGIC, 8);
    SET_LE_32(&buf[LIBSAVE_1OOM_OFFS_VERSION], LIBSAVE_1OOM_VERSION);
    strncpy((char *)&buf[LIBSAVE_1OOM_OFFS_NAME], savename, SAVE_NAME_LEN);
}

/* -------------------------------------------------------------------------- */

int libsave_1oom_save_do(const char *filename, const char *savename, const struct game_s *g)
{
    FILE *fd;
    uint8_t hdr[LIBSAVE_1OOM_HDR_SIZE];
    uint8_t *savebuf = NULL;
    size_t len;
    int res = -1;
    savebuf = lib_malloc(LIBSAVE_1OOM_DATA_SIZE);
    if ((len = libsave_1oom_encode(savebuf, LIBSAVE_1OOM_DATA_SIZE, g)) <= 0) {
        lib_free(savebuf);
        savebuf = NULL;
        return -1;
    }
    if (os_make_path_for(filename)) {
        log_error("Save: failed to create path for '%s'\n", filename);
    }
    libsave_1oom_make_header(hdr, savename);
    fd = fopen(filename, "wb+");
    if (0
      || (!fd)
      || (fwrite(hdr, LIBSAVE_1OOM_HDR_SIZE, 1, fd) != 1)
      || (fwrite(savebuf, len, 1, fd) != 1)
    ) {
        log_error("Save: failed to save '%s'\n", filename);
        unlink(filename);
        goto done;
    }
    log_message("Save: save '%s' '%s'\n", filename, savename);
    res = 0;
done:
    if (fd) {
        fclose(fd);
        fd = NULL;
    }
    lib_free(savebuf);
    savebuf = NULL;
    return res;
}
