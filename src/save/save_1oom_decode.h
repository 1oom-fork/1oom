#ifndef INC_1OOM_SAVE_1OOM_DECODE_H
#define INC_1OOM_SAVE_1OOM_DECODE_H

#include "bits.h"
#include "types.h"
#include "game_types.h"
#include "save_1oom.h"

/* -------------------------------------------------------------------------- */

#define SG_1OOM_DE_DUMMY(_n_)  pos += (_n_)
#define SG_1OOM_DE_U8(_v_)  (_v_) = buf[pos++]
#define SG_1OOM_DE_U16(_v_)  (_v_) = GET_LE_16(&buf[pos]), pos += 2
#define SG_1OOM_DE_U32(_v_)  (_v_) = GET_LE_32(&buf[pos]), pos += 4
#define SG_1OOM_DE_BV(_v_, _n_)  do { int l = ((_n_) + 7) / 8; memcpy((_v_), &buf[pos], l); pos += l; } while (0)
#define SG_1OOM_DE_TBL_U8(_v_, _n_)  do { for (int i_ = 0; i_ < (_n_); ++i_) { (_v_)[i_] = buf[pos]; ++pos; } } while (0)
#define SG_1OOM_DE_TBL_U16(_v_, _n_)  do { for (int i_ = 0; i_ < (_n_); ++i_) { (_v_)[i_] = GET_LE_16(&buf[pos]); pos += 2; } } while (0)
#define SG_1OOM_DE_TBL_U32(_v_, _n_)  do { for (int i_ = 0; i_ < (_n_); ++i_) { (_v_)[i_] = GET_LE_32(&buf[pos]); pos += 4; } } while (0)
#define SG_1OOM_DE_TBL_TBL_U8(_v_, _no_, _ni_)  do { for (int o_ = 0; o_ < (_no_); ++o_) { for (int i_ = 0; i_ < (_ni_); ++i_) { (_v_)[o_][i_] = buf[pos++]; } } } while (0)
#define SG_1OOM_DE_TBL_TBL_U16(_v_, _no_, _ni_)  do { for (int o_ = 0; o_ < (_no_); ++o_) { for (int i_ = 0; i_ < (_ni_); ++i_) { (_v_)[o_][i_] = GET_LE_16(&buf[pos]); pos += 2; } } } while (0)
#define SG_1OOM_DE_TBL_TBL_U32(_v_, _no_, _ni_)  do { for (int o_ = 0; o_ < (_no_); ++o_) { for (int i_ = 0; i_ < (_ni_); ++i_) { (_v_)[o_][i_] = GET_LE_32(&buf[pos]); pos += 4; } } } while (0)

#define SG_1OOM_DE_DUMMY_V0(_n_)  if (version == 0) { SG_1OOM_DE_DUMMY(_n_); }

static int libsave_1oom_decode_player_id(const uint8_t *buf, int pos, player_id_t *player, uint32_t version)
{
    if (version == 0) {
        uint8_t tmp;
        SG_1OOM_DE_U8(tmp);
        if (tmp == 6) {
            *player = PLAYER_NONE;
        } else {
            *player = (player_id_t)tmp;
        }
    } else {
        uint16_t tmp;
        SG_1OOM_DE_U16(tmp);
        *player = (int16_t)tmp;
    }
    return pos;
}

#define SG_1OOM_DE_PLAYER_ID(_v_)  do { pos = libsave_1oom_decode_player_id(buf, pos, &(_v_), version); } while (0)

static int libsave_1oom_decode_hated_id(const uint8_t *buf, int pos, player_id_t *player, uint32_t version)
{
    uint16_t tmp;
    SG_1OOM_DE_U16(tmp);
    if (version == 0) {
        if (tmp == 6) {
            *player = PLAYER_NONE;
        } else {
            *player = (player_id_t)tmp;
        }
    } else {
        *player = (int16_t)tmp;
    }
    return pos;
}

#define SG_1OOM_DE_HATED_ID(_v_)  do { pos = libsave_1oom_decode_hated_id(buf, pos, &(_v_), version); } while (0)

static int libsave_1oom_decode_planet_id(const uint8_t *buf, int pos, planet_id_t *pli, uint32_t version)
{
    if (version == 0) {
        uint8_t tmp;
        SG_1OOM_DE_U8(tmp);
        *pli = (int8_t)(tmp);
    } else {
        uint16_t tmp;
        SG_1OOM_DE_U16(tmp);
        *pli = (int16_t)tmp;
    }
    return pos;
}

#define SG_1OOM_DE_PLANET_ID(_v_)  do { pos = libsave_1oom_decode_planet_id(buf, pos, &(_v_), version); } while (0)

static int libsave_1oom_decode_shipdesign_id(const uint8_t *buf, int pos, shipdesign_id_t *sdi, uint32_t version)
{
    if (version < 2) {
        uint8_t tmp;
        SG_1OOM_DE_U8(tmp);
        *sdi = (int8_t)(tmp);
    } else {
        uint16_t tmp;
        SG_1OOM_DE_U16(tmp);
        *sdi = (int16_t)(tmp);
    }
    return pos;
}

#define SG_1OOM_DE_SHIPDESIGN_ID(_v_)  do { pos = libsave_1oom_decode_shipdesign_id(buf, pos, &(_v_), version); } while (0)

#endif
