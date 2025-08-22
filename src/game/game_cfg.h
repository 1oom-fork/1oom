#ifndef INC_GAME_CFG_H
#define INC_GAME_CFG_H

#include "types.h"

typedef enum {
    CMOO_OFFS_MUSIC_CARD = 0x0,
    CMOO_OFFS_MUSIC_PORT = 0x2,
    CMOO_OFFS_MUSIC_IRQ = 0x4,
    CMOO_OFFS_MUSIC_6 = 0x6,

    CMOO_OFFS_SFX_CARD = 0x8,
    CMOO_OFFS_SFX_PORT = 0xa,
    CMOO_OFFS_SFX_IRQ = 0xc,
    CMOO_OFFS_SFX_DMA = 0xe,

    CMOO_OFFS_SOUND_MODE = 0x10,
    CMOO_OFFS_USE_MOUSE = 0x12,
    CMOO_OFFS_GAME_OPTIONS = 0x14,

    CMOO_OFFS_HAVE_SAVE1 = 0x16,
    CMOO_OFFS_HAVE_SAVE2 = 0x18,
    CMOO_OFFS_HAVE_SAVE3 = 0x1a,
    CMOO_OFFS_HAVE_SAVE4 = 0x1c,
    CMOO_OFFS_HAVE_SAVE5 = 0x1e,
    CMOO_OFFS_HAVE_SAVE6 = 0x20,

    CMOO_OFFS_SAVE1_NAME = 0x22,
    CMOO_OFFS_SAVE2_NAME = 0x36,
    CMOO_OFFS_SAVE3_NAME = 0x4a,
    CMOO_OFFS_SAVE4_NAME = 0x5e,
    CMOO_OFFS_SAVE5_NAME = 0x72,
    CMOO_OFFS_SAVE6_NAME = 0x86,

    CMOO_LEN = 0x9a
} cmoo_offs_t;

static inline cmoo_offs_t cmoo_havesave_offs(int i)
{
    if ((i < 0) || (i > 5)) {
        return CMOO_LEN;
    }
    return CMOO_OFFS_HAVE_SAVE1 + i * 2;
}

static inline cmoo_offs_t cmoo_savename_offs(int i)
{
    if ((i < 0) || (i > 5)) {
        return CMOO_LEN;
    }
    return CMOO_OFFS_SAVE1_NAME + i * 0x14;
}

extern uint8_t *cmoo_buf;

extern int game_cfg_init(void);
extern int game_cfg_load(void);
extern int game_cfg_write(void);
extern void game_cfg_shutdown(void);

#endif
