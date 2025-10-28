#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "game_new.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_fix.h"
#include "game_num.h"
#include "game_shipdesign.h"
#include "game_str.h"
#include "game_tech.h"
#include "game_techtypes.h"
#include "lbx.h"
#include "lib.h"
#include "log.h"
#include "rnd.h"
#include "types.h"
#include "util.h"
#include "util_math.h"

/* -------------------------------------------------------------------------- */

#define EMPEROR_NAMES_PER_RACE  6
#define EMPEROR_NAME_LBX_LEN    20

static const uint8_t tbl_nebula_data[NEBULA_TYPE_NUM][4][4] = {
    { /* 0 */
        { 0x14, 0x07, 0x0f, 0x1f },
        { 0x05, 0x12, 0x0c, 0x02 },
        { 0x28, 0x1c, 0x1f, 0x24 },
        { 0x14, 0x27, 0x1b, 0x04 }
    },
    { /* 1 */
        { 0x0a, 0x14, 0x1b, 0x1e },
        { 0x11, 0x0d, 0x0d, 0x05 },
        { 0x1a, 0x26, 0x29, 0x29 },
        { 0x25, 0x18, 0x10, 0x10 }
    },
    { /* 2 */
        { 0x09, 0x03, 0x17, 0x1a },
        { 0x02, 0x08, 0x12, 0x0f },
        { 0x16, 0x19, 0x29, 0x23 },
        { 0x19, 0x15, 0x20, 0x24 }
    },
    { /* 3 */
        { 0x19, 0x08, 0x0a, 0x10 },
        { 0x0c, 0x0f, 0x09, 0x07 },
        { 0x1e, 0x1c, 0x16, 0x17 },
        { 0x1c, 0x1d, 0x0f, 0x09 }
    },
    { /* 4 */
        { 0x07, 0x0f, 0x15, 0x19 },
        { 0x02, 0x0f, 0x16, 0x23 },
        { 0x19, 0x25, 0x29, 0x29 },
        { 0x13, 0x1c, 0x23, 0x27 }
    },
    { /* 5 */
        { 0x03, 0x08, 0x0e, 0x09 },
        { 0x0e, 0x15, 0x02, 0x0a },
        { 0x23, 0x24, 0x1a, 0x22 },
        { 0x14, 0x23, 0x1f, 0x0f }
    },
    { /* 6 */
        { 0x0d, 0x09, 0x13, 0x05 },
        { 0x03, 0x0b, 0x21, 0x16 },
        { 0x1b, 0x22, 0x1b, 0x08 },
        { 0x21, 0x20, 0x25, 0x1e }
    },
    { /* 7 */
        { 0x08, 0x0f, 0x1c, 0x25 },
        { 0x08, 0x18, 0x02, 0x0a },
        { 0x25, 0x22, 0x23, 0x27 },
        { 0x19, 0x1f, 0x07, 0x12 }
    },
    { /* 8 */
        { 0x08, 0x05, 0x05, 0x1b },
        { 0x06, 0x01, 0x0c, 0x03 },
        { 0x23, 0x0c, 0x09, 0x25 },
        { 0x1f, 0x08, 0x15, 0x0a }
    },
    { /* 9 */
        { 0x04, 0x08, 0x0d, 0x25 },
        { 0x02, 0x12, 0x0e, 0x11 },
        { 0x2a, 0x26, 0x26, 0x29 },
        { 0x12, 0x1e, 0x21, 0x1d }
    }
};

