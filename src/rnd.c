#include "config.h"

#include "rnd.h"
#include "hw.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

uint16_t rnd_0_nm1(uint16_t n, uint32_t *seed)
{
    uint32_t r = *seed;
    r ^= (r << 13);
    r ^= (r >> 17);
    r ^= (r << 5);
    *seed = r;
    return (r >> 16) % n;
}

uint16_t rnd_1_n(uint16_t n, uint32_t *seed)
{
    return 1 + rnd_0_nm1(n, seed);
}

uint32_t rnd_get_new_seed(void)
{
    uint32_t seed;
    seed = hw_get_time_us();
    if (seed == 0) {
        seed = 123;
    }
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
