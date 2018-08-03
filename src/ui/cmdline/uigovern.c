#include "config.h"

#include <stdio.h>
#include <string.h>

#include "uigovern.h"
#include "boolvec.h"
#include "comp.h"
#include "game.h"
#include "game_misc.h"
#include "game_planet.h"
#include "game_str.h"
#include "uidefs.h"
#include "uiinput.h"
#include "util.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

int ui_cmd_govern_toggle(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    planet_t *p = &(g->planet[g->planet_focus_i[api]]);
    if (p->owner != api) {
        return -1;
    }
    BOOLVEC_TOGGLE(p->extras, PLANET_EXTRAS_GOVERNOR);
    if (BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) {
        game_planet_govern(g, p);
    }
    return 0;
}

int ui_cmd_govern_readjust(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    planet_t *p = &(g->planet[g->planet_focus_i[api]]);
    if ((p->owner == api) && BOOLVEC_IS1(p->extras, PLANET_EXTRAS_GOVERNOR)) {
        game_planet_govern(g, p);
    }
    return 0;
}

int ui_cmd_govern_readjust_all(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    game_planet_govern_all_owned_by(g, api);
    return 0;
}

int ui_cmd_govern_bases(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    int v;
    planet_t *p = &(g->planet[g->planet_focus_i[api]]);
    if (p->owner != api) {
        return -1;
    }
    if (param[0].type == INPUT_TOKEN_NUMBER) {
        v = param[0].data.num;
    } else if (param[0].type == INPUT_TOKEN_RELNUMBER) {
        v = p->target_bases + param[0].data.num;
    } else {
        return -1;
    }
    SETRANGE(v, 0, 0xffff);
    p->target_bases = v;
    return 0;
}

int ui_cmd_govern_rest(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    struct input_list_s rl_in[] = {
        { 0, "1", NULL, game_str_tbl_gv_rest[0] },
        { 1, "2", NULL, game_str_tbl_gv_rest[1] },
        { 2, "3", NULL, game_str_tbl_gv_rest[2] },
        { -1, "q", NULL, "(quit)" },
        { 0, NULL, NULL, NULL }
    };
    int v;
    planet_t *p = &(g->planet[g->planet_focus_i[api]]);
    if (p->owner != api) {
        return -1;
    }
    v = ui_input_list(game_str_gv_rest, "> ", rl_in);
    if (v >= 0) {
        BOOLVEC_SET(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_SHIP, ((v & 1) != 0));
        BOOLVEC_SET(p->extras, PLANET_EXTRAS_GOV_SPEND_REST_IND, ((v & 2) != 0));
    }
    return 0;
}

int ui_cmd_govern_sg_toggle(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    BOOLVEC_TOGGLE(g->evn.gov_no_stargates, api);
    return 0;
}

int ui_cmd_govern_eco_mode(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    struct input_list_s rl_in[] = {
        { 0, "1", NULL, game_str_tbl_gv_ecom[0] },
        { 1, "2", NULL, game_str_tbl_gv_ecom[1] },
        { 2, "3", NULL, game_str_tbl_gv_ecom[2] },
        { 3, "4", NULL, game_str_tbl_gv_ecom[3] },
        { 4, "5", NULL, game_str_tbl_gv_ecom[4] },
        { -1, "q", NULL, "(quit)" },
        { 0, NULL, NULL, NULL }
    };
    int v;
    v = ui_input_list(game_str_gv_ecom, "> ", rl_in);
    if (v >= 0) {
        g->evn.gov_eco_mode[api] = v;
    }
    return 0;
}

int ui_cmd_govern_opts(struct game_s *g, int api, struct input_token_s *param, int num_param, void *var)
{
    const planet_t *p = &(g->planet[g->planet_focus_i[api]]);
    if (p->owner == api) {
        int rest = (p->extras[0] >> 1) & 3;
        printf("%s: target bases %i, %s %s\n", game_str_gv_thispl, p->target_bases, game_str_gv_rest, game_str_tbl_gv_rest[rest]);
    }
    {
        bool allow_sg = BOOLVEC_IS0(g->evn.gov_no_stargates, api);
        governor_eco_mode_t eco_mode = g->evn.gov_eco_mode[api];
        printf("%s:\n%s%s\n%s: %s\n", game_str_gv_allpl, allow_sg ? "": "Do not", game_str_gv_starg, game_str_gv_ecom, game_str_tbl_gv_ecom[eco_mode]);
    }
    return 0;
}