/* index to planets.lbx + 1 */
/* index to starview.lbx - 6 */
static const uint8_t tbl_planet_type_infogfx[PLANET_TYPE_NUM][6] = {
    { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
    { 0x05, 0x05, 0x14, 0x14, 0x22, 0x22 },
    { 0x04, 0x04, 0x06, 0x06, 0x17, 0x17 },
    { 0x0b, 0x0b, 0x0d, 0x0d, 0x18, 0x18 },
    { 0x07, 0x07, 0x08, 0x08, 0x23, 0x23 },
    { 0x20, 0x20, 0x20, 0x1e, 0x1e, 0x1e },
    { 0x02, 0x02, 0x1a, 0x1a, 0x20, 0x20 },
    { 0x09, 0x09, 0x09, 0x19, 0x19, 0x19 },
    { 0x11, 0x11, 0x11, 0x1d, 0x1d, 0x1d },
    { 0x0f, 0x0f, 0x12, 0x12, 0x1f, 0x1f },
    { 0x15, 0x15, 0x15, 0x1c, 0x1c, 0x1c },
    { 0x0e, 0x0e, 0x0e, 0x1b, 0x1b, 0x1b },
    { 0x03, 0x03, 0x01, 0x10, 0x0c, 0x16 },
    { 0x01, 0x01, 0x0a, 0x0a, 0x13, 0x21 }
};

static const int8_t tbl_orion_race_relation[RACE_NUM][RACE_NUM] = {
    { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 0,-2, 0,-3,-1,-1, 0,-1 },
    { 1, 0, 0, 0, 0, 0,-1, 0, 1,-1 },
    { 1,-2, 0, 0, 0,-1,-1, 0,-1,-1 },
    { 1, 0, 0, 0, 0, 0, 0, 0, 0,-1 },
    { 1,-3, 0,-1, 0, 0,-1, 0, 0,-1 },
    { 1,-1,-1,-1, 0,-1, 0, 0, 0,-1 },
    { 1,-1, 0, 0, 0, 0, 0, 0, 0,-1 },
    { 1, 0, 1,-1, 0, 0, 0, 0, 0,-1 },
    { 1,-1,-1,-1,-1,-1,-1,-1,-1, 1 }
};

/* -------------------------------------------------------------------------- */

static void game_generate_planets(struct game_s *g)
{
    /* assumes the planet data is already 0'd by caller */
    uint16_t tblpx[PLANETS_MAX], tblpy[PLANETS_MAX];

    for (uint16_t h = 0; h < g->galaxy_h; ++h) {
        for (uint16_t w = 0; w < g->galaxy_w; ++w) {
            uint16_t x, y;
            again:
            x = rnd_1_n(0x2b, &g->seed) + w * 0x1c + 9;
            y = rnd_1_n(0x33, &g->seed) + h * 0x20 + 7;
            for (uint16_t i = 0; i < (h * g->galaxy_w + w); ++i) {
                if ((tblpx[i] < x) || (tblpy[i] < y)) {
                    if (util_math_dist_fast(tblpx[i], tblpy[i], x, y) < 20) {
                        goto again;
                    }
                } else if (util_math_dist_fast(tblpx[i], tblpy[i], x, y) < 10) {
                    goto again;
                }
            }
            tblpx[h * g->galaxy_w + w] = x;
            tblpy[h * g->galaxy_w + w] = y;
        }
    }

    for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
        bool in_nebula;
        planet_t *p;

        p = &g->planet[i];
        p->x = tblpx[i] + 4;
        p->y = tblpy[i] + 4;

        switch (rnd_0_nm1(0x14, &g->seed)) {
            /*
            case 0x00:
            case 0x01:
            case 0x02:
            */
            default:
                p->star_type = STAR_TYPE_YELLOW;
                break;
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x08:
                p->star_type = STAR_TYPE_RED;
                break;
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0c:
            case 0x0d:
                p->star_type = STAR_TYPE_GREEN;
                break;
            case 0x0e:
            case 0x0f:
            case 0x10:
                p->star_type = STAR_TYPE_BLUE;
                break;
            case 0x11:
            case 0x12:
                p->star_type = STAR_TYPE_WHITE;
                break;
            case 0x13:
                p->star_type = STAR_TYPE_NEUTRON;
                break;
        }

        p->look = rnd_0_nm1(2, &g->seed) * 6;
        p->frame = rnd_0_nm1(50, &g->seed);
        /* Needed for the original sequence of random numbers */
        /*p->field_16 =*/ rnd_0_nm1(4, &g->seed);

        in_nebula = false;
        for (int k = 0; k < g->nebula_num; ++k) {
            for (int j = 0; j < 4; ++j) {
                if (1
                  && (p->x >= g->nebula_x0[k][j]) && (p->x <= g->nebula_x1[k][j])
                  && (p->y >= g->nebula_y0[k][j]) && (p->y <= g->nebula_y1[k][j])
                ) {
                    in_nebula = true;
                    /* could break out here */
                }
            }
        }

        {
            planet_type_t t;
            int16_t r;
            t = PLANET_TYPE_NOT_HABITABLE;
            r = rnd_1_n(20, &g->seed) - (in_nebula ? 4 : 0);
            switch (p->star_type) {
                case STAR_TYPE_YELLOW:
                    /*if (r < -1) { t = PLANET_TYPE_NOT_HABITABLE; }*/
                    if (r == -1) { t = PLANET_TYPE_RADIATED; }
                    if (r == 0) { t = PLANET_TYPE_TOXIC; }
                    if (r == 1) { t = PLANET_TYPE_INFERNO; }
                    if (r == 2) { t = PLANET_TYPE_TUNDRA; }
                    if (r == 3) { t = PLANET_TYPE_BARREN; }
                    if (r == 4) { t = PLANET_TYPE_MINIMAL; }
                    if (r == 5) { t = PLANET_TYPE_DESERT; }
                    if (r == 6) { t = PLANET_TYPE_STEPPE; }
                    if ((r > 6) && (r < 9)) { t = PLANET_TYPE_ARID; }
                    if ((r > 8) && (r < 0xb)) { t = PLANET_TYPE_OCEAN; }
                    if ((r > 0xa) && (r < 0xd)) { t = PLANET_TYPE_JUNGLE; }
                    if (r > 0xc) { t = PLANET_TYPE_TERRAN; }
                    break;
                case STAR_TYPE_RED:
                    /*if (r < 2) { t = PLANET_TYPE_NOT_HABITABLE; }*/
                    if (r == 2) { t = PLANET_TYPE_RADIATED; }
                    if (r == 3) { t = PLANET_TYPE_TOXIC; }
                    if (r == 4) { t = PLANET_TYPE_INFERNO; }
                    if (r == 5) { t = PLANET_TYPE_DEAD; }
                    if (r == 6) { t = PLANET_TYPE_TUNDRA; }
                    if (r == 7) { t = PLANET_TYPE_BARREN; }
                    if (r == 8) { t = PLANET_TYPE_MINIMAL; }
                    if ((r > 8) && (r < 0xb)) { t = PLANET_TYPE_DESERT; }
                    if ((r > 0xa) && (r < 0xd)) { t = PLANET_TYPE_STEPPE; }
                    if ((r > 0xc) && (r < 0x10)) { t = PLANET_TYPE_ARID; }
                    if ((r > 0xf) && (r < 0x12)) { t = PLANET_TYPE_OCEAN; }
                    if ((r > 0x11) && (r < 0x14)) { t = PLANET_TYPE_JUNGLE; }
                    if (r == 0x14) { t = PLANET_TYPE_TERRAN; }
                    break;
                case STAR_TYPE_GREEN:
                    /*if (r < 2) { t = PLANET_TYPE_NOT_HABITABLE; }*/
                    if (r == 2) { t = PLANET_TYPE_RADIATED; }
                    if (r == 3) { t = PLANET_TYPE_TOXIC; }
                    if (r == 4) { t = PLANET_TYPE_INFERNO; }
                    if (r == 5) { t = PLANET_TYPE_DEAD; }
                    if (r == 6) { t = PLANET_TYPE_TUNDRA; }
                    if (r == 7) { t = PLANET_TYPE_BARREN; }
                    if (r == 8) { t = PLANET_TYPE_MINIMAL; }
                    if (r == 9) { t = PLANET_TYPE_DESERT; }
                    if ((r > 9) && (r < 0xc)) { t = PLANET_TYPE_STEPPE; }
                    if ((r > 0xb) && (r < 0xe)) { t = PLANET_TYPE_ARID; }
                    if ((r > 0xd) && (r < 0x10)) { t = PLANET_TYPE_OCEAN; }
                    if ((r > 0xf) && (r < 0x12)) { t = PLANET_TYPE_JUNGLE; }
                    if (r > 0x11) { t = PLANET_TYPE_TERRAN; }
                    break;
                case STAR_TYPE_WHITE:
                    /*if (r < 3) { t = PLANET_TYPE_NOT_HABITABLE; }*/
                    if (r == 3) { t = PLANET_TYPE_RADIATED; }
                    if ((r > 3) && (r < 6)) { t = PLANET_TYPE_TOXIC; }
                    if ((r > 5) && (r < 8)) { t = PLANET_TYPE_INFERNO; }
                    if ((r > 7) && (r < 0xa)) { t = PLANET_TYPE_DEAD; }
                    if ((r > 9) && (r < 0xc)) { t = PLANET_TYPE_TUNDRA; }
                    if ((r > 0xb) && (r < 0xe)) { t = PLANET_TYPE_BARREN; }
                    if ((r > 0xd) && (r < 0x10)) { t = PLANET_TYPE_MINIMAL; }
                    if ((r > 0xf) && (r < 0x12)) { t = PLANET_TYPE_DESERT; }
                    if (r == 0x12) { t = PLANET_TYPE_STEPPE; }
                    if (r == 0x13) { t = PLANET_TYPE_ARID; }
                    if (r == 0x14) { t = PLANET_TYPE_OCEAN; }
                    break;
                case STAR_TYPE_BLUE:
                    /*if (r < 4) { t = PLANET_TYPE_NOT_HABITABLE; }*/
                    if ((r > 3) && (r < 6)) { t = PLANET_TYPE_RADIATED; }
                    if ((r > 5) && (r < 8)) { t = PLANET_TYPE_TOXIC; }
                    if ((r > 7) && (r < 0xa)) { t = PLANET_TYPE_INFERNO; }
                    if ((r > 9) && (r < 0xc)) { t = PLANET_TYPE_DEAD; }
                    if ((r > 0xb) && (r < 0xe)) { t = PLANET_TYPE_TUNDRA; }
                    if ((r > 0xd) && (r < 0x10)) { t = PLANET_TYPE_BARREN; }
                    if ((r > 0xf) && (r < 0x12)) { t = PLANET_TYPE_MINIMAL; }
                    if (r == 0x12) { t = PLANET_TYPE_DESERT; }
                    if (r == 0x13) { t = PLANET_TYPE_STEPPE; }
                    if (r == 0x14) { t = PLANET_TYPE_ARID; }
                    break;
                case STAR_TYPE_NEUTRON:
                    /*if (r < 5) { t = PLANET_TYPE_NOT_HABITABLE; }*/
                    if ((r > 4) && (r < 0xa)) { t = PLANET_TYPE_RADIATED; }
                    if ((r > 9) && (r < 0xd)) { t = PLANET_TYPE_TOXIC; }
                    if ((r > 0xc) && (r < 0x10)) { t = PLANET_TYPE_INFERNO; }
                    if ((r > 0xf) && (r < 0x12)) { t = PLANET_TYPE_DEAD; }
                    if (r == 0x12) { t = PLANET_TYPE_TUNDRA; }
                    if (r == 0x13) { t = PLANET_TYPE_BARREN; }
                    if (r == 0x14) { t = PLANET_TYPE_MINIMAL; }
                    break;
                default:
                    break;
            }
            p->type = t;
        }

        {
            uint16_t a;
            switch (p->type) {
                case PLANET_TYPE_NOT_HABITABLE:
                    a = 10;
                    break;
                case PLANET_TYPE_RADIATED:
                case PLANET_TYPE_TOXIC:
                case PLANET_TYPE_INFERNO:
                    a = rnd_1_n(7, &g->seed) * 5 + 5;
                    break;
                case PLANET_TYPE_DEAD:
                case PLANET_TYPE_TUNDRA:
                    a = rnd_1_n(7, &g->seed) * 5 + 0xf;
                    break;
                case PLANET_TYPE_BARREN:
                case PLANET_TYPE_MINIMAL:
                    a = rnd_1_n(5, &g->seed) * 5 + 0x19;
                    break;
                /*
                case PLANET_TYPE_DESERT:
                case PLANET_TYPE_STEPPE:
                case PLANET_TYPE_ARID:
                case PLANET_TYPE_OCEAN:
                */
                default:
                    a = rnd_1_n(4, &g->seed) * 5 + (((int)(p->type)) - (int)PLANET_TYPE_DESERT) * 0xa + 0x1e;
                    break;
                case PLANET_TYPE_JUNGLE:
                    a = rnd_1_n(4, &g->seed) * 5 + 0x46;
                    break;
                case PLANET_TYPE_TERRAN:
                    a = rnd_1_n(4, &g->seed) * 5 + 0x50;
                    break;
            }
            p->max_pop2 = a;
        }

        {
            uint8_t di;
            di = rnd_1_n(10, &g->seed);
            while ((di == 1) || (di == 10)) {
                if (di == 1) {
                    p->max_pop2 -= 20;
                } else {
                    p->max_pop2 += 20;
                }
                di = rnd_1_n(10, &g->seed);
                SETMAX(p->max_pop2, 10);
            }
            SETMIN(p->max_pop2, 120);
        }

        p->max_pop3 = p->max_pop2;
        p->max_pop1 = p->max_pop2;
        p->battlebg = in_nebula ? 0 : rnd_1_n(4, &g->seed);
        p->special = PLANET_SPECIAL_NORMAL;
        {
            star_type_t star_type;
            int16_t di;
            star_type = p->star_type;
            if (p->type >= PLANET_TYPE_STEPPE) {
                di = rnd_1_n(0x14, &g->seed);
                if (star_type == STAR_TYPE_RED) { di -= 4; } else if (star_type == STAR_TYPE_GREEN) { di -= 2; }
                if (di <= 2) {
                    p->special = PLANET_SPECIAL_POOR;
                }
                di = rnd_1_n(0x14, &g->seed);
                if (star_type == STAR_TYPE_RED) { di -= 4; } else if (star_type == STAR_TYPE_GREEN) { di -= 2; }
                if (di <= 5) {
                    if (p->special == PLANET_SPECIAL_POOR) {
                        p->special = PLANET_SPECIAL_ULTRA_POOR;
                    }
                }
            }
            di = rnd_1_n(0x14, &g->seed) - (in_nebula ? 8 : 0);
            if (star_type == STAR_TYPE_BLUE) { di -= 2; } else if (star_type == STAR_TYPE_NEUTRON) { di -= 5; }
            if ((((int)PLANET_TYPE_STEPPE) - ((int)p->type)) > di) {
                p->special = PLANET_SPECIAL_RICH;
            }
            di = rnd_1_n(0x14, &g->seed) - (in_nebula ? 8 : 0);
            if (star_type == STAR_TYPE_BLUE) { di -= 2; } else if (star_type == STAR_TYPE_NEUTRON) { di -= 5; }
            if (di < 6) {
                if (p->special == PLANET_SPECIAL_RICH) {
                    p->special = PLANET_SPECIAL_ULTRA_RICH;
                }
            }

        }
        if (1
          && (p->type >= PLANET_TYPE_MINIMAL)
          && (p->type <= PLANET_TYPE_OCEAN)
          && (p->special == PLANET_SPECIAL_NORMAL)
          && (rnd_1_n(10, &g->seed) == 1)
        ) {
            p->special = PLANET_SPECIAL_ARTIFACTS;
        }
        {
            int16_t di;
            di = rnd_1_n(100, &g->seed);
            if (p->special == PLANET_SPECIAL_ULTRA_RICH) {
                di -= 20;
            } else if (p->special == PLANET_SPECIAL_RICH) {
                di -= 10;
            }
            di -= 15 - (int)(p->type);
            if (di < 15) {
                p->rocks = PLANET_ROCKS_MANY;
            } else if (di < 40) {
                p->rocks = PLANET_ROCKS_SOME;
            } else {
                p->rocks = PLANET_ROCKS_NONE;
            }
        }
        p->growth = PLANET_GROWTH_NORMAL;
        if (p->type < PLANET_TYPE_MINIMAL) {
            p->growth = PLANET_GROWTH_HOSTILE;
        } else if ((p->type > PLANET_TYPE_DESERT) && (rnd_1_n(12, &g->seed) == 1)) {
            uint16_t v;
            p->growth = PLANET_GROWTH_FERTILE;
            v = p->max_pop2 + (p->max_pop2 / 20) * 5;
            p->max_pop2 = v;
            p->max_pop3 = v;
            p->max_pop1 = v;
        }
        p->owner = PLAYER_NONE;
        p->prev_owner = PLAYER_NONE;
        p->claim = PLAYER_NONE;
        if (p->type == PLANET_TYPE_NOT_HABITABLE) {
            p->growth = PLANET_GROWTH_NORMAL;
            p->special = PLANET_SPECIAL_NORMAL;
        }
        p->pop_oper_fact = 2;
        p->infogfx = tbl_planet_type_infogfx[p->type][rnd_0_nm1(6, &g->seed)] - 1;
        p->slider[PLANET_SLIDER_IND] = 100;
        p->reloc = i;
    }

    {
        uint16_t tx = 0, ty = 0;
        for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
            planet_t *p;
            p = &g->planet[i];
            if (p->x > tx) {
                tx = p->x;
            }
            if (p->y > ty) {
                ty = p->y;
            }
        }
        g->galaxy_maxx = tx + 27;
        g->galaxy_maxy = ty + 25;
    }

    for (player_id_t i = PLAYER_0; i < PLAYER_NUM; ++i) {
        g->evn.voted[i] = PLAYER_NONE;
    }

    {
        planet_id_t i;
        planet_t *p;
        i = rnd_0_nm1(g->galaxy_stars, &g->seed);
        g->evn.planet_orion_i = i;
        g->evn.have_guardian = true;
        p = &g->planet[i];
        p->type = PLANET_TYPE_TERRAN;
        p->max_pop3 = 120;
        p->max_pop2 = 120;
        p->max_pop1 = 120;
        p->special = PLANET_SPECIAL_4XTECH;
        p->growth = PLANET_GROWTH_NORMAL;
        p->infogfx = tbl_planet_type_infogfx[p->type][rnd_0_nm1(6, &g->seed)] - 1;
    }
}

