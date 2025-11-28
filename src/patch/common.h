#ifndef MOO1_PATCH_BIN_H
#define MOO1_PATCH_BIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    const uint8_t *match;
    const uint8_t *replace;
    int offset_start;
    int offset_end;
} bin_chunk_t;

typedef struct {
    const char *filename;
    const bin_chunk_t *data;
} bin_patch_t;

typedef enum {
    PATCH_STATUS_INVALID = 0,
    PATCH_STATUS_ORIGINAL,
    PATCH_STATUS_PATCHED,
    PATCH_STATUS_NUM,
} bin_patch_status_t;

extern bin_patch_status_t get_patch_set_status(const bin_patch_t patch_set[]);
extern void apply_patch_set(const bin_patch_t patch_set[]);
extern void remove_patch_set(const bin_patch_t patch_set[]);

#endif
