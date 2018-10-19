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
#include "game_misc.h"
#include "game_num.h"
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
    player_id_t pa = au->pa;
    if (IS_AI(g, pa)) {
        game_ai->aud_start_human(au);
    } else {
        /* TODO multiplayer */
        au->dtype = 22;
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
    if (g->evn.diplo_msg_subtype == -1) {
        const char *msg;
        char *buf = au->buf;
        {
            int v = rnd_0_nm1(g->gaux->diplomat.d0[dtype], &g->seed);
            if (g->players == 2) {
                --v;
                SETMAX(v, 0);
            }
            g->evn.diplo_msg_subtype = v;
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
                        s = (strchr("aeiou", tolower(game_str_tbl_race[ea->race][0])) != NULL) ? "n" : "";
                        break;
                    case 2:
                        s = (!eh->diplo_p2[pa]) ? game_str_au_facts : game_str_au_bases;
                        break;
                    case 0xb:
                        if (eh->treaty[pa] == TREATY_ALLIANCE) {
                            s = game_str_au_allian;
                        } else if (eh->treaty[pa] == TREATY_NONAGGRESSION) {
                            s = game_str_au_nonagg;
                        } else if (eh->trade_bc != 0) {
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
                        s = game_str_tbl_race[g->eto[eh->attack_bounty[pa]].race];
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
                        if (eh->attack_gift_bc[pa] != 0) {
                            len = sprintf(buf, "%i %s.", eh->attack_gift_bc[pa], game_str_bc);
                        } else {
                            len = game_audience_print_tech(g, eh->attack_gift_field[pa], eh->attack_gift_tech[pa], buf, true);
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
        case 28:
        case 58:
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
    g->evn.diplo_msg_subtype = -1;
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
    if (IS_AI(g, au->pa)) {
        dtype = game_ai->aud_get_dtype(au, dtype, a2);
    } else {
        /* TODO */
        if ((a2 >= 0) && (a2 <= 2)) {
            dtype = 31;
        }
    }
    au->dtype = dtype;
    g->evn.diplo_msg_subtype = -1;
    game_audience_get_str1(au);
    ui_audience_show3(au);
}

typedef enum {
    AUD_MP_ASK_NAP,
    AUD_MP_ASK_ALLIANCE,
    AUD_MP_ASK_PEACE,
    AUD_MP_ASK_WAR,
    AUD_MP_ASK_BREAK,
    AUD_MP_ASK_TRADE
} aud_mp_ask_t;

static int game_audience_mp_ask_accept(struct audience_s *au, aud_mp_ask_t atype)
{
    struct game_s *g = au->g;
    player_id_t pa = au->pa;
    char *buf = au->buf;
    int16_t selected = 0;
    g->evn.diplo_msg_subtype = -1;
    buf += sprintf(buf, "\x02(%s)\x01", g->emperor_names[pa]);
    switch (atype) {
        case AUD_MP_ASK_NAP:
            buf += sprintf(buf, " %s?", &game_str_au_opts2[0][2]);
            break;
        case AUD_MP_ASK_ALLIANCE:
            buf += sprintf(buf, " %s?", &game_str_au_opts2[1][2]);
            break;
        case AUD_MP_ASK_PEACE:
            buf += sprintf(buf, " %s?", &game_str_au_opts2[2][2]);
            break;
        case AUD_MP_ASK_WAR:
            /* TODO */
            break;
        case AUD_MP_ASK_BREAK:
            /* TODO */
            break;
        case AUD_MP_ASK_TRADE:
            buf += sprintf(buf, " %s %i %s?", game_str_au_tradea, au->new_trade_bc, game_str_au_bcpery);
            break;
    }
    for (int i = 0; i < 4; ++i) {
        au->strtbl[i] = game_str_au_optsmp1[i];
    }
    au->strtbl[4] = 0;
    if (atype == AUD_MP_ASK_TRADE) {
        au->strtbl[2] = 0;
    }
    au->condtbl = 0;
    selected = ui_audience_ask4(au);
    if ((selected == -1) || (selected == 1)) {
        return 0;
    } else if (selected == 0) {
        return 3;
    } else if (selected == 2) {
        return 2;
    } else {
        return 1;
    }
}

static int game_audiece_mp_sweeten_tech_field(struct audience_s *au, tech_field_t f)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    const empiretechorbit_t *eh = &(g->eto[ph]);
    const empiretechorbit_t *ea = &(g->eto[pa]);
    uint8_t num = 0, pos = 0;
    uint8_t techtbl[TECH_PER_FIELD + 2];
    {
        const shipresearch_t *srd = &(g->srd[ph]);
        const uint8_t *rct = &(srd->researchcompleted[f][0]);
        uint16_t tc = eh->tech.completed[f];
        uint8_t rf = ea->spyreportfield[ph][f];
        for (int i = 0; i < tc; ++i) {
            uint8_t rc;
            rc = rct[i];
            if (rc <= rf) {
                if (!game_tech_player_has_tech(g, f, rc, pa)) {
                    techtbl[num++] = rc;
                }
            }
        }
    }
    /* HACK */
    techtbl[num++] = 0xfe/*back*/;
    techtbl[num++] = 0xff/*forget*/;
    while (1) {
        int16_t selected;
        int i;
        char *cbuf;
        cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
        for (i = 0; (i < 5) && ((i + pos) < num); ++i) {
            char buf[0x60];
            int len;
            uint8_t v;
            au->strtbl[i] = cbuf;
            v = techtbl[i + pos];
            if (v == 0xfe/*back*/) {
                len = sprintf(cbuf, "%s", game_str_au_back);
            } else if (v == 0xff/*forget*/) {
                len = sprintf(cbuf, "%s", game_str_au_optsmp1[1]);
            } else {
                len = sprintf(cbuf, "%s %s", game_str_au_bull, game_tech_get_name(g->gaux, f, v, buf));
            }
            cbuf += len + 1;
        }
        if ((i + pos) < num) {
            au->strtbl[i++] = game_str_au_nextp;
        }
        au->strtbl[i] = 0;
        au->condtbl = 0;
        selected = ui_audience_ask4(au);
        if (selected < 0) {
            return -1;
        } else {
            uint8_t v = techtbl[selected + pos];
            if (v == 0xfe/*back*/) {
                return 0;
            } else if (v == 0xff/*forget*/) {
                return -1;
            } else {
                return v;
            }
        }
    }
}

static bool game_audiece_mp_sweeten_tech(struct audience_s *au, tech_field_t *fieldptr, uint8_t *techptr)
{
    struct game_s *g = au->g;
    player_id_t pa = au->pa;
    g->evn.diplo_msg_subtype = -1;
    sprintf(au->buf, "\x02(%s)\x01", g->emperor_names[pa]);
    while (1) {
        int16_t selected;
        char *cbuf;
        cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
        for (tech_field_t f = 0; f < TECH_FIELD_NUM; ++f) {
            int len;
            au->strtbl[f] = cbuf;
            len = sprintf(cbuf, "%s %s", game_str_au_bull, game_str_tbl_te_field[f]);
            cbuf += len + 1;
        }
        au->strtbl[TECH_FIELD_NUM] = 0;
        au->condtbl = 0;
        selected = ui_audience_ask4(au);
        if (selected == -1) {
            return false;
        } else {
            int res;
            res = game_audiece_mp_sweeten_tech_field(au, selected);
            if (res > 0) {
                *fieldptr = selected;
                *techptr = res;
                return true;
            } else if (res < 0) {
                return false;
            }
        }
    }
}

static bool game_audiece_mp_sweeten_bc(struct audience_s *au, int *bcptr)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    int16_t selected = 0;
    uint8_t bcnum;
    uint32_t reserve = eh->reserve_bc;
    uint16_t bctbl[5];
    char *cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
    SETMIN(reserve, game_num_max_tribute_bc);
    reserve = ((reserve) / 20) * 20;
    if (reserve < 100) {
        bcnum = reserve / 20;
        bctbl[0] = 20;
        bctbl[1] = 40;
        bctbl[2] = 60;
        bctbl[3] = 80;
        bctbl[4] = 100;
    } else {
        bcnum = 5;
        bctbl[0] = ((reserve / 5) / 20) * 20;
        bctbl[1] = (((2 * reserve) / 5) / 20) * 20;
        bctbl[2] = (((3 * reserve) / 5) / 20) * 20;
        bctbl[3] = (((4 * reserve) / 5) / 20) * 20;
        bctbl[4] = reserve;
    }
    g->evn.diplo_msg_subtype = -1;
    sprintf(au->buf, "\x02(%s)\x01", g->emperor_names[pa]);
    for (int i = 0; i < bcnum; ++i) {
        int len;
        au->strtbl[i] = cbuf;
        len = sprintf(cbuf, "%s %i %s", game_str_au_bull, bctbl[i], game_str_bc);
        cbuf += len + 1;
    }
    au->strtbl[bcnum] = game_str_au_optsmp1[1];
    au->strtbl[bcnum + 1] = 0;
    au->condtbl = 0;
    selected = ui_audience_ask4(au);
    if ((selected >= 0) && (selected < bcnum)) {
        *bcptr = bctbl[selected];
        return true;
    }
    return false;
}

static int game_audience_sweeten(struct audience_s *au, int a0)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
    empiretechorbit_t *ea = &(g->eto[pa]);
    int bc = 0;
    tech_field_t field = 0;
    uint8_t tech = 0;
    char buf[0x60];
    bool flag_bc;
    int16_t selected = 0;
    if (IS_AI(g, pa)) {
        if (!game_ai->aud_sweeten(au, &bc, &field, &tech)) {
            return 0;
        }
    } else {
        if (a0 == 1) {
            if (!game_audiece_mp_sweeten_tech(au, &field, &tech)) {
                return 0;
            }
        } else {
            if (!game_audiece_mp_sweeten_bc(au, &bc)) {
                return 0;
            }
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
        game_tech_get_new(g, pa, field, tech, TECHSOURCE_TRADE, ph, PLAYER_NONE, false);
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
        if ((i != ph) && (i != pa) && ((ea->treaty[i] == TREATY_ALLIANCE) || (game_num_aud_ask_break_nap && (ea->treaty[i] == TREATY_NONAGGRESSION)))) {
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
        case 0: /* Non-Aggression Pact */
            if (IS_AI(g, pa)) {
                si = game_ai->aud_treaty_nap(au);
            } else {
                si = game_audience_mp_ask_accept(au, AUD_MP_ASK_NAP);
            }
            if ((si == 1) || (si == 2)) {
                si = game_audience_sweeten(au, si);
            }
            if (si == 3) {
                game_diplo_set_treaty(g, ph, pa, TREATY_NONAGGRESSION);
            }
            dtype = 62;
            break;
        case 1: /* Alliance */
            if (IS_AI(g, pa)) {
                si = game_ai->aud_treaty_alliance(au);
            } else {
                si = game_audience_mp_ask_accept(au, AUD_MP_ASK_ALLIANCE);
            }
            if ((si == 1) || (si == 2)) {
                si = game_audience_sweeten(au, si);
            }
            if (si == 3) {
                game_diplo_set_treaty(g, ph, pa, TREATY_ALLIANCE);
            }
            dtype = 63;
            break;
        case 2: /* Peace Treaty */
            if (IS_AI(g, pa)) {
                si = game_ai->aud_treaty_peace(au);
            } else {
                si = game_audience_mp_ask_accept(au, AUD_MP_ASK_PEACE);
            }
            if ((si == 1) || (si == 2)) {
                si = game_audience_sweeten(au, si);
            }
            if (si == 3) {
                game_diplo_stop_war(g, ph, pa);
            }
            game_diplo_annoy(g, ph, pa, 2);
            dtype = 65;
            break;
        case 3: /* Declaration of War on Another Race */
            dtype = 67;
            au->pstartwar = audience_menu_race(au, war_tbl, war_num, game_str_au_whowar);
            if (au->pstartwar != PLAYER_NONE) {
                if (IS_AI(g, pa)) {
                    si = game_ai->aud_treaty_declare_war(au);
                } else {
                    si = game_audience_mp_ask_accept(au, AUD_MP_ASK_WAR);
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
        case 4: /* Break Alliance With Another Race */
            dtype = 68;
            au->pwar = audience_menu_race(au, all_tbl, all_num, game_str_au_whobrk);
            if (au->pwar != PLAYER_NONE) {
                if (IS_AI(g, pa)) {
                    si = game_ai->aud_treaty_break_alliance(au);
                } else {
                    si = game_audience_mp_ask_accept(au, AUD_MP_ASK_BREAK);
                }
                if ((si == 1) || (si == 2)) {
                    si = game_audience_sweeten(au, si);
                }
                if (si == 3) {
                    game_diplo_break_treaty(g, pa, au->pwar);
                }
            } else {
                selected = -1;
            }
            break;
        case 5: /* Forget It */
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
        int si;
        au->new_trade_bc = au->bctbl[selected];
        if (IS_AI(g, pa)) {
            si = game_ai->aud_trade(au);
        } else {
            si = game_audience_mp_ask_accept(au, AUD_MP_ASK_TRADE);
        }
        if (si < 3) {
            si = 0;
        } else {
            game_diplo_set_trade(g, ph, pa, au->new_trade_bc);
        }
        game_audience_set_dtype(au, 64, si);
    }
}

static void audience_menu_threat(struct audience_s *au)
{
    struct game_s *g = au->g;
    player_id_t ph = au->ph, pa = au->pa;
    empiretechorbit_t *eh = &(g->eto[ph]);
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
        case 0: /* Break Non-Aggression Pact */
        case 1: /* Break Alliance */
            game_diplo_break_treaty(g, ph, pa);
            dtype = 73;
            selected = 0;
            break;
        case 2: /* Break Trade Agreement */
            game_diplo_break_trade(g, ph, pa);
            dtype = 73;
            selected = 0;
            break;
        case 3: /* Threaten To Attack */
            selected = 0;
            if (IS_AI(g, pa)) {
                dtype = game_ai->aud_threaten(au);
            } else {
                /* TODO */
                dtype = 73;
            }
            break;
        default:
            break;
    }
    if ((selected != -1) && (selected != 4)) {
        game_diplo_annoy(g, ph, pa, 10);    /* WASBUG MOO1 does this before the if, annoying by merely entering the menu */
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
    uint32_t reserve = eh->reserve_bc;
    uint16_t bctbl[4];
    char *cbuf = &(au->buf[AUDIENCE_CBUF_POS]);
    SETMIN(reserve, game_num_max_tribute_bc);
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
        eh->reserve_bc -= bctbl[selected];
        ea->reserve_bc += bctbl[selected];
        if (IS_AI(g, pa)) {
            game_ai->aud_tribute_bc(au, selected, bctbl[selected]);
        } else {
            /* TODO */
        }
        game_audience_set_dtype(au, 1, 3);
    } else {
        struct spy_esp_s s[1];
        s->spy = pa;
        s->target = ph;
        /* WASBUG
           MOO1 does game_spy_esp_sub1(g, s, tav[i], 0) where tav is a global table also used by audience_menu_tech and i is bcnum.
           If the tech menu is never visited or the table is not filled up to bcnum then the value is 0.
           Values larger than 0 only filter out tech of lesser worth, and who would not want to use those as bribes?
        */
        if (game_spy_esp_sub1(g, s, 0, 0) > 0) {
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
                game_tech_get_new(g, pa, s->tbl_field[selected], s->tbl_tech2[selected], TECHSOURCE_TRADE, ph, PLAYER_NONE, false);
                if (IS_AI(g, pa)) {
                    game_ai->aud_tribute_tech(au, selected, s->tbl_field[selected], s->tbl_tech2[selected]);
                } else {
                    game_tech_finish_new(au->g, pa);
                    ui_audience_newtech(au, pa);
                }
                eh->tribute_field[pa] = s->tbl_field[selected];
                eh->tribute_tech[pa] = s->tbl_tech2[selected];
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
    int v;
    if (IS_AI(g, pa)) {
        v = game_ai->aud_tech_scale(au);
    } else {
        /* TODO */
        v = 75;
    }
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
                        g->evn.newtech[pa].num = 0;
                        game_tech_get_new(g, ph, gotf, gott, TECHSOURCE_TRADE, pa, PLAYER_NONE, false);
                        game_tech_get_new(g, pa, thf[selected2][selected], tht[selected2][selected], TECHSOURCE_TRADE, ph, PLAYER_NONE, false); /* WASBUG last ph was pa */
                        if (g->evn.newtech[ph].num != 0) {
                            if (IS_HUMAN(g, pa) && (g->evn.newtech[pa].num != 0)) {
                                game_tech_finish_new(au->g, pa);
                                game_tech_finish_new(au->g, ph);
                                ui_audience_newtech(au, PLAYER_NONE);
                            } else {
                                game_tech_finish_new(au->g, ph);
                                ui_audience_newtech(au, ph);
                            }
                        } else if (IS_HUMAN(g, pa) && (g->evn.newtech[pa].num != 0)) {
                            game_tech_finish_new(au->g, pa);
                            ui_audience_newtech(au, pa);
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
        if (IS_AI(g, pa) && game_ai->aud_later(au)) {
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
        g->evn.diplo_msg_subtype = -1;
        game_audience_get_str1(au);
        ui_audience_show1(au);
    }
    switch (au->mode) {
        case 2:
            selected = 1;  /* FIXME BUG? used below for dtype == 76 if !game_audience_sub2() */
            if (game_audience_sub2(au)) {
                au->dtype = au->dtype_next;
                g->evn.diplo_msg_subtype = -1;
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
                            game_tech_get_new(g, ph, eh->offer_field[pa], eh->offer_tech[pa], TECHSOURCE_TRADE, pa, PLAYER_NONE, false);
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
                    game_tech_get_new(g, ph, eh->au_tech_trade_field[pa][selected], eh->au_tech_trade_tech[pa][selected], TECHSOURCE_TRADE, pa, PLAYER_NONE, false);
                }
                if ((au->dtype == 58) && game_num_aud_bounty_give) {    /* WASBUG MOO1 never gives the bounty */
                    eh->reserve_bc += eh->attack_gift_bc[pa];
                    if (eh->attack_gift_tech[pa] != 0) {
                        game_tech_get_new(g, ph, eh->attack_gift_field[pa], eh->attack_gift_tech[pa], TECHSOURCE_TRADE, pa, PLAYER_NONE, false);
                    }
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
    g->evn.diplo_msg_subtype = -1;
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
    /* WASBUG save/load does these */
    game_update_tech_util(g);
    game_update_within_range(g);
    game_update_visibility(g);
}