static void game_generate_galaxy(struct game_s *g)
{
    {
        uint16_t a = 0, w = 0, h = 0;

        switch (g->galaxy_size) {
            default:
            case GALAXY_SIZE_SMALL:
                a = rnd_0_nm1(2, &g->seed);
                w = 6;
                h = 4;
                break;
            case GALAXY_SIZE_MEDIUM:
                a = rnd_1_n(3, &g->seed);
                w = 8;
                h = 6;
                break;
            case GALAXY_SIZE_LARGE:
                a = rnd_1_n(2, &g->seed) + 1;
                w = 10;
                h = 7;
                break;
            case GALAXY_SIZE_HUGE:
                a = rnd_1_n(3, &g->seed) + 1;
                w = 12;
                h = 9;
                break;
        }

        g->nebula_num = a;
        g->galaxy_w = w;
        g->galaxy_h = h;
        g->galaxy_maxx = (w - 1) * 0x1c + 0x14;
        g->galaxy_maxy = (h - 1) * 0x20 + 0x10;
        g->galaxy_stars = w * h;
    }

    g->guardian_killer = PLAYER_NONE;

    {
        uint16_t gw, gh;
        uint16_t v8 = 0, i = 0;

        gw = g->galaxy_maxx - 70;
        gh = g->galaxy_maxy - 70;

        while (i < g->nebula_num) {
            uint16_t found, j;

            g->nebula_type[i] = rnd_0_nm1(NEBULA_TYPE_NUM, &g->seed);
            g->nebula_x[i] = rnd_1_n(gw, &g->seed) + 4;
            g->nebula_y[i] = rnd_1_n(gh, &g->seed) + 4;

            found = j = 0;

            while (j < i) {
                if (0
                  || (g->nebula_type[i] == g->nebula_type[j])
                  || ((abs(g->nebula_x[i] - g->nebula_x[j]) < 0x3c) && (abs(g->nebula_y[i] - g->nebula_y[j]) < 0x3c))
                ) {
                    found = 1;
                }
                ++j;
            }
            if (found == 0) {
                ++i;
            }
            if (++v8 > 200) {
                g->nebula_num = i;
                i = 0;
            }
        }
    }

    for (int i = 0; i < g->nebula_num; ++i) {
        uint16_t type = g->nebula_type[i];
        for (int j = 0; j < 4; ++j) {
            g->nebula_x0[i][j] = tbl_nebula_data[type][0][j] + g->nebula_x[i];
            g->nebula_x1[i][j] = tbl_nebula_data[type][2][j] + g->nebula_x[i];
            g->nebula_y0[i][j] = tbl_nebula_data[type][1][j] + g->nebula_y[i];
            g->nebula_y1[i][j] = tbl_nebula_data[type][3][j] + g->nebula_y[i];
        }
    }

    game_generate_planets(g);
}

