#ifndef INC_1OOM_BITS_H
#define INC_1OOM_BITS_H

#include "types.h"

static inline void SET_LE_16(uint8_t *dst, uint16_t val)
{
    dst[0] = (uint8_t)(val & 0xffu);
    dst[1] = (uint8_t)((val >> 8) & 0xffu);
}

static inline void SET_LE_S16(uint8_t *dst, int16_t val)
{
    SET_LE_16(dst, (uint16_t)val);
}

static inline uint16_t GET_LE_16(const uint8_t *src)
{
    return ((uint16_t)(src[0])) | (((uint16_t)(src[1])) << 8);
}

static inline int16_t GET_LE_S16(const uint8_t *src)
{
    return (int16_t)GET_LE_16(src);
}

static inline void SET_LE_32(uint8_t *dst, uint32_t val)
{
    dst[0] = (uint8_t)(val & 0xffu);
    dst[1] = (uint8_t)((val >> 8) & 0xffu);
    dst[2] = (uint8_t)((val >> 16) & 0xffu);
    dst[3] = (uint8_t)((val >> 24) & 0xffu);
}

static inline void SET_LE_S32(uint8_t *dst, int32_t val)
{
    SET_LE_32(dst, (uint32_t)val);
}

static inline uint32_t GET_LE_32(const uint8_t *src)
{
    return ((uint32_t)GET_LE_16(src)) | (((uint32_t)GET_LE_16(src + 2)) << 16);
}

static inline int32_t GET_LE_S32(const uint8_t *src)
{
    return (int32_t)GET_LE_32(src);
}

static inline uint32_t GET_LE_24(const uint8_t *src)
{
    return ((uint32_t)GET_LE_16(src)) | (((uint32_t)(src[2])) << 16);
}

static inline void SET_BE_16(uint8_t *dst, uint16_t val)
{
    dst[1] = (uint8_t)(val & 0xffu);
    dst[0] = (uint8_t)((val >> 8) & 0xffu);
}

static inline uint16_t GET_BE_16(const uint8_t *src)
{
    return ((uint16_t)(src[1])) | (((uint16_t)(src[0])) << 8);
}

static inline void SET_BE_32(uint8_t *dst, uint32_t val)
{
    dst[3] = (uint8_t)(val & 0xffu);
    dst[2] = (uint8_t)((val >> 8) & 0xffu);
    dst[1] = (uint8_t)((val >> 16) & 0xffu);
    dst[0] = (uint8_t)((val >> 24) & 0xffu);
}

static inline uint32_t GET_BE_32(const uint8_t *src)
{
    return ((uint32_t)GET_BE_16(src + 2)) | (((uint32_t)GET_BE_16(src)) << 16);
}

#endif
