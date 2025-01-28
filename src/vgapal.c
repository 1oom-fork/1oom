#include "config.h"

#include <string.h>

#include "vgapal.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

uint8_t vgapal[256 * 3] = {0};

/* -------------------------------------------------------------------------- */

void vgapal_set(const uint8_t *pal, uint16_t first, uint16_t num)
{
    memcpy(&vgapal[first * 3], pal, num * 3);
}