static void game_generate_planet_names(struct game_s *g)
{
    BOOLVEC_DECLARE(in_use, PLANETS_MAX);
    BOOLVEC_CLEAR(in_use, PLANETS_MAX);
    for (planet_id_t i = PLANET_0; i < g->galaxy_stars; ++i) {
        uint16_t j;
        do {
            j = rnd_0_nm1(PLANETS_MAX, &g->seed);
        } while (BOOLVEC_IS1(in_use, j));
        BOOLVEC_SET1(in_use, j);
        strcpy(g->planet[i].name, game_str_tbl_planet_names[j]);
    }
    strcpy(g->planet[g->evn.planet_orion_i].name, game_str_planet_name_orion);
}

static void game_generate_race_banner(struct game_s *g)
{
    const uint16_t race_tbl[] = {7, 1, 5, 0, 3, 9, 4, 6, 2, 8};
    BOOLVEC_DECLARE(in_use, (int)BANNER_NUM);
    uint16_t loops;
    loops = 0;
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        race_t race;
        bool in_use = false;
        race = g->eto[i].race;
        if (race != RACE_RANDOM) {
            continue;
        }
        race = race_tbl[rnd_0_nm1(RACE_NUM, &g->seed)];
        for (player_id_t j = PLAYER_0; j < i; ++j) {
            if (g->eto[j].race == race) {
                in_use = true;
                --i;
                break;
            }
        }
        if (!in_use) {
            g->eto[i].race = race;
        }
    }
    loops = 0;
    BOOLVEC_CLEAR(in_use, (int)BANNER_NUM);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        banner_t banner;
        banner = g->eto[i].banner;
        if (banner != BANNER_RANDOM) {
            BOOLVEC_SET1(in_use, banner);
            continue;
        }
        banner = rnd_0_nm1(BANNER_NUM, &g->seed);
        if (BOOLVEC_IS0(in_use, banner)) {
            BOOLVEC_SET1(in_use, banner);
            g->eto[i].banner = banner;
        } else {
            --i; /* try again */
            if (++loops == 100) {
                for (banner = 0; banner < BANNER_NUM; ++banner) {
                    if (BOOLVEC_IS0(in_use, banner)) {
                        BOOLVEC_SET1(in_use, banner);
                        g->eto[i].banner = banner;
                        break;
                    }
                }
                loops = 0;
            }
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if (IS_AI(g, i)) {
            race_t r;
            r = g->eto[i].race;
            g->eto[i].trait2 = game_num_tbl_trait2[r][rnd_0_nm1(TRAIT2_TBL_NUM, &g->seed)];
        }
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        race_t r;
        r = g->eto[i].race;
        g->eto[i].trait1 = game_num_tbl_trait1_vanilla[r][rnd_0_nm1(TRAIT1_TBL_NUM, &g->seed)];
    }
}

