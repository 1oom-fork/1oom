#include "config.h"

#include "game_aux.h"
#include "bits.h"
#include "game.h"
#include "lbx.h"
#include "lib.h"
#include "log.h"
#include "os.h"
#include "types.h"
#include "util_math.h"
#include "game_num.h"
#include "game_parsed.h"
#include "game_shipdesign.h"

/* -------------------------------------------------------------------------- */

static void init_star_dist(struct game_aux_s *gaux, struct game_s *g)
{
    int stars = g->galaxy_stars;
    for (int i = 0; i < stars; ++i) {
        gaux->star_dist[i][i] = 0;
        for (int j = i + 1; j < stars; ++j) {
            uint8_t dist;
            dist = (uint8_t)util_math_dist_steps(g->planet[i].x, g->planet[i].y, g->planet[j].x, g->planet[j].y);
            gaux->star_dist[i][j] = dist;
            gaux->star_dist[j][i] = dist;
#ifdef FEATURE_MODEBUG
            if (dist == 0) {
                LOG_DEBUG((0, "%s: dist %i  (%i,%i -> %i, %i)\n", __func__, dist, g->planet[i].x, g->planet[i].y, g->planet[j].x, g->planet[j].y));
            }
#endif
        }
    }
}

static void check_lbx_t5(const uint8_t *t, const char *lbxname, uint16_t want_num, uint16_t want_size)
{
    uint16_t num, size;
    num = GET_LE_16(t);
    t += 2;
    size = GET_LE_16(t);
    if (num != want_num) {
        log_fatal_and_die("%s.lbx: expected %i entries, got %i!\n", lbxname, want_num, num);
    }
    if (size != want_size) {
        log_fatal_and_die("%s.lbx: expected size %i, got %i!\n", lbxname, want_size, size);
    }
}

/* -------------------------------------------------------------------------- */

int game_aux_init(struct game_aux_s *gaux, struct game_s *g)
{
    uint8_t *data, *t;

    memset(g, 0, sizeof(struct game_s));
    g->gaux = gaux;

    t = lbxfile_item_get(LBXFILE_RESEARCH, 0);
    gaux->research.d0 = t + 4;
    t = lbxfile_item_get(LBXFILE_RESEARCH, 1);
    gaux->research.names = (char *)t + 4;
    t = lbxfile_item_get(LBXFILE_RESEARCH, 2);
    gaux->research.descr = (char *)t + 4;
    /* TODO check num/size*/

    t = lbxfile_item_get(LBXFILE_DIPLOMAT, 1);
    check_lbx_t5(t, "diplomat", DIPLOMAT_MSG_NUM, DIPLOMAT_MSG_LEN);
    gaux->diplomat.msg = (const char *)(t + 4);

    data = t = lbxfile_item_get(LBXFILE_DIPLOMAT, 0);
    check_lbx_t5(data, "diplomat", DIPLOMAT_D0_NUM, 2);
    t += 4;
    for (int i = 0; i < DIPLOMAT_D0_NUM; ++i, t += 2) {
        gaux->diplomat.d0[i] = GET_LE_16(t); /* all values < 0x10 */
    }
    lbxfile_item_release(LBXFILE_DIPLOMAT, data);

    data = t = lbxfile_item_get(LBXFILE_FIRING, 0);
    check_lbx_t5(data, "firing", NUM_SHIPLOOKS, 0x1c);
    t += 4;
    for (int j = 0; j < NUM_SHIPLOOKS; ++j) {
        for (int i = 0; i < 12; ++i, t += 2) {
            gaux->firing[j].d0[i] = GET_LE_16(t); /* all values < 0x20 */
        }
        gaux->firing[j].target_x = GET_LE_16(t); /* all values < 0x20 */
        t += 2;
        gaux->firing[j].target_y = GET_LE_16(t); /* all values < 0x20 */
        t += 2;
    }
    lbxfile_item_release(LBXFILE_FIRING, data);

    t = lbxfile_item_get(LBXFILE_EVENTMSG, 0);
    check_lbx_t5(t, "eventmsg", EVENTMSG_NUM, EVENTMSG_LEN);
    gaux->eventmsg = (const char *)(t + 4);

    gaux->move_temp = 0;
    gaux->savenamebuflen = FSDEV_PATH_MAX;
    gaux->savenamebuf = lib_malloc(gaux->savenamebuflen);
    gaux->savebuflen = sizeof(struct game_s) + 64;
    gaux->savebuf = lib_malloc(gaux->savebuflen);
    gaux->flag_cheat_galaxy = false;
    gaux->flag_cheat_events = false;
    gaux->initialized = true;
    return 0;
}

void game_aux_shutdown(struct game_aux_s *gaux)
{
    if (gaux->initialized) {
        lbxfile_item_release_file(LBXFILE_RESEARCH);
        lbxfile_item_release_file(LBXFILE_EVENTMSG);
        lbxfile_item_release_file(LBXFILE_DIPLOMAT);
        if (gaux->move_temp) {
            lib_free(gaux->move_temp);
            gaux->move_temp = 0;
        }
        lib_free(gaux->savenamebuf);
        gaux->savenamebuf = 0;
        lib_free(gaux->savebuf);
        gaux->savebuf = 0;
    }
}

uint8_t game_aux_get_firing_param_x(const struct game_aux_s *gaux, uint8_t look, uint8_t a2, bool dir)
{
    const uint8_t *f = &(gaux->firing[look].d0[0]);
    if (!dir) {
        if (a2 == 1) {
            return f[2];
        } else if (a2 == 2) {
            return f[4];
        } else /*if (a2 == 3)*/ {
            return f[0];
        }
    } else {
        if (a2 == 1) {
            return f[8];
        } else if (a2 == 2) {
            return f[10];
        } else /*if (a2 == 3)*/ {
            return f[6];
        }
    }
}

