#include "config.h"

#include <stdio.h>
#include <string.h>

#include "game_news.h"
#include "comp.h"
#include "game.h"
#include "game_aux.h"
#include "game_stat.h"
#include "game_str.h"
#include "game_tech.h"
#include "log.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

void game_news_get_msg(const struct game_s *g, struct news_s *ns, char *buf)
{
    const char *msg = EVENTMSG_PTR(g->gaux, ns->type, ns->subtype);
    for (int i = 0; (i < EVENTMSG_LEN) && (msg[i] != 0); ++i) {
        char c;
        c = msg[i];
        if (c & 0x80) {
            int len;
            switch (c & 0x7f) {
                case 0:
                    len = sprintf(buf, "%s", g->planet[ns->planet_i].name);
                    buf += len;
                    break;
                case 1:
                    len = sprintf(buf, "%i", ns->num1);
                    buf += len;
                    break;
                case 2:
                    len = sprintf(buf, "%i", ns->num2);
                    buf += len;
                    break;
                case 3:
                    if (ns->num1 != 1) {
                        *buf++ = 's';
                    }
                    break;
                case 0xe:
                    if ((ns->num1 == 2) || (ns->num1 == 3)) {
                        *buf++ = 'n';
                    }
                    *buf++ = ' ';
                    break;
                case 0xd:
                    if (ns->num1 == 1) {
                        *buf++ = 's';
                    }
                    break;
                case 4:
                    len = sprintf(buf, "%s", game_str_tbl_race[ns->race]);
                    buf += len;
                    break;
                case 5:
                    len = sprintf(buf, "%s", game_str_tbl_race[g->eto[ns->num2].race]);
                    buf += len;
                    break;
                case 6:
                    len = sprintf(buf, "%s", game_str_tbl_te_field[ns->num1]);
                    buf += len;
                    break;
                case 7:
                    game_tech_get_name(g->gaux, ns->num2 / 50, ns->num2 % 50, buf);
                    len = strlen(buf);
                    buf += len;
                    break;
                case 8:
                    len = sprintf(buf, "%s", game_str_tbl_trait1[ns->num1]);
                    buf += len;
                    break;
                case 9:
                    len = sprintf(buf, "%s", game_str_tbl_trait2[ns->num2]);
                    buf += len;
                    break;
                case 0xc:
                    len = sprintf(buf, "%s", g->emperor_names[ns->num1]);
                    buf += len;
                    break;
                default:
                    *buf++ = c;
                    break;
            }
        } else {
            *buf++ = c;
        }
    }
    *buf = 0;
    if (ns->type == GAME_NEWS_STATS) {
        int num = 0;
        int tbl_player[PLAYER_NUM];
        int tbl_stat[PLAYER_NUM];
        for (int i = 0; i < PLAYER_NUM; ++i) {
            ns->stats[i] = 0;
        }
        for (int i = 0; i < g->players; ++i) {
            if (g->evn.home[i] != PLANET_NONE) {
                int v;
                tbl_player[num] = i;
                switch (ns->subtype) {
                    case 0:
                        v = game_stat_prod(g, i);
                        SETMAX(v, 1);
                        break;
                    case 1:
                        v = game_stat_pop(g, i);
                        SETMAX(v, 1);
                        break;
                    case 2:
                        v = game_stat_tech(g, i);
                        SETMAX(v, 1);
                        break;
                    case 3:
                        v = game_stat_fleet(g, i);
                        SETMAX(v, 1);
                        break;
                    default:
                        v = 0;
                        break;
                }
                tbl_stat[num] = v;
                if (v != 0) {
                    ++num;
                }
            }
        }
        ns->statsnum = num;
        for (int loops = 0; loops < num; ++loops) {
            for (int i = 0; i < (num - 1); ++i) {
                int v0, v1;
                v0 = tbl_stat[i];
                v1 = tbl_stat[i + 1];
                if (v0 < v1) {
                    tbl_stat[i] = v1;
                    tbl_stat[i + 1] = v0;
                    v0 = tbl_player[i]; tbl_player[i] = tbl_player[i + 1]; tbl_player[i + 1] = v0;
                }
            }
        }
        for (int i = 0; i < num; ++i) {
            ns->stats[i] = game_str_tbl_race[g->eto[tbl_player[i]].race];
        }
    } else {
        ns->statsnum = 0;
    }
}
