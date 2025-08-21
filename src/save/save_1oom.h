#ifndef INC_1OOM_SAVE_1OOM_H
#define INC_1OOM_SAVE_1OOM_H

#include "types.h"
#include "game_types.h"

#define LIBSAVE_1OOM_HDR_SIZE  64
#define LIBSAVE_1OOM_DATA_SIZE (sizeof(struct game_s) + 64)
#define LIBSAVE_1OOM_MAGIC "1oomSAVE"
#define LIBSAVE_1OOM_END   0x646e450a/*dnE\n*/
#define LIBSAVE_1OOM_OFFS_VERSION  8
#define LIBSAVE_1OOM_OFFS_NAME 16

#define LIBSAVE_1OOM_VERSION   1

#endif
