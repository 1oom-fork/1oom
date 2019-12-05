#include "config.h"

#include "rnd.h"
#include "hw.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

uint64_t rnd_entropy_pool; /* uninitialized on purpose - it IS an entropy pool after all */

uint32_t rnd32(uint32_t *seed) {
    uint32_t r = *seed;
    r ^= (r << 13);
    r ^= (r >> 17);
    r ^= (r << 5);
    *seed = r;
    return r;
}

uint32_t rnd32alt(uint32_t *seed) {
    uint32_t r = *seed;
    r ^= (r << 7);
    r ^= (r >> 25);
    r ^= (r << 12);
    *seed = r;
    return r;
}

uint32_t rnd_get_new_seed(void)
{
    uint32_t seed;
    do {
        rnd_entropy_pool += hw_get_time_us();
        rnd_entropy_pool *= 718874301;
        rnd_entropy_pool ^= rnd_entropy_pool >> 29;
        seed = (uint32_t)rnd_entropy_pool;
    } while(!seed);
    return seed;
}

uint16_t rnd_bitfiddle(uint16_t ax)
{
    if (ax == 0) {
        return 0x35c8;
    }
    int loops = 8;
    uint16_t bx, dx;
    do {
        dx = ax;
        bx = ax;
        bx >>= 1;
        ax ^= bx;
        bx >>= 1;
        ax ^= bx;
        bx >>= 2;
        ax ^= bx;
        bx >>= 2;
        ax ^= bx;
        bx >>= 1;
        ax ^= bx;
        dx >>= 1;
        if (ax & 1) { dx |= 0x8000; }
        ax = dx;
    } while (--loops);
    return ax;
}
