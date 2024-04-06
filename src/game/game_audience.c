/* FIXME multiplayer, split to ai */

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_audience.h"
#include "comp.h"
#include "game.h"
#include "game_ai.h"
#include "game_aux.h"
#include "game_diplo.h"
#include "game_fix.h"
#include "game_spy.h"
#include "game_str.h"
#include "game_tech.h"
#include "log.h"
#include "rnd.h"
#include "ui.h"

/* -------------------------------------------------------------------------- */

#define DEBUGLEVEL_AUDIENCE 3

static void game_audience_prepare(struct audience_s *au, player_id_t ph, player_id_t pa)
{
    au->buf = ui_get_strbuf();
    au->ph = ph;
    au->pa = pa;
    for (int i = 0; i < AUDIENCE_STR_MAX; ++i) {
        au->strtbl[i] = 0;
    }
    au->gfxi = 0;
    au->musi = 0;
    au->dtype_next = 0;
}

static void game_audience_start_human(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int v;
    v = eh->trust[pa] + game_diplo_get_mood(g, ph, pa) + game_diplo_tbl_reldiff[ea->trait1];
    if (v < -100) {
        au->dtype = (eh->treaty[pa] >= TREATY_WAR) ? 20 : 21;
        au->mode = 1;
    } else {
        v += eh->relation1[pa];
        au->dtype = (v > -50) ? 22 : 23;
        au->mode = 0;
    }
}

static int game_audience_print_tech(struct game_s *g, tech_field_t field, uint8_t tech, char *buf, bool add_str)
{
    int len;
    game_tech_get_name(g->gaux, field, tech, buf);
    len = strlen(buf);
    buf[len++] = ' ';
    if (add_str) {
        len += sprintf(&buf[len], "%s.", game_str_au_tech);
    } else {
        buf[len] = '\0';
    }
    return len;
}

static void game_audience_str_append_offer(const struct game_s *g, char *buf, tech_field_t field, uint8_t tech, uint16_t bc)
{
    buf += strlen(buf);
    *buf++ = ' ';
    *buf = 0;
    if (tech != 0) {
        game_tech_get_name(g->gaux, field, tech, buf);
        buf += strlen(buf);
        sprintf(buf, " %s", game_str_au_tech);
    } else if (bc != 0) {
        sprintf(buf, "%u %s", bc, game_str_bc);
    }
}

static const char *game_audience_get_str1(struct audience_s *au)
{
    struct game_s *g = au->g;
    uint8_t dtype = au->dtype;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    bool flag_framed = false;
    if ((dtype == 5) || (dtype == 7) || (dtype == 35) || (dtype == 37) || (dtype == 43) || (dtype == 45) || (dtype == 51) || (dtype == 53)) {
        flag_framed = true;
        --dtype;
    }
    if (g->gaux->diplo_d0_rval == -1) {
        const char *msg;
        char *buf = au->buf;
        {
            int v = rnd_0_nm1(g->gaux->diplomat.d0[dtype], &g->seed);
            if (g->players == 2) {
                --v;
                SETMAX(v, 0);
            }
            g->gaux->diplo_d0_rval = v;
            msg = DIPLOMAT_MSG_PTR(g->gaux, v, dtype);
        }
        for (int i = 0; (i < DIPLOMAT_MSG_LEN) && (msg[i] != 0); ++i) {
            char c;
            c = msg[i];
            if (c & 0x80) {
                const char *s;
                int len = 0;
                s = 0;
                switch (c & 0x7f) {
                    case 0:
                        s = game_str_tbl_race[eh->race];
                        break;
                    case 1:
                        s = game_str_tbl_race[ea->race];
                        break;
                    case 0x16:
                        s = (ea->race == RACE_ALKARI) ? "n" : ""; /* FIXME variable string */
                        break;
                    case 2:
                        s = (!eh->diplo_p2[pa]) ? game_str_au_facts : game_str_au_bases;
                        break;
                    case 0xb:
                        if (eh->treaty[pa] == TREATY_ALLIANCE) {
                            s = game_str_au_allian;
                        } else if (eh->treaty[pa] == TREATY_NONAGGRESSION) {
                            s = game_str_au_nonagg;
                        } else if (eh->trade_bc[pa] != 0) {
                            s = game_str_au_tradea;
                        } else {
                            s = game_str_au_treaty;
                        }
                        break;
                    case 0x17:
                        if (eh->broken_treaty[pa] == TREATY_ALLIANCE) {
                            s = game_str_au_allian;
                        } else if (eh->broken_treaty[pa] == TREATY_NONAGGRESSION) {
                            s = game_str_au_nonagg;
                        } else if (eh->broken_treaty[pa] == TREATY_NONE) {
                            s = game_str_au_tradea;
                        } else {
                            s = game_str_au_treaty;
                        }
                        break;
                    case 3:
                        s = g->emperor_names[ph];
                        break;
                    case 9:
                        s = g->emperor_names[pa];
                        break;
                    case 4:
                        len = sprintf(buf, "%s", game_str_tbl_te_field[eh->diplo_p2[pa]]);
                        buf[0] = tolower(buf[0]);   /* FIXME BUG? this leaves "force Field" */
                        break;
                    case 5:
                        s = g->planet[eh->diplo_p1[pa]].name;
                        break;
                    case 6:
                        s = game_str_tbl_race[g->eto[eh->diplo_p2[pa]].race];
                        break;
                    case 8:
                        s = game_str_tbl_race[g->eto[eh->hated[pa]].race];
                        break;
                    case 0xc:
                        s = game_str_tbl_race[g->eto[eh->au_ask_break_treaty[pa]].race];
                        break;
                    case 0xf:
                        len = sprintf(buf, "%i", g->year + YEAR_BASE);
                        break;
                    case 0x13:
                        len = sprintf(buf, "%i", au->tribute_bc);
                        break;
                    case 7:
                        len = sprintf(buf, "%u", eh->au_want_trade[pa]);
                        break;
                    case 0x15:
                        len = sprintf(buf, "%u", eh->trade_bc[pa]);
                        break;
                    case 0xa:
                        len = sprintf(buf, "\x02 %s\x01", game_str_au_amreca);
                        break;
                    case 0x11:
                        s = game_str_tbl_race[g->eto[au->pstartwar].race];
                        break;
                    case 0x12:
                        s = game_str_tbl_race[g->eto[au->pwar].race];
                        break;
                    case 0xd:
                        if (eh->au_attack_gift_bc[pa] != 0) {
                            len = sprintf(buf, "%i %s.", eh->au_attack_gift_bc[pa], game_str_bc);
                        } else {
                            len = game_audience_print_tech(g, eh->au_attack_gift_field[pa], eh->au_attack_gift_tech[pa], buf, true);
                        }
                        break;
                    case 0xe:
                        len = game_audience_print_tech(g, eh->au_want_field[pa], eh->au_want_tech[pa], buf, true);
                        break;
                    case 0x10:
                        len = game_audience_print_tech(g, au->tribute_field, au->tribute_tech, buf, false);
                        break;
                    case 0x14:
                        len = game_audience_print_tech(g, au->tribute_field, au->tribute_tech, buf, true);
                        break;
                    default:
                        *buf = c;
                        len = 1;
                        break;
                }
                if (s) {
                    len = sprintf(buf, "%s", s);
                }
                buf += len;
            } else {
                *buf++ = c;
            }
        }
        *buf = 0;
        if (flag_framed) {
            buf += sprintf(buf, "\x02 %s\x01", game_str_au_framed);
        }
        au->gfxi = DIPLOMAT_MSG_GFX(msg);
        au->musi = DIPLOMAT_MSG_MUS(msg);
        strcpy(&au->buf[AUDIENCE_BUF2_POS], au->buf);
    } else {
        strcpy(au->buf, &au->buf[AUDIENCE_BUF2_POS]);
    }
    return au->buf;
}

