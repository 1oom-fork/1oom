#ifndef INC_1OOM_SAVE_1OOM_ENCODE_H
#define INC_1OOM_SAVE_1OOM_ENCODE_H

#include "bits.h"
#include "types.h"
#include "game_types.h"
#include "save_1oom.h"

/* -------------------------------------------------------------------------- */

#define SG_1OOM_EN_DUMMY(_n_)  memset(&buf[pos], 0, (_n_)), pos += (_n_)
#define SG_1OOM_EN_U8(_v_)  buf[pos++] = (_v_)
#define SG_1OOM_EN_U16(_v_)  SET_LE_16(&buf[pos], (_v_)), pos += 2
#define SG_1OOM_EN_U32(_v_)  SET_LE_32(&buf[pos], (_v_)), pos += 4
#define SG_1OOM_EN_BV(_v_, _n_)  do { int l = ((_n_) + 7) / 8; memcpy(&buf[pos], (_v_), l); pos += l; } while (0)
#define SG_1OOM_EN_TBL_U8(_v_, _n_)  do { for (int i_ = 0; i_ < (_n_); ++i_) { buf[pos] = (_v_)[i_]; ++pos; } } while (0)
#define SG_1OOM_EN_TBL_U16(_v_, _n_)  do { for (int i_ = 0; i_ < (_n_); ++i_) { SET_LE_16(&buf[pos], (_v_)[i_]); pos += 2; } } while (0)
#define SG_1OOM_EN_TBL_U32(_v_, _n_)  do { for (int i_ = 0; i_ < (_n_); ++i_) { SET_LE_32(&buf[pos], (_v_)[i_]); pos += 4; } } while (0)
#define SG_1OOM_EN_TBL_TBL_U8(_v_, _no_, _ni_)  do { for (int o_ = 0; o_ < (_no_); ++o_) { for (int i_ = 0; i_ < (_ni_); ++i_) { buf[pos++] = (_v_)[o_][i_]; } } } while (0)
#define SG_1OOM_EN_TBL_TBL_U16(_v_, _no_, _ni_)  do { for (int o_ = 0; o_ < (_no_); ++o_) { for (int i_ = 0; i_ < (_ni_); ++i_) { SET_LE_16(&buf[pos], (_v_)[o_][i_]); pos += 2; } } } while (0)
#define SG_1OOM_EN_TBL_TBL_U32(_v_, _no_, _ni_)  do { for (int o_ = 0; o_ < (_no_); ++o_) { for (int i_ = 0; i_ < (_ni_); ++i_) { SET_LE_32(&buf[pos], (_v_)[o_][i_]); pos += 4; } } } while (0)

#define SG_1OOM_EN_DUMMY_V0(_n_)  if (LIBSAVE_1OOM_VERSION == 0) { SG_1OOM_EN_DUMMY(_n_); }

static int libsave_1oom_encode_player_id(uint8_t *buf, int pos, player_id_t player)
{
    if (LIBSAVE_1OOM_VERSION == 0) {
        if (player == PLAYER_NONE) {
            SG_1OOM_EN_U8(6);
        } else {
            SG_1OOM_EN_U8(player);
        }
    } else {
        SG_1OOM_EN_U16(player);
    }
    return pos;
}

#define SG_1OOM_EN_PLAYER_ID(_v_)  do { pos = libsave_1oom_encode_player_id(buf, pos, (_v_)); } while (0)

static int libsave_1oom_encode_hated_id(uint8_t *buf, int pos, player_id_t player)
{
    if (LIBSAVE_1OOM_VERSION == 0) {
        if (player == PLAYER_NONE) {
            SG_1OOM_EN_U16(6);
        } else {
            SG_1OOM_EN_U16(player);
        }
    } else {
        SG_1OOM_EN_U16(player);
    }
    return pos;
}

#define SG_1OOM_EN_HATED_ID(_v_)  do { pos = libsave_1oom_encode_hated_id(buf, pos, (_v_)); } while (0)

static int libsave_1oom_encode_planet_id(uint8_t *buf, int pos, planet_id_t pli)
{
    if (LIBSAVE_1OOM_VERSION == 0) {
        SG_1OOM_EN_U8(pli);
    } else {
        SG_1OOM_EN_U16(pli);
    }
    return pos;
}

#define SG_1OOM_EN_PLANET_ID(_v_)  do { pos = libsave_1oom_encode_planet_id(buf, pos, (_v_)); } while (0)

#endif
