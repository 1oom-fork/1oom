#include "config.h"

#include "vgabuf.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

uint8_t vgabuf[NUM_VIDEOBUF * VGABUF_SIZE_EXT] = {0};