static int16_t game_audience_sub3(struct audience_s *au)
{
    struct game_s *g = au->g;
    char *cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    int len;
    int16_t selected = 0, seldef;
    game_audience_get_str1(au);
    cbuf[0] = '0';
    for (int i = 0; i < AUDIENCE_STR_MAX; ++i) {
        au->strtbl[i] = 0;
    }
    switch (au->dtype) {
        case 24:
        case 25:
        case 26:
        case 30:
        case 76:
            au->strtbl[0] = cbuf;
            len = sprintf(cbuf, "%s %s", game_str_au_bull, game_str_au_accept);
            cbuf += len + 1;
            au->strtbl[1] = cbuf;
            len = sprintf(cbuf, "%s %s", game_str_au_bull, game_str_au_reject);
            selected = 1;
            break;
        case 27:
            au->strtbl[0] = cbuf;
            len = sprintf(cbuf, "%s %s", game_str_au_bull, game_str_au_agree);
            cbuf += len + 1;
            au->strtbl[1] = cbuf;
            len = sprintf(cbuf, "%s %s", game_str_au_bull, game_str_au_forget);
            selected = 1;
            break;
        case 29:
            {
                int i;
                for (i = 0; (i < 4) && (i < eh->au_tech_trade_num[pa]); ++i) {
                    au->strtbl[i] = cbuf;
                    len = sprintf(cbuf, "%s ", game_str_au_bull);
                    cbuf += len;
                    game_tech_get_name(g->gaux, eh->au_tech_trade_field[pa][i], eh->au_tech_trade_tech[pa][i], cbuf);
                    cbuf += strlen(cbuf) + 1;
                }
                au->strtbl[i] = cbuf;
                sprintf(cbuf, "%s %s", game_str_au_bull, game_str_au_forget2);
                selected = i;
                /*game_audience_get_str1(au);*/ /* FIXME why again? */
            }
            break;
        default:
            LOG_DEBUG((1, "%s: BUG unhandled dtype %u\n", __func__, au->dtype));
            break;
    }
    seldef = selected;
    if ((au->dtype == 28) || (au->dtype == 58) || (au->dtype == 29)) {
        ui_audience_show2(au);
        selected = 1;
        if (au->dtype == 29) {
            strcpy(au->buf, game_str_au_inxchng);
            au->condtbl = 0;
            selected = ui_audience_ask2a(au);
        }
    } else {
        /*62346*/
        au->condtbl = 0;
        selected = ui_audience_ask2b(au);
        if (((selected == 1) || (selected == -1)) && ((eh->offer_tech[pa] != 0) || (eh->offer_bc[pa] != 0))) {
            bool flag_qtype = (!rnd_0_nm1(2, &g->seed));
            strcpy(au->buf, flag_qtype ? game_str_au_whatif1 : game_str_au_perrec1);
            game_audience_str_append_offer(g, au->buf, eh->offer_field[pa], eh->offer_tech[pa], eh->offer_bc[pa]);
            if (flag_qtype) {
                int l;
                l = strlen(au->buf);
                au->buf[l++] = ' ';
                strcpy(&au->buf[l], game_str_au_whatif2);
            }
            strcat(au->buf, game_str_au_ques);
            au->condtbl = 0;
            selected = ui_audience_ask2b(au);
        }
    }
    /*624eb*/
    if (selected == -1) {
        selected = seldef;
    }
    return selected;
}