static void game_generate_home_etc(struct game_s *g)
{
    planet_id_t tblhome[PLAYER_NUM], homei;
    uint16_t loops;
    bool flag_all_ok;
start_of_func:
    flag_all_ok = false;
    loops = 0;
    game_generate_race_banner(g);
    while ((!flag_all_ok) && (loops < 200)) {
        flag_all_ok = true;
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            planet_id_t pi;
            bool flag_again2;
            flag_again2 = true;
            while (flag_again2) {
                if (g->galaxy_stars > 24) {
                    pi = rnd_1_n(g->galaxy_w - 2, &g->seed) + rnd_1_n(g->galaxy_h - 2, &g->seed) * g->galaxy_w;
                } else if (g->players > 3) {
                    pi = rnd_0_nm1(g->galaxy_stars, &g->seed);
                } else {
                    pi = rnd_1_n(g->galaxy_w - 2, &g->seed) + rnd_1_n(g->galaxy_h - 2, &g->seed) * g->galaxy_w;
                }
                flag_again2 = false;
                for (player_id_t j = PLAYER_0; j < i; ++j) {
                    if (tblhome[j] == pi) {
                        flag_again2 = true;
                    }
                }
                if (pi == g->evn.planet_orion_i) {
                    flag_again2 = true;
                }
            }
            tblhome[i] = pi;
        }
        {
            uint16_t dist, mindist;
            mindist = 10000;
            for (player_id_t i = PLAYER_0; (i < g->players) && (i < 2); ++i) {
                for (player_id_t j = PLAYER_0; j < g->players; ++j) {
                    if (i == j) { continue; }
                    dist = util_math_dist_fast(g->planet[tblhome[i]].x, g->planet[tblhome[i]].y, g->planet[tblhome[j]].x, g->planet[tblhome[j]].y);
                    SETMIN(mindist, dist);
                }
            }
            if (mindist < 80) {
                /* homeworlds too close */
                flag_all_ok = false;    /* could break out here */
            }
            for (player_id_t i = PLAYER_0; (i < g->players) && (i < 2); ++i) {
                mindist = 10000;
                for (planet_id_t j = PLANET_0; j < g->galaxy_stars; ++j) {
                    planet_t *p;
                    p = &g->planet[j];
                    if (1
                      && (p->type >= PLANET_TYPE_MINIMAL)
                      && (j != g->evn.planet_orion_i)
                      && (j != tblhome[i])
                    ) {
                        dist = util_math_dist_fast(g->planet[tblhome[i]].x, g->planet[tblhome[i]].y, p->x, p->y);
                        SETMIN(mindist, dist);
                    }
                }
                if (mindist > 29) {
                    /* ok planet too far away */
                    flag_all_ok = false;    /* could break out here */
                }
            }
            for (player_id_t i = PLAYER_0; (i < g->players) && (i < 2); ++i) {
                mindist = 10000;
                for (planet_id_t j = PLANET_0; j < g->galaxy_stars; ++j) {
                    planet_t *p;
                    p = &g->planet[j];
                    if (1
                      && (j != g->evn.planet_orion_i)
                      && (j != tblhome[i])
                    ) {
                        dist = util_math_dist_fast(g->planet[tblhome[i]].x, g->planet[tblhome[i]].y, p->x, p->y);
                        SETMIN(mindist, dist);
                    }
                }
                if (mindist > 29) {
                    /* regular planet too far away */
                    flag_all_ok = false;    /* could break out here */
                }
            }
        }
        ++loops;
    }
#if 0
    /* FIXME in MOO1 this is actually after the if (!flag_all_ok) test, making it ineffective */
    for (player_id_t i = PLAYER_0; (i < g->players) && (i <= g->difficulty); ++i) {
        uint16_t dist, mindist;
        mindist = 10000;
        for (planet_id_t j = PLANET_0; j < g->galaxy_stars; ++j) {
            planet_t *p;
            p = &g->planet[j];
            if (1
              && (p->type >= PLANET_TYPE_MINIMAL)
              && (j != g->evn.planet_orion_i)
              && (j != tblhome[i])
            ) {
                dist = util_math_dist_fast(g->planet[tblhome[i]].x, g->planet[tblhome[i]].y, p->x, p->y);
                SETMIN(mindist, dist);
            }
        }
        if (mindist > "\x1d\x23\x27\x3c\x50"[g->difficulty]) {
            /* ok planet too far away */
            flag_all_ok = false;    /* could break out here */
        }
    }
