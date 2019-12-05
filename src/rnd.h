#ifndef INC_1OOM_RND_H
#define INC_1OOM_RND_H

#include "types.h"

extern uint64_t rnd_entropy_pool;

extern uint32_t rnd32(uint32_t *seed);
extern uint32_t rnd32alt(uint32_t *seed);
extern uint16_t rnd_0_nm1(uint16_t n, uint32_t *seed);
extern uint16_t rnd_1_n(uint16_t n, uint32_t *seed);
extern uint32_t rnd_get_new_seed(void);
extern uint16_t rnd_bitfiddle(uint16_t ax);

/* define as macros to allow for static optimization 
 * extern uint16_t rnd_0_nm1(uint16_t n, uint32_t *seed);
 * extern uint16_t rnd_1_n(uint16_t n, uint32_t *seed); */

#define rnd_0_nm1(n,s) ((uint16_t)(rnd32((s)) % (uint32_t)(n)))
#define rnd_1_n(n,s) ((uint16_t)(1 + rnd32((s)) % (uint32_t)(n)))

#endif