static bool game_audience_sub2(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa, pf = PLAYER_NONE;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if ((i != ph) && (i != pa) && (ea->treaty[i] >= TREATY_WAR) && (eh->treaty[i] == TREATY_ALLIANCE)) {
            pf = i;
        }
    }
    if (pf == PLAYER_NONE) {
        return true;
    }
    au->dtype = 27;
    eh->au_ask_break_treaty[pa] = pf;
    g->gaux->diplo_d0_rval = -1;
    if (game_audience_sub3(au) != 0) {
        /* ea->relation1[ph] -= rnd_1_n(6, &g->seed) + 6; SETMAX(ea->relation1[ph], -100); BUG? useless */
        ea->relation1[ph] = -100;
        eh->relation1[pa] = -100;
        return false;
    } else {
        game_diplo_break_treaty(g, ph, pf);
        return true;
    }
}

static void game_audience_set_dtype(struct audience_s *au, uint8_t dtype, int a2)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    au->dtype = dtype;
    if ((dtype == 31) || ((dtype >= 33) && (dtype <= 41)) || ((dtype >= 62) && (dtype <= 68))) {
        switch (a2) {
            case 0:
            case 1:
            case 2:
                if ((eh->blunder[pa] != 0) && (!rnd_0_nm1(2, &g->seed))) {
                    au->dtype = eh->blunder[pa] + 30;
                    eh->blunder[pa] = 0;
                } else if ((eh->broken_treaty[pa] != TREATY_NONE) && (!rnd_0_nm1(4, &g->seed))) {
                    au->dtype = 33;
                    eh->broken_treaty[pa] = TREATY_NONE;
                } else {
                    au->dtype = 31;
                }
                break;
            case 3:
                if ((!rnd_0_nm1(4, &g->seed)) && (eh->tribute_tech[pa] != 0)) {
                    au->tribute_field = eh->tribute_field[pa];
                    au->tribute_tech = eh->tribute_tech[pa];
                    eh->tribute_tech[pa] = 0;
                    au->dtype = 66;
                }
                break;
            default:
                break;
        }
    }
    g->gaux->diplo_d0_rval = -1;
    game_audience_get_str1(au);
    ui_audience_show3(au);
}

static int game_audience_check_mood(struct audience_s *au, int a0, int a2)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int v;
    switch (a2) {
        default:
        case 0:
            v = eh->mood_treaty[pa];
            break;
        case 1:
            v = eh->mood_trade[pa];
            break;
        case 2:
            v = eh->mood_peace[pa];
            break;
        case 3:
            v = eh->mood_tech[pa];
            break;
    }
    v += eh->trust[pa] + eh->relation1[pa] + ((eh->race == RACE_HUMAN) ? 50 : 0) + game_diplo_tbl_reldiff[ea->trait1];
    v += rnd_1_n(100, &g->seed);
    v -= a0;
    if (eh->treaty[pa] == TREATY_ALLIANCE) {
        v += 40;
    }
    game_diplo_annoy(g, ph, pa, 1);
    switch (a2) {
        default:
        case 0:
            eh->mood_treaty[pa] -= rnd_1_n(30, &g->seed) + 20;
            break;
        case 1:
            eh->mood_trade[pa] -= rnd_1_n(30, &g->seed) + 20;
            break;
        case 2:
            eh->mood_peace[pa] -= rnd_1_n(50, &g->seed) + 50;
            break;
        case 3:
            eh->mood_tech[pa] -= rnd_1_n(50, &g->seed) + 20;
            break;
    }
    if (v < -75) {
        return 0;
    } else if (v < -50) {
        return 1;
    } else if (v < -0) {
        return 2;
    } else {
        return 3;
    }
}

static int game_audience_sweeten(struct audience_s *au, int a0)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int bc;
    tech_field_t field = 0;
    uint8_t tech;
    char buf[0x60];
    bool flag_bc;
    int16_t selected = 0;
    bc = (((rnd_1_n(8, &g->seed) + rnd_1_n(8, &g->seed)) * g->year) / 25) * 25;
    if (bc > eh->reserve_bc) {
        bc = 0;
    }
    {
        struct spy_esp_s s[1];
        s->spy = pa;
        s->target = ph;
        if (game_spy_esp_sub1(g, s, g->year, 1) > 0) {
            field = s->tbl_field[0];
            tech = s->tbl_tech2[0];
        } else {
            tech = 0;
        }
    }
    if ((bc == 0) && (tech == 0)) {
        return 0;
    }
    if (((tech != 0) && (a0 == 1)) || (bc == 0)) {
        flag_bc = false;
    } else {
        tech = 0;
        flag_bc = true;
    }
    buf[0] = 0;
    game_audience_str_append_offer(g, buf, field, tech, bc);
    if (!rnd_0_nm1(2, &g->seed)) {
        sprintf(au->buf, "%s%s %s", game_str_au_perthr1, buf, game_str_au_perthr2);
    } else {
        sprintf(au->buf, "%s%s %s", game_str_au_alsoof1, buf, game_str_au_alsoof2);
    }
    au->strtbl[0] = game_str_au_opts3[0];
    au->strtbl[1] = game_str_au_opts3[1];
    au->strtbl[2] = 0;
    au->condtbl = 0;
    selected = ui_audience_ask3(au);
    if ((selected == -1) || (selected == 1)) {
        return 1;
    }
    if (!flag_bc) {
        game_tech_get_new(g, pa, field, tech, TECHSOURCE_TRADE, ph, 0, false);
    } else {
        eh->reserve_bc -= bc;
        ea->reserve_bc += bc;
    }
    return 3;
}