#endif
    if (!flag_all_ok) {
        game_generate_galaxy(g);
        game_generate_planet_names(g);
        goto start_of_func;
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        planet_t *p;
        homei = tblhome[i];
        g->evn.home[i] = homei;
        g->planet_focus_i[i] = homei;
        p = &g->planet[homei];
        p->owner = i;
        p->claim = i;
        p->type = PLANET_TYPE_TERRAN;
        p->star_type = STAR_TYPE_YELLOW;
        p->max_pop2 = 100;
        p->max_pop3 = 100;
        p->max_pop1 = 100;
        p->infogfx = tbl_planet_type_infogfx[p->type][rnd_0_nm1(6, &g->seed)] - 1;
        p->growth = PLANET_GROWTH_NORMAL;
        p->special = PLANET_SPECIAL_NORMAL;
        BOOLVEC_SET1(p->explored, i);
        BOOLVEC_SET1(p->within_srange, i);
        p->within_frange[i] = 1;

        switch (g->difficulty) {
            case DIFFICULTY_SIMPLE:
            case DIFFICULTY_EASY:
            case DIFFICULTY_AVERAGE:
                p->pop = 50;
                p->factories = 30;
                p->total_prod = 55;
                break;
            case DIFFICULTY_HARD:
            case DIFFICULTY_IMPOSSIBLE:
                p->pop = 40;
                p->factories = 30;
                p->total_prod = 50;
                break;
            default:
                break;
        }
        p->pop_prev = p->pop;
        game_new_generate_home_name(g->eto[i].race, p->name);
    }
    for (planet_id_t j = PLANET_0; j < g->galaxy_stars; ++j) {
        player_id_t owner;
        owner = g->planet[j].owner;
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            g->seen[i][j].owner = (owner == i) ? i : PLAYER_NONE;
        }
        /* seen_pop.._factories cleared here */
    }
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e;
        e = &g->eto[i];
        e->have_colony_for = PLANET_TYPE_MINIMAL;
        e->factory_adj_cost = 10;
        e->factory_cost = 10;
        e->have_engine = 1;
        e->ai_p3_countdown = rnd_1_n(6, &g->seed) + 14;
        e->ai_p2_countdown = rnd_1_n(10, &g->seed);
        e->colonist_oper_factories = 2;
        e->tech.percent[TECH_FIELD_WEAPON] = 1;
        e->tech.percent[TECH_FIELD_FORCE_FIELD] = 1;
        e->tech.percent[TECH_FIELD_PLANETOLOGY] = 1;
        e->tech.percent[TECH_FIELD_COMPUTER] = 1;
        e->tech.percent[TECH_FIELD_PROPULSION] = 1;
        e->tech.percent[TECH_FIELD_CONSTRUCTION] = 1;
        if (IS_HUMAN(g, i)) {
            e->tech.slider[TECH_FIELD_WEAPON] = 20;
            e->tech.slider[TECH_FIELD_FORCE_FIELD] = 15;
            e->tech.slider[TECH_FIELD_PLANETOLOGY] = 15;
            e->tech.slider[TECH_FIELD_CONSTRUCTION] = 10;
        } else {
            e->tech.slider[TECH_FIELD_PLANETOLOGY] = 40;
            e->tech.slider[TECH_FIELD_CONSTRUCTION] = 20;
        }
        e->tech.slider[TECH_FIELD_COMPUTER] = 20;
        e->tech.slider[TECH_FIELD_PROPULSION] = 20;
        if (IS_AI(g, i)) {
            uint16_t n;
            planet_t *p, *q;
#if 1
            /* BUG? MOO1 sets this way earlier if the function, leaving it to the home of last player */
            homei = tblhome[i];
#endif
            q = &g->planet[homei];
            n = 0;
            for (planet_id_t j = PLANET_0; j < g->galaxy_stars; ++j) {
                p = &g->planet[j];
                if (1
                  && (p->type > PLANET_TYPE_MINIMAL - 1)
                  && (util_math_dist_fast(p->x, p->y, q->x, q->y) <= 0x1e)
                ) {
                    ++n;
                }
            }
            if (n > 1) {
                q->slider[PLANET_SLIDER_SHIP] = 10;
                q->slider[PLANET_SLIDER_DEF] = 0;
                q->slider[PLANET_SLIDER_IND] = 90;
                q->slider[PLANET_SLIDER_ECO] = 0;
                q->slider[PLANET_SLIDER_TECH] = 0;
            } else {
                e->ai_p3_countdown = rnd_1_n(6, &g->seed) + 24;
                q->slider[PLANET_SLIDER_SHIP] = 0;
                q->slider[PLANET_SLIDER_DEF] = 0;
                q->slider[PLANET_SLIDER_IND] = 60;
                q->slider[PLANET_SLIDER_ECO] = 10;
                q->slider[PLANET_SLIDER_TECH] = 40;
                e->tech.slider[TECH_FIELD_WEAPON] = 0;
                e->tech.slider[TECH_FIELD_FORCE_FIELD] = 0;
                e->tech.slider[TECH_FIELD_PLANETOLOGY] = 0;
                e->tech.slider[TECH_FIELD_CONSTRUCTION] = 0;
                e->tech.slider[TECH_FIELD_COMPUTER] = 0;
                e->tech.slider[TECH_FIELD_PROPULSION] = 100;
            }
        }
        {
            shipresearch_t *srd;
            shipdesign_t *sd;
            srd = &g->srd[i];
            sd = &srd->design[0];
            e->shipdesigns_num = startship_num;
            for (shipdesign_id_t j = SHIPDESIGN_0; j < startship_num; ++j, ++sd) {
                *sd = tbl_startship[j];
                strcpy(sd->name, game_str_tbl_stship_names[j]);
                sd->look += e->banner * SHIP_LOOK_PER_BANNER;
            }
            memcpy(&g->current_design[i], &srd->design[SHIPDESIGN_0], sizeof(shipdesign_t));
            for (shipdesign_id_t j = SHIPDESIGN_0; j < NUM_SHIPDESIGNS; ++j) {
                shipcount_t n;
                n = startfleet_ships[j];
                e->orbit[tblhome[i]].ships[j] = n;
                srd->shipcount[j] = n;
            }
        }
        e->fuel_range = 3;
        /* BUG? these shipi values are wrong, but fixed by first next turn */
        e->shipi_colony = 1;
        e->shipi_bomber = 4;
    }
}

static void game_generate_relation_etc(struct game_s *g)
{
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        empiretechorbit_t *e;
        e = &g->eto[i];
        for (player_id_t j = PLAYER_0; j < g->players; ++j) {
            uint16_t v;
            v = tbl_orion_race_relation[e->race][g->eto[j].race] * 12;
            e->relation1[j] = v;
            e->relation2[j] = v;
            for (tech_field_t k = TECH_FIELD_COMPUTER; k < TECH_FIELD_NUM; ++k) {
                e->spyreportfield[j][k] = 1;
            }
        }
    }
}

