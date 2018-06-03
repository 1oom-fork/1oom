#ifndef INC_1OOM_GAME_NEWS_H
#define INC_1OOM_GAME_NEWS_H

#include "game_types.h"
#include "types.h"

/* order in eventmsg.lbx */
typedef enum {
    GAME_NEWS_NONE = 0,
    GAME_NEWS_PLAGUE, /*1*/
    GAME_NEWS_QUAKE, /*2*/
    GAME_NEWS_NOVA, /*3*/
    GAME_NEWS_ACCIDENT, /*4*/
    GAME_NEWS_05,
    GAME_NEWS_06,
    GAME_NEWS_COMET, /*7*/
    GAME_NEWS_PIRATES, /*8*/
    GAME_NEWS_09,
    GAME_NEWS_REBELLION, /*10*/
    GAME_NEWS_CRYSTAL, /*11*/
    GAME_NEWS_AMOEBA, /*12*/
    GAME_NEWS_13,
    GAME_NEWS_14,
    GAME_NEWS_15,
    GAME_NEWS_16,
    GAME_NEWS_ORION, /*17*/
    GAME_NEWS_COUP, /*18*/
    GAME_NEWS_STARS, /*19*/
    GAME_NEWS_STATS, /*20*/
    GAME_NEWS_GENOCIDE, /*21*/
    GAME_NEWS_GUARDIAN
} news_type_t;

struct news_s {
    news_type_t type;
    int subtype;
    int statsnum;
    int num1;
    int num2;
    race_t race;
    const char *stats[PLAYER_NUM];
    uint8_t planet_i;
};

struct game_s;
extern void game_news_get_msg(const struct game_s *g, struct news_s *ns, char *buf);

#endif