static player_id_t audience_menu_race(struct audience_s *au, player_id_t *rtbl, uint8_t rnum, const char *titlestr)
{
    struct game_s *g = au->g;
    char *cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
    int16_t selected = 0;
    for (int i = 0; i < rnum; ++i) {
        empiretechorbit_t *e = &(g->eto[rtbl[i]]);
        int len;
        au->strtbl[i] = cbuf;
        len = sprintf(cbuf, "%s %s", game_str_au_bull, game_str_tbl_races[e->race]);
        cbuf += len + 1;
    }
    au->strtbl[rnum] = game_str_au_opts3[1];
    au->strtbl[rnum + 1] = 0;
    strcpy(au->buf, titlestr);
    au->condtbl = 0;
    selected = ui_audience_ask4(au);
    if ((selected >= 0) && (selected < rnum)) {
        return rtbl[selected];
    }
    return PLAYER_NONE;
}

static void audience_menu_treaty(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int16_t selected = 0;
    bool condtbl[6];
    uint8_t war_num, all_num, dtype;
    player_id_t war_tbl[PLAYER_NUM], all_tbl[PLAYER_NUM];
    int si;
    for (int i = 0; i < TBLLEN(condtbl); ++i) {
        condtbl[i] = true;
    }
    strcpy(au->buf, game_str_au_youprte);
    if (eh->treaty[pa] != TREATY_NONE) {
        condtbl[0] = false;
    }
    if (eh->treaty[pa] >= TREATY_ALLIANCE) {
        condtbl[1] = false;
    }
    if (eh->treaty[pa] < TREATY_WAR) {
        condtbl[2] = false;
    }
    if (eh->treaty[pa] >= TREATY_WAR) {
        condtbl[3] = false;
    }
    war_num = 0;
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if ((i != ph) && (i != pa) && (ea->treaty[i] < TREATY_WAR)) {
            war_tbl[war_num++] = i;
        }
    }
    if (war_num == 0) {
        condtbl[3] = false;
    }
    all_num = 0;
    for (player_id_t i = PLAYER_0; i < g->players; ++i) {
        if ((i != ph) && (i != pa) && (ea->treaty[i] == TREATY_ALLIANCE)) {
            all_tbl[all_num++] = i;
        }
    }
    if (all_num == 0) {
        condtbl[4] = false;
    }
    for (int i = 0; i < 6; ++i) {
        au->strtbl[i] = game_str_au_opts2[i];
    }
    au->strtbl[6] = 0;
    au->condtbl = condtbl;
    selected = ui_audience_ask4(au);
    switch (selected) {
        case 0:
            if (eh->relation1[pa] > 10) {
                si = game_audience_check_mood(au, 50, 0);
            } else {
                si = 0;
            }
            if ((si == 1) || (si == 2)) {
                si = game_audience_sweeten(au, si);
            }
            if (si == 3) {
                game_diplo_set_treaty(g, ph, pa, TREATY_NONAGGRESSION);
            }
            dtype = 62;
            break;
        case 1:
            if (eh->relation1[pa] > 50) {
                si = game_audience_check_mood(au, 125, 0);
            } else {
                si = 0;
            }
            if ((si == 1) || (si == 2)) {
                si = game_audience_sweeten(au, si);
            }
            if (si == 3) {
                game_diplo_set_treaty(g, ph, pa, TREATY_ALLIANCE);
            }
            dtype = 63;
            break;
        case 2:
            si = game_audience_check_mood(au, 60, 0); /* FIXME BUG? should be 2 for mood_peace? */
            if ((si == 1) || (si == 2)) {
                si = game_audience_sweeten(au, si);
            }
            if (si == 3) {
                game_diplo_stop_war(g, ph, pa);
            }
            game_diplo_annoy(g, ph, pa, 2);
            dtype = 65;
            break;
        case 3:
            dtype = 67;
            au->pstartwar = audience_menu_race(au, war_tbl, war_num, game_str_au_whowar);
            if (au->pstartwar != PLAYER_NONE) {
                if ((eh->treaty[pa] == TREATY_ALLIANCE) && (eh->treaty[au->pstartwar] == TREATY_WAR)) {
                    si = (!rnd_0_nm1(8, &g->seed)) ? 2 : 3;
                } else {
                    si = game_audience_check_mood(au, ea->relation1[au->pstartwar] + 150, 0);
                }
                if ((si == 1) || (si == 2)) {
                    si = game_audience_sweeten(au, si);
                }
                if (si == 3) {
                    game_diplo_start_war(g, pa, au->pstartwar);
                }
            } else {
                selected = -1;
            }
            break;
        case 4:
            dtype = 68;
            au->pwar = audience_menu_race(au, all_tbl, all_num, game_str_au_whobrk);
            if (au->pwar != PLAYER_NONE) {
                if (eh->relation1[pa] > 24) {
                    si = game_audience_check_mood(au, ea->relation1[au->pwar] + 175, 0);
                    if ((si == 1) || (si == 2)) {
                        si = game_audience_sweeten(au, si);
                    }
                    if (si == 3) {
                        game_diplo_break_treaty(g, pa, au->pwar);
                    }
                } else {
                    si = 2;
                }
            } else {
                selected = -1;
            }
            break;
        case 5:
        default:
            selected = -1;
            dtype = 0;
            si = 0;
            break;
    }
    if ((selected != -1) && (si != 1)) {
        game_audience_set_dtype(au, dtype, si);
    }
}