static bool game_generate_research_check_field(uint8_t (*rl)[TECH_TIER_NUM][3], tech_field_t field, const char *items)
{
    for (int tier = 0; tier < TECH_TIER_NUM; ++tier) {
        for (int l = 0; l < 3; ++l) {
            uint8_t v;
            v = rl[field][tier][l];
            if (v && strchr(items, v)) {
                return true;
            }
        }
    }
    return false;
}

static void game_generate_research(struct game_s *g, const uint8_t *rflag)
{
    bool flag_got_essentials = false;
    while (!flag_got_essentials) {
        flag_got_essentials = true;
        for (player_id_t pli = PLAYER_0; pli < g->players; ++pli) {
            uint16_t rmax;
            uint8_t (*rl)[TECH_TIER_NUM][3];
            rl = g->srd[pli].researchlist;
            rmax = (g->eto[pli].race == RACE_PSILON) ? 3 : 2;
            for (tech_field_t field = TECH_FIELD_COMPUTER; field < TECH_FIELD_NUM; ++field) {
                for (int tier = 0; tier < TECH_TIER_NUM; ++tier) {
                    for (int l = 0; l < 3; ++l) {
                        rl[field][tier][l] = 0;
                    }
                }
                for (int tier = 0; tier < TECH_TIER_NUM; ++tier) {
                    uint16_t num_taken;
                    num_taken = 0;
                    for (int l = 0; l < 2/*?*/; ++l) {
                        rl[field][tier][l] = 0;
                    }
                    while (num_taken == 0) {
                        for (int16_t ti = tier * 5 + 4; ti >= (tier * 5); --ti) {
                            bool flag_skip;
                            flag_skip = false;
                            if (g->eto[pli].race == RACE_SILICOID) {
                                ++ti;
                                if ((field == TECH_FIELD_PLANETOLOGY)
                                  && strchr("\x10\x16\x03\x06\x09\x0c\x0f\x12\x05\x0d\x22", ti)
                                ) {
                                    flag_skip = true;
                                } else if ((field == TECH_FIELD_CONSTRUCTION)
                                  && strchr("\x05\x0f\x19\x23\x2d", ti)
                                ) {
                                    flag_skip = true;
                                }
                                --ti;
                            }
                            if (rflag[field * 50 + ti] == 0) {
                                flag_skip = true;
                            }
                            if ((!flag_skip) && (rnd_1_n(4, &g->seed) <= rmax) && (num_taken < 3)) {
                                rl[field][tier][num_taken++] = ti + 1;
                            }
                        }
                        for (int loops = 0; loops < 3; ++loops) {
                            for (int l = 0; l < 2; ++l) {
                                uint8_t v0, v1;
                                v1 = rl[field][tier][l + 1];
                                if (v1 != 0) {
                                    v0 = rl[field][tier][l];
                                    if (v0 > v1) {
                                        rl[field][tier][l] = v1;
                                        rl[field][tier][l + 1] = v0;
                                    }
                                }
                            }
                        }
                    }
                }
                if (field == TECH_FIELD_COMPUTER) {
                    if (!game_generate_research_check_field(rl, field, "\x08\x12\x1c\x26\x30")) {
                        flag_got_essentials = false;
                    }
                }
                if (field == TECH_FIELD_PROPULSION) {
                    if (!game_generate_research_check_field(rl, field, "\x03\x05")) {
                        flag_got_essentials = false;
                    }
                }
                if (field == TECH_FIELD_PLANETOLOGY) {
                    if (!game_generate_research_check_field(rl, field, "\x0c\x16\x20\x2a")) {
                        flag_got_essentials = false;
                    }
                }
                if (field == TECH_FIELD_WEAPON) {
                    if (!game_generate_research_check_field(rl, field, "\x04\x08\x0b\x12\x1b\x22\x29\x2c")) {
                        flag_got_essentials = false;
                    }
                }
            }
        }
    }

    for (player_id_t pli = PLAYER_0; pli < g->players; ++pli) {
        empiretechorbit_t *e;
        shipresearch_t *srd;
        e = &g->eto[pli];
        srd = &g->srd[pli];
        e->base_shield = SHIP_SHIELD_CLASS_I;
        e->base_comp = SHIP_COMP_MARK_I;
        e->base_weapon = WEAPON_NUCLEAR_MISSILE_2;
        for (tech_field_t field = TECH_FIELD_COMPUTER; field < TECH_FIELD_NUM; ++field) {
            e->tech.completed[field] = 1;
            srd->researchcompleted[field][0] = 1;
        }
    }

    for (player_id_t pli = PLAYER_0; pli < g->players; ++pli) {
        empiretechorbit_t *e;
        shipresearch_t *srd;
        if (IS_HUMAN(g, pli)) {
            continue;
        }
        e = &g->eto[pli];
        srd = &g->srd[pli];
        for (tech_field_t field = TECH_FIELD_COMPUTER; field < TECH_FIELD_NUM; ++field) {
            uint16_t v = 0;
            while (v == 0) {
                v = srd->researchlist[field][0][rnd_0_nm1(3, &g->seed)];
            }
            if (game_ai_fix_first_tech_cost) {
                game_tech_start_next(g, pli, field, v);
            } else {
                e->tech.project[field] = v;
                e->tech.cost[field] = v * v * 50;
            }
        }
    }
}

static void game_generate_misc(struct game_s *g)
{
    g->year = 1;
    g->evn.year = 40;
    for (player_id_t pli = PLAYER_0; pli < g->players; ++pli) {
        empiretechorbit_t *e;
        e = &g->eto[pli];
        for (player_id_t pli2 = PLAYER_0; pli2 < g->players; ++pli2) {
            e->hated[pli2] = PLAYER_NONE;
            e->mutual_enemy[pli2] = PLAYER_NONE;
        }
    }
}

static void game_generate_emperor_names(struct game_s *g, const uint8_t *namedata)
{
    for (player_id_t pli = PLAYER_0; pli < g->players; ++pli) {
        uint16_t base;
        if (g->emperor_names[pli][0] != '\0') {
            continue;
        }
        base = ((uint16_t)g->eto[pli].race) * EMPEROR_NAMES_PER_RACE;
        for (int loops = 0; loops < 200; ++loops) {
            char buf[EMPEROR_NAME_LBX_LEN];
            const char *str;
            bool flag_name_used;
            str = (const char *)&namedata[4 + (base + rnd_0_nm1(EMPEROR_NAMES_PER_RACE, &g->seed)) * EMPEROR_NAME_LBX_LEN];
            strcpy(buf, str);
            util_trim_whitespace(buf); /* fix "Zygot  " */
            flag_name_used = false;
            for (player_id_t i = PLAYER_0; i < g->players; ++i) {
                if (strcasecmp(buf, g->emperor_names[i]) == 0) {
                    flag_name_used = true;
                    break;
                }
            }
            if (!flag_name_used) {
                strcpy(g->emperor_names[pli], buf);
                break;
            }
        }
    }
}