uint8_t game_aux_get_firing_param_y(const struct game_aux_s *gaux, uint8_t look, uint8_t a2, bool dir)
{
    const uint8_t *f = &(gaux->firing[look].d0[0]);
    if (!dir) {
        if (a2 == 1) {
            return f[3];
        } else if (a2 == 2) {
            return f[5];
        } else /*if (a2 == 3)*/ {
            return f[1];
        }
    } else {
        if (a2 == 1) {
            return f[9];
        } else if (a2 == 2) {
            return f[11];
        } else /*if (a2 == 3)*/ {
            return f[7];
        }
    }
}

void game_aux_start(struct game_aux_s *gaux, struct game_s *g)
{
    int n = 0;
    g->gaux = gaux;
    init_star_dist(gaux, g);
    for (int i = 0; i < g->players; ++i) {
        if (BOOLVEC_IS0(g->is_ai, i)) {
            ++n;
        }
    }
    gaux->local_players = n;
    if ((n > 1) && !gaux->move_temp) {
        gaux->move_temp = lib_malloc(sizeof(*gaux->move_temp));
    }
    game_aux_set_rules(g->xoptions);
}

void game_aux_set_rules(uint8_t xopt)
{
    xopt &= XOPTION_RULES_MASK;

    if (xopt == XOPTION_RULES_FIX) {
        tbl_monster[MONSTER_GUARDIAN][3].special[1] = 16;
        tbl_monster[MONSTER_GUARDIAN][4].special[1] = 26;
        tbl_monster[MONSTER_GUARDIAN][4].repair = 30;
        tbl_startship[0].engines = 2;
        tbl_startship[1].engines = 27;
        tbl_startship[2].engines = 110;
        tbl_startship[3].engines = 85;
        tbl_startship[4].engines = 100;
        tbl_startship[0].cost = 8;
        tbl_startship[1].cost = 14;
        tbl_startship[2].cost = 74;
        tbl_startship[3].cost = 65;
        tbl_startship[4].cost = 570;
        game_num_ng_tech[RACE_SILICOID][TECH_FIELD_PLANETOLOGY][24] = 1;
        game_num_ng_tech[RACE_SILICOID][TECH_FIELD_PLANETOLOGY][30] = 1;
        game_num_max_factories = 2700;
        game_num_accident_chk_factories = true;
        game_num_bt_wait_no_reload = true;
        game_num_bt_precap_tohit = true;
        game_num_bt_no_tohit_acc = true;
        game_num_bt_oracle_fix = true;
        game_num_news_orion = true;
        game_num_weapon_list_max = 64;
        game_num_aud_bounty_give = true;
        game_num_monster_rest_att = true;
        game_num_orbital_weap_any = true;
        game_num_orbital_weap_4 = true;
        game_num_orbital_torpedo = true;
        game_num_orbital_comp_fix = true;
        game_num_combat_trans_fix = true;
        game_num_stargate_redir_fix = true;
        game_num_trans_redir_fix = true;
        game_num_first_tech_rp_fix = true;
        game_num_waste_calc_fix = true;
        game_num_waste_adjust_fix = true;
        game_num_eco_slider_slack = 0;
        game_num_reset_tform_to_max = false;
        game_num_soil_rounding_fix = true;
        /*  we do not touch those as in fixbugs.pbx
         *  game_num_aud_ask_break_nap = false;
         *  game_num_retreat_redir_fix = false;
         *  game_num_doom_stack_fix = true;       */
    } else if (xopt == XOPTION_RULES_V13) {
        tbl_monster[MONSTER_GUARDIAN][3].special[1] = 26;
        tbl_monster[MONSTER_GUARDIAN][4].special[1] = 0;
        tbl_monster[MONSTER_GUARDIAN][4].repair = 0;
        tbl_startship[0].engines = 10;
        tbl_startship[1].engines = 30;
        tbl_startship[2].engines = 115;
        tbl_startship[3].engines = 90;
        tbl_startship[4].engines = 205;
        tbl_startship[0].cost = 10;
        tbl_startship[1].cost = 15;
        tbl_startship[2].cost = 66;
        tbl_startship[3].cost = 86;
        tbl_startship[4].cost = 591;
        game_num_ng_tech[RACE_SILICOID][TECH_FIELD_PLANETOLOGY][24] = 0;
        game_num_ng_tech[RACE_SILICOID][TECH_FIELD_PLANETOLOGY][30] = 0;
        game_num_max_factories = 2500;
        game_num_accident_chk_factories = false;
        game_num_bt_wait_no_reload = false;
        game_num_bt_precap_tohit = false;
        game_num_bt_no_tohit_acc = false;
        game_num_bt_oracle_fix = false;
        game_num_news_orion = false;
        game_num_weapon_list_max = 30;
        game_num_aud_bounty_give = false;
        game_num_monster_rest_att = false;
        game_num_orbital_weap_any = false;
        game_num_orbital_weap_4 = false;
        game_num_orbital_torpedo = false;
        game_num_orbital_comp_fix = false;
        game_num_combat_trans_fix = false;
        game_num_stargate_redir_fix = false;
        game_num_trans_redir_fix = false;
        game_num_first_tech_rp_fix = false;
        game_num_waste_calc_fix = false;
        game_num_waste_adjust_fix = false;
        game_num_eco_slider_slack = 7;
        game_num_reset_tform_to_max = true;
        game_num_soil_rounding_fix = false;
        /* we do set those, otherwise the option would be redundant */
        game_num_aud_ask_break_nap = false;
        game_num_retreat_redir_fix = false;
        game_num_doom_stack_fix = false;    /* you asked for it, you got it! */
    }
}