static void audience_menu_trade(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    int16_t selected = 0;
    char *cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
    for (int i = 0; i < AUDIENCE_BC_MAX; ++i) {
        int len;
        au->strtbl[i] = cbuf;
        len = sprintf(cbuf, "%s %i %s", game_str_au_bull, au->bctbl[i], game_str_au_bcpery);
        cbuf += len + 1;
    }
    au->strtbl[au->num_bc] = game_str_au_opts3[1];
    au->strtbl[au->num_bc + 1] = 0;
    au->condtbl = 0;
    selected = ui_audience_ask4(au);
    game_diplo_annoy(g, ph, pa, 1);
    eh->mood_trade[pa] -= rnd_1_n(30, &g->seed);
    if ((selected != -1) && (selected != au->num_bc)) {
        int si = game_audience_check_mood(au, 50, 1);
        if (si < 3) {
            si = 0;
        } else {
            game_diplo_set_trade(g, ph, pa, au->bctbl[selected]);
        }
        game_audience_set_dtype(au, 64, si);
    }
}

static void audience_menu_threat(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int16_t selected = 0;
    bool condtbl[5];
    uint8_t dtype = 0;
    for (int i = 0; i < TBLLEN(condtbl); ++i) {
        condtbl[i] = true;
    }
    strcpy(au->buf, game_str_au_youract);
    if (eh->treaty[pa] != TREATY_NONAGGRESSION) {
        condtbl[0] = false;
    }
    if (eh->treaty[pa] != TREATY_ALLIANCE) {
        condtbl[1] = false;
    }
    if (eh->trade_bc[pa] == 0) {
        condtbl[2] = false;
    }
    for (int i = 0; i < 5; ++i) {
        au->strtbl[i] = game_str_au_opts4[i];
    }
    au->strtbl[5] = 0;
    au->condtbl = condtbl;
    selected = ui_audience_ask4(au);
    switch (selected) {
        case 0:
        case 1:
            game_diplo_break_treaty(g, ph, pa);
            dtype = 73;
            selected = 0;
            break;
        case 2:
            game_diplo_break_trade(g, ph, pa);
            dtype = 73;
            selected = 0;
            break;
        case 3:
            {
                int v;
                selected = 0;
                v = rnd_1_n(200, &g->seed) + eh->mood_treaty[pa] / 2;
                v += game_diplo_tbl_reldiff[ea->trait1] * 2;
                if (ea->total_production_bc > 0) {
                    v += (eh->total_production_bc * 100) / ea->total_production_bc;
                } else {
                    v += 100;
                }
                /*6541d*/
                SUBSATT(eh->relation1[pa], rnd_1_n(15, &g->seed), -100);
                ea->relation1[ph] = eh->relation1[pa];
                eh->mood_treaty[pa] = -120;
                if (v < 170) {
                    if ((rnd_1_n(15, &g->seed) - game_diplo_tbl_reldiff[ea->trait1]) > rnd_1_n(100, &g->seed)) {
                        game_diplo_start_war(g, ph, pa);
                        dtype = 13;
                    } else {
                        dtype = 69;
                    }
                } else {
                    if (game_ai_fix_spy_hiding) {
                        ea->spymode_next[ph] = SPYMODE_HIDE;
                        ea->spymode[ph] = SPYMODE_HIDE;
                    } else {
                        eh->spymode_next[pa] = SPYMODE_HIDE;
                        eh->spymode[pa] = SPYMODE_HIDE;
                    }
                    g->evn.ceasefire[ph][pa] = rnd_1_n(15, &g->seed) + 5;
                    dtype = 70;
                    if (v >= 275) {
                        struct spy_esp_s s[1];
                        s->spy = ph;
                        s->target = pa;
                        if (game_spy_esp_sub1(g, s, 0, 1) > 0) {
                            au->tribute_field = s->tbl_field[0];
                            au->tribute_tech = s->tbl_tech2[0];
                            game_tech_get_new(g, ph, au->tribute_field, au->tribute_tech, TECHSOURCE_TRADE, pa, 0, false);   /* WASBUG? pa was 0 */
                        }
                    } else if (v >= 200) {
                        int bc;
                        bc = (((rnd_1_n(8, &g->seed) + 2) * g->year) / 25) * 25;
                        if (bc != 0) {
                            eh->reserve_bc += bc;
                            au->tribute_bc = bc;
                            dtype = 71;
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
    game_diplo_annoy(g, ph, pa, 10);    /* FIXME BUG? MOO1 does this before the if, annoying by only entering the menu */
    if ((selected != -1) && (selected != 4)) {
        game_audience_set_dtype(au, dtype, 3);
    }
}

static void audience_menu_tribute(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int16_t selected = 0;
    uint8_t bcnum;
    uint32_t reserve;
    uint16_t bctbl[4];
    reserve = eh->reserve_bc;
    char *cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
    SETMIN(reserve, 32000);
    reserve = ((reserve) / 25) * 25;
    if (reserve < 100) {
        bcnum = reserve / 25;
        bctbl[0] = 25;
        bctbl[1] = 50;
        bctbl[2] = 75;
        bctbl[3] = 100;
    } else {
        bcnum = 4;
        bctbl[0] = ((reserve / 4) / 25) * 25;
        bctbl[1] = ((reserve / 2) / 25) * 25;
        bctbl[2] = (((3 * reserve) / 4) / 25) * 25;
        bctbl[3] = reserve;
    }
    strcpy(au->buf, game_str_au_whattr);
    for (int i = 0; i < bcnum; ++i) {
        int len;
        au->strtbl[i] = cbuf;
        len = sprintf(cbuf, "%s %i %s", game_str_au_bull, bctbl[i], game_str_bc);
        cbuf += len + 1;
    }
    au->strtbl[bcnum] = game_str_au_techn;
    au->strtbl[bcnum + 1] = 0;
    au->condtbl = 0;
    selected = ui_audience_ask4(au);
    if (selected == -1) {
        return;
    }
    if (selected < bcnum) {
        int v;
        if (ea->total_production_bc != 0) {
            /* FIXME BUG MOO1 uses the value
                  (reserve * 12) / (0x786e + 0xdd4 * pa + 0x326b0000)
               == (reserve * 12) / ea->relation1
               == 0
               Maybe the divisor was supposed to be eh->relation1[pa]? Or ea->total_production_bc?
               This makes money tributes quite useless.
            */
            v = 0;
        } else {
            v = 10;
        }
        v = ((selected + 1) * v) / 10;
        if (eh->race == RACE_HUMAN) {
            v *= 2;
        }
        eh->reserve_bc -= bctbl[selected];
        ea->reserve_bc += bctbl[selected];
        ADDSATT(eh->relation1[pa], v, 100);
        ea->relation1[ph] = eh->relation1[pa];
        if (eh->treaty[pa] >= TREATY_WAR) {
            SETMIN(ea->relation1[ph], -25);
        }
        if (eh->treaty[pa] != TREATY_ALLIANCE) {
            SETMIN(ea->relation1[ph], 65);
        }
        /* FIXME BUG? eh->relation1[pa] = ea->relation1[ph]; is missing */
        game_audience_set_dtype(au, 1, 3);
    } else {
        struct spy_esp_s s[1];
        int hmm1 = 0; /* FIXME BUG = diplo_p2_sub1_zhmm4[bcnum]; uninitialized, wrong index */
        s->spy = pa;
        s->target = ph;
        if (game_spy_esp_sub1(g, s, hmm1, 0) > 0) {
            int i;
            cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
            for (i = 0; (i < 4) && (i < s->tnum); ++i) {
                int len;
                au->strtbl[i] = cbuf;
                len = sprintf(cbuf, "%s ", game_str_au_bull);
                cbuf += len;
                game_tech_get_name(g->gaux, s->tbl_field[i], s->tbl_tech2[i], cbuf);
                cbuf += strlen(cbuf) + 1;
            }
            au->strtbl[i++] = game_str_au_opts3[1];
            au->strtbl[i] = 0;
            strcpy(au->buf, game_str_au_whattr);
            au->condtbl = 0;
            selected = ui_audience_ask4(au);
            if ((selected != -1) && (selected < s->tnum) && (selected < 4)) {
                int v;
                game_tech_get_new(g, pa, s->tbl_field[selected], s->tbl_tech2[selected], TECHSOURCE_TRADE, ph, 0, false);
                if (eh->relation1[pa] < 0) {
                    v = 20;
                } else {
                    v = (100 - eh->relation1[pa]) / 10;
                }
                v = ((rnd_1_n(8, &g->seed) + rnd_1_n(8, &g->seed) + (s->tbl_tech2[selected] / 4)) * v) / 10;
                if (eh->race == RACE_HUMAN) {
                    v *= 2;
                }
                ADDSATT(eh->relation1[pa], v, 100);
                ea->relation1[ph] = eh->relation1[pa];
                ADDSATT(eh->mood_peace[pa], v, 200);
                ADDSATT(eh->trust[pa], rnd_1_n(8, &g->seed) + 2, 30);
                eh->tribute_field[pa] = s->tbl_field[selected];
                eh->tribute_tech[pa] = s->tbl_tech2[selected];
                if (eh->treaty[pa] >= TREATY_WAR) {
                    SETMIN(ea->relation1[ph], -25);
                }
                if (eh->treaty[pa] != TREATY_ALLIANCE) {
                    SETMIN(ea->relation1[ph], 70);
                }
                /* FIXME BUG? eh->relation1[pa] = ea->relation1[ph]; is missing */
                game_audience_set_dtype(au, 1, 3);
            }
        } else {
            game_audience_set_dtype(au, 75, 3);
        }
    }
}

static void audience_menu_tech(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int v, di;
    di = eh->mood_tech[pa];
    if (di > 0) {
        di /= 5;
    }
    SETMIN(di, 30);
    if (eh->treaty[pa] == TREATY_ALLIANCE) {
        di += 25;
    }
    v = eh->trust[pa] + eh->relation1[pa] / 2 + ((eh->race == RACE_HUMAN) ? 50 : 0) + game_diplo_tbl_reldiff[ea->trait1] + rnd_1_n(100, &g->seed) - 125;
    if (v < 0) {
        v = abs(v) + 100;
    } else {
        v = 20000 / (v + 200);
    }
    v /= 4;
    SETMAX(v, 75);
    {
        struct spy_esp_s s[1];
        tech_field_t taf[TECH_SPY_MAX]; /* diplo_p2_sub1_field */
        uint8_t tat[TECH_SPY_MAX];
        int tav[TECH_SPY_MAX];
        tech_field_t thf[TECH_SPY_MAX][TECH_SPY_MAX];
        uint8_t tht[TECH_SPY_MAX][TECH_SPY_MAX];
        tech_field_t thaf[TECH_SPY_MAX * TECH_SPY_MAX];
        uint8_t that[TECH_SPY_MAX * TECH_SPY_MAX];
        int tanum, thnum[TECH_SPY_MAX], total_thnum;
        s->spy = ph;
        s->target = pa;
        if (game_spy_esp_sub1(g, s, 0, 1) > 0) {
            tanum = s->tnum;
            for (int i = 0; i < tanum; ++i) {
                taf[i] = s->tbl_field[i];
                tat[i] = s->tbl_tech2[i];
                tav[i] = (s->tbl_value[i] * v) / 100;
            }
            s->spy = pa;
            s->target = ph;
            total_thnum = 0;
            for (int i = 0; i < tanum; ++i) {
                if (game_spy_esp_sub1(g, s, tav[i], 0) > 0) {
                    int n;
                    n = s->tnum;
                    thnum[total_thnum] = n;
                    for (int j = 0; j < n; ++j) {
                        thf[total_thnum][j] = s->tbl_field[j];
                        tht[total_thnum][j] = s->tbl_tech2[j];
                    }
                    thaf[total_thnum] = taf[i];
                    that[total_thnum] = tat[i];
                    ++total_thnum;
                }
            }
            /*6568e*/
            if (total_thnum > 0) {
                int16_t selected = 0;
                int i;
                char *cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
                game_diplo_annoy(g, ph, pa, 1);
                eh->mood_tech[pa] -= rnd_1_n(50, &g->seed) + 20;
                for (i = 0; (i < 5) && (i < total_thnum); ++i) {
                    int len;
                    au->strtbl[i] = cbuf;
                    len = sprintf(cbuf, "%s ", game_str_au_bull);
                    cbuf += len;
                    game_tech_get_name(g->gaux, thaf[i], that[i], cbuf);
                    cbuf += strlen(cbuf) + 1;
                }
                /*65724*/
                au->strtbl[i] = 0;
                strcpy(au->buf, game_str_au_whatech);
                au->condtbl = 0;
                selected = ui_audience_ask4(au);
                if (selected != -1) {
                    tech_field_t gotf = thaf[selected];
                    uint8_t gott = that[selected];
                    int selected2 = selected, n = thnum[selected];
                    cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
                    for (i = 0; (i < 4) && (i < n); ++i) {
                        int len;
                        au->strtbl[i] = cbuf;
                        len = sprintf(cbuf, "%s ", game_str_au_bull);
                        cbuf += len;
                        game_tech_get_name(g->gaux, thf[selected2][i], tht[selected2][i], cbuf);
                        cbuf += strlen(cbuf) + 1;
                    }
                    au->strtbl[i] = game_str_au_opts3[1];
                    au->strtbl[i + 1] = 0;
                    strcpy(au->buf, game_str_au_whatrad);
                    au->condtbl = 0;
                    selected = ui_audience_ask4(au);
                    if ((selected != -1) && (selected < i)) {
                        g->evn.newtech[ph].num = 0;
                        game_tech_get_new(g, ph, gotf, gott, TECHSOURCE_TRADE, pa, 0, false);
                        game_tech_get_new(g, pa, thf[selected2][selected], tht[selected2][selected], TECHSOURCE_TRADE, pa, 0, false); /* FIXME BUG? last pa should be ph? */
                        if (g->evn.newtech[ph].num != 0) {
                            ui_audience_newtech(au);
                        }
                    }
                }
            } else {
                game_audience_set_dtype(au, 75, 3);
            }
        } else {
            game_audience_set_dtype(au, 75, 3);
        }
    }
    game_diplo_annoy(g, ph, pa, 3);
    SETMIN(eh->mood_tech[pa], 50);
}

static void audience_menu_main(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    bool flag_done = false;
    bool condtbl[6];
    while (!flag_done) {
        int16_t selected;
        for (int i = 0; i < TBLLEN(condtbl); ++i) {
            condtbl[i] = true;
        }
        if (eh->treaty[pa] >= TREATY_WAR) {
            condtbl[1] = false;
            condtbl[2] = false;
            condtbl[4] = false;
        }
        {
            struct spy_esp_s s[1];
            s->spy = pa;
            s->target = ph;
            if ((eh->reserve_bc < 25) && (game_spy_esp_sub1(g, s, 0, 0) == 0)) {
                condtbl[3] = false;
            }
        }
        {
            int prod, want_trade, cur_trade = eh->trade_bc[pa];
            prod = MIN(eh->total_production_bc, g->eto[pa].total_production_bc) / 4;
            SETMIN(prod, 32000);
            want_trade = (prod / 25) * 25 - cur_trade;
            if (want_trade <= 0) {
                condtbl[1] = false;
            } else if (want_trade < 125) {
                au->num_bc = want_trade / 25;
                au->bctbl[0] = cur_trade + 25;
                au->bctbl[1] = cur_trade + 50;
                au->bctbl[2] = cur_trade + 75;
                au->bctbl[3] = cur_trade + 100;
            } else {
                au->num_bc = AUDIENCE_BC_MAX;
                au->bctbl[0] = want_trade / 5 + cur_trade;
                au->bctbl[1] = (((want_trade * 2) / 5) / 25) * 25 + cur_trade;
                au->bctbl[2] = (((want_trade * 3) / 5) / 25) * 25 + cur_trade;
                au->bctbl[3] = (((want_trade * 4) / 5) / 25) * 25 + cur_trade;
                au->bctbl[4] = want_trade + cur_trade;
            }
        }
        if (game_diplo_get_mood(g, ph, pa) < -100) {
            game_audience_set_dtype(au, 74, 3);
            break;
        }
        strcpy(au->buf, game_str_au_howmay);
        for (int i = 0; i < 6; ++i) {
            au->strtbl[i] = game_str_au_opts1[i];
        }
        au->strtbl[6] = 0;
        au->condtbl = condtbl;
        selected = ui_audience_ask4(au);
        switch (selected) {
            case 0: /* Propose Treaty */
                audience_menu_treaty(au);
                break;
            case 1: /* Form Trade Agreement */
                audience_menu_trade(au);
                break;
            case 2: /* Threaten/Break Treaty and Trade */
                audience_menu_threat(au);
                break;
            case 3: /* Offer Tribute */
                audience_menu_tribute(au);
                break;
            case 4: /* Exchange Technology */
                audience_menu_tech(au);
                break;
            default:
            case -1:
            case 5: /* Good Bye */
                flag_done = true;
                break;
        }
    }
}

static void game_audience_do(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int16_t selected;
    if ((au->mode >= 0) && (au->mode <= 2)) {
        g->gaux->diplo_d0_rval = -1;
        game_audience_get_str1(au);
        ui_audience_show1(au);
    }
    switch (au->mode) {
        case 2:
            selected = 1;  /* FIXME BUG? used below for dtype == 76 if !game_audience_sub2() */
            if (game_audience_sub2(au)) {
                au->dtype = au->dtype_next;
                g->gaux->diplo_d0_rval = -1;
                if ((selected = game_audience_sub3(au)) == 0) {
                    if (au->dtype == 24) {
                        game_diplo_set_treaty(g, ph, pa, TREATY_NONAGGRESSION);
                    } else if (au->dtype == 25) {
                        game_diplo_set_treaty(g, ph, pa, TREATY_ALLIANCE);
                    } else if (au->dtype == 26) {
                        game_diplo_set_trade(g, ph, pa, eh->au_want_trade[pa]);
                    } else if (au->dtype == 30) {
                        game_diplo_stop_war(g, ph, pa);
                        if (eh->relation1[pa] < 80) {
                            eh->relation1[pa] += 20;
                            ea->relation1[ph] = eh->relation1[pa];
                        }
                        game_diplo_annoy(g, ph, pa, 2);
                    } else if (au->dtype == 76) {
                        game_diplo_start_war(g, ph, au->pwar);
                    }
                    if ((au->dtype == 24) || (au->dtype == 25) || (au->dtype == 26) || (au->dtype == 30)) { /* FIXME 76? */
                        eh->reserve_bc += eh->offer_bc[pa];
                        if (eh->offer_tech[pa] != 0) {
                            game_tech_get_new(g, ph, eh->offer_field[pa], eh->offer_tech[pa], TECHSOURCE_TRADE, pa, 0, false);
                        }
                    }
                } else {
                    /*6074c*/
                    if (au->dtype == 76) {
                        game_diplo_break_treaty(g, pa, ph);
                    }
                }
                /*60761*/
                if (au->dtype == 29) {
                    game_tech_get_new(g, ph, eh->au_tech_trade_field[pa][selected], eh->au_tech_trade_tech[pa][selected], TECHSOURCE_TRADE, pa, 0, false);
                }
            }
            /*607a9*/
            game_diplo_annoy(g, ph, pa, 1);
            if ((au->dtype == 24) || (au->dtype == 25)) {
                eh->mood_treaty[pa] -= rnd_1_n(30, &g->seed) + 20;
            }
            if (au->dtype == 26) {
                eh->mood_trade[pa] -= rnd_1_n(30, &g->seed) + 20;
            }
            if (au->dtype == 30) {
                eh->mood_peace[pa] -= rnd_1_n(50, &g->seed) + 50;
            }
            if (au->dtype == 29) {
                eh->mood_tech[pa] -= rnd_1_n(30, &g->seed) + 20;
            }
            if (au->dtype == 76) {
                au->dtype = (selected != 0) ? 77 : 78;
                au->mode = 6;
                game_audience_set_dtype(au, au->dtype, 3);
            }
            break;
        case 6:
            game_audience_set_dtype(au, au->dtype, 3);
            break;
        case 0:
            audience_menu_main(au);
            break;
        case 1:
            break;
        default:
            break;
    }
}

/* -------------------------------------------------------------------------- */

void game_audience(struct game_s *g, player_id_t ph, player_id_t pa)
{
    struct audience_s au[1];
    empiretechorbit_t *eh = &(g->eto[ph]);
    au->g = g;
    game_diplo_limit_mood_treaty(g);
    game_audience_prepare(au, ph, pa);
    ui_audience_start(au);
    au->mode = 6;
    au->dtype = eh->diplo_type[pa];
    if ((au->dtype == 66) || (au->dtype == 1)) {
        /* FIXME BUG? no ui_audience_end call after _start */
        ui_audience_end(au);
        return;
    }
    if (au->dtype == 0) {
        game_audience_start_human(au);
    }
    g->gaux->diplo_d0_rval = -1;
    if (((au->dtype >= 24) && (au->dtype <= 30)) || (au->dtype == 58) || (au->dtype == 76)) {
        au->dtype_next = au->dtype;
        au->dtype = 22;
        au->mode = 2;
        au->pwar = eh->au_ally_attacker[pa];
    }
    game_audience_do(au);
    ui_audience_end(au);
    if (au->dtype == 32) {
        game_diplo_break_treaty(g, pa, ph);
    }
}