/* -------------------------------------------------------------------------- */

int game_new(struct game_s *g, struct game_aux_s *gaux, struct game_new_options_s *opt)
{
    uint8_t researchflag[6 * 50];
    memset(g, 0, sizeof(struct game_s));
    g->gaux = gaux;
    if (opt->galaxy_seed == 0) {
        g->galaxy_seed = rnd_get_new_seed();
    } else {
        g->galaxy_seed = opt->galaxy_seed;
    }
    g->seed = g->galaxy_seed;
    g->players = opt->players;
    g->difficulty = opt->difficulty;
    g->galaxy_size = opt->galaxy_size;
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        g->eto[i].race = opt->pdata[i].race;
        g->eto[i].banner = opt->pdata[i].banner;
        if (opt->pdata[i].is_ai) {
            BOOLVEC_SET1(g->is_ai, i);
        }
    }
    {
        const uint8_t *rawdata;
        rawdata = gaux->research.d0;
        for (int f = 0; f < 6; ++f) {
            for (int t = 0; t < 50; ++t) {
                researchflag[f * 50 + t] = (rawdata[(f * 50 + t) * 6] != 0xff) ? 1 : 0;
            }
            researchflag[f * 50] = 0;
        }
    }
    researchflag[TECH_FIELD_WEAPON * 50 + (TECH_WEAP_DEATH_RAY - 1)] = 0;
    {
        uint32_t vo, vr = 0, vb = 0, m = 1, va = 0;
        vo = g->difficulty + g->galaxy_size * 10 + g->players * 100;
        for (player_id_t i = PLAYER_0; i < g->players; ++i, m *= 0x10) {
            vr += ((g->eto[i].race + 1) % (RACE_NUM + 1)) * m;
        }
        m = 1;
        for (player_id_t i = PLAYER_0; i < g->players; ++i, m *= 10) {
            vb += ((g->eto[i].banner + 1) % (BANNER_NUM + 1)) * m;
            if (IS_HUMAN(g, i)) {
                va += m;
            }
        }
        log_message("Game: new game -new %u:0x%x:%u:0x%x:%u\n", vo, vr, vb, g->galaxy_seed, va);
    }
    game_generate_galaxy(g);
    game_generate_planet_names(g);
    game_generate_home_etc(g);
    game_generate_relation_etc(g);
    game_generate_research(g, researchflag);
    game_generate_misc(g);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        char *b;
        const char *str;
        str = opt->pdata[i].playername;
        b = g->emperor_names[i];
        strncpy(b, str, EMPEROR_NAME_LEN);
        b[EMPEROR_NAME_LEN - 1] = '\0';
        util_trim_whitespace(b);
        str = opt->pdata[i].homename;
        if (*str != '\0') {
            b = g->planet[g->planet_focus_i[i]].name;
            strncpy(b, str, PLANET_NAME_LEN);
            b[PLANET_NAME_LEN - 1] = '\0';
            util_trim_whitespace(b);
        }
    }
    {
        uint8_t *namedata = lbxfile_item_get(LBXFILE_NAMES, 0, NULL);
        game_generate_emperor_names(g, namedata);
        lbxfile_item_release(LBXFILE_NAMES, namedata);
    }
    g->active_player = PLAYER_0;
    return 0;
}

int game_new_tutor(struct game_s *g, struct game_aux_s *gaux)
{
    struct game_new_options_s opt = GAME_NEW_OPTS_DEFAULT;
    opt.galaxy_size = GALAXY_SIZE_MEDIUM;
    opt.players = 4;
    opt.difficulty = DIFFICULTY_SIMPLE;
    opt.pdata[PLAYER_0].race = RACE_KLACKON;
    opt.pdata[PLAYER_0].banner = BANNER_WHITE;
    strcpy(opt.pdata[PLAYER_0].playername, "Mr Tutor");
    strcpy(opt.pdata[PLAYER_0].homename, "SOL");
    opt.galaxy_seed = 0xfda3f;
    return game_new(g, gaux, &opt);
}

void game_new_generate_emperor_name(race_t race, char *buf)
{
    if (race == RACE_RANDOM) {
        strcpy(buf, game_str_rndempname);
    } else {
        uint32_t seed = rnd_get_new_seed();
        uint8_t *namedata = lbxfile_item_get(LBXFILE_NAMES, 0, NULL);
        int base = race * EMPEROR_NAMES_PER_RACE;
        const char *str = (const char *)&namedata[4 + (base + rnd_0_nm1(EMPEROR_NAMES_PER_RACE, &seed)) * EMPEROR_NAME_LBX_LEN];
        /* TODO check if in use for the case of forced same races */
        strcpy(buf, str);
        util_trim_whitespace(buf); /* fix "Zygot  " */
        lbxfile_item_release(LBXFILE_NAMES, namedata);
    }
}

void game_new_generate_home_name(race_t race, char *buf)
{
    /* TODO check if in use for the case of forced same races */
    strcpy(buf, game_str_tbl_home_names[race]);
}

void game_new_generate_other_emperor_name(struct game_s *g, player_id_t player)
{
    uint8_t *namedata = lbxfile_item_get(LBXFILE_NAMES, 0, NULL);
    int base = g->eto[player].race * EMPEROR_NAMES_PER_RACE;
    for (int loops = 0; loops < 500; ++loops) {
        const char *str;
        char buf[EMPEROR_NAME_LBX_LEN + 1];
        bool flag_in_use;
        str = (const char *)&namedata[4 + (base + rnd_0_nm1(EMPEROR_NAMES_PER_RACE, &g->seed)) * EMPEROR_NAME_LBX_LEN];
        strcpy(buf, str);
        util_trim_whitespace(buf); /* fix "Zygot  " */
        flag_in_use = true;
        for (player_id_t i = PLAYER_0; i < g->players; ++i) {
            if (strcasecmp(g->emperor_names[i], buf) == 0) {
                flag_in_use = false;
                break;
            }
        }
        if (!flag_in_use) {
            strcpy(g->emperor_names[player], buf);
            break;
        }
    }
    lbxfile_item_release(LBXFILE_NAMES, namedata);
}
