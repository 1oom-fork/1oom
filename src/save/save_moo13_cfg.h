#ifndef INC_SAVE_MOO13_CFG_H
#define INC_SAVE_MOO13_CFG_H

typedef enum {
    SAVE_CMOO_OFFS_MUSIC_CARD = 0x0,
    SAVE_CMOO_OFFS_MUSIC_PORT = 0x2,
    SAVE_CMOO_OFFS_MUSIC_IRQ = 0x4,
    SAVE_CMOO_OFFS_MUSIC_6 = 0x6,

    SAVE_CMOO_OFFS_SFX_CARD = 0x8,
    SAVE_CMOO_OFFS_SFX_PORT = 0xa,
    SAVE_CMOO_OFFS_SFX_IRQ = 0xc,
    SAVE_CMOO_OFFS_SFX_DMA = 0xe,

    SAVE_CMOO_OFFS_SOUND_MODE = 0x10,
    SAVE_CMOO_OFFS_USE_MOUSE = 0x12,
    SAVE_CMOO_OFFS_GAME_OPTIONS = 0x14,

    SAVE_CMOO_OFFS_HAVE_SAVE1 = 0x16,
    SAVE_CMOO_OFFS_HAVE_SAVE2 = 0x18,
    SAVE_CMOO_OFFS_HAVE_SAVE3 = 0x1a,
    SAVE_CMOO_OFFS_HAVE_SAVE4 = 0x1c,
    SAVE_CMOO_OFFS_HAVE_SAVE5 = 0x1e,
    SAVE_CMOO_OFFS_HAVE_SAVE6 = 0x20,

    SAVE_CMOO_OFFS_SAVE1_NAME = 0x22,
    SAVE_CMOO_OFFS_SAVE2_NAME = 0x36,
    SAVE_CMOO_OFFS_SAVE3_NAME = 0x4a,
    SAVE_CMOO_OFFS_SAVE4_NAME = 0x5e,
    SAVE_CMOO_OFFS_SAVE5_NAME = 0x72,
    SAVE_CMOO_OFFS_SAVE6_NAME = 0x86,

    SAVE_CMOO_LEN = 0x9a
} save_cmoo_offs_t;

static inline save_cmoo_offs_t save_cmoo_havesave_offs(int i)
{
    if ((i < 0) || (i > 5)) {
        return SAVE_CMOO_LEN;
    }
    return SAVE_CMOO_OFFS_HAVE_SAVE1 + i * 2;
}

static inline save_cmoo_offs_t save_cmoo_savename_offs(int i)
{
    if ((i < 0) || (i > 5)) {
        return SAVE_CMOO_LEN;
    }
    return SAVE_CMOO_OFFS_SAVE1_NAME + i * 0x14;
}

#endif
