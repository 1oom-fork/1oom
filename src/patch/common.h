#ifndef MOO1_PATCH_BIN_H
#define MOO1_PATCH_BIN_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

extern int bin_check(FILE *f, int offset, const uint8_t str[], int len);
extern int bin_check_nop(FILE *f, int offset, int len);
extern int bin_set_nop(FILE *f, int offset, int len);
extern int bin_replace(FILE *f, int offset, const uint8_t str[], int len);

typedef struct {
    const uint8_t *match;
    const uint8_t *replace;
    int offset_start;
    int offset_end;
} bin_patch_t;

typedef enum {
    PATCH_STATUS_INVALID = 0,
    PATCH_STATUS_ORIGINAL,
    PATCH_STATUS_PATCHED,
    PATCH_STATUS_NUM,
} bin_patch_status_t;

extern bin_patch_status_t get_patch_status(FILE *f, const bin_patch_t *patch);
extern void apply_patch(FILE *f, const bin_patch_t *patch);
extern void remove_patch(FILE *f, const bin_patch_t *patch);

extern bin_patch_status_t get_patch_set_status(FILE *f, const bin_patch_t *patch_set);
extern void apply_patch_set(FILE *f, const bin_patch_t *patch_set);
extern void remove_patch_set(FILE *f, const bin_patch_t *patch_set);

extern bin_patch_status_t execute_patcher(const char *filename, const bin_patch_t *patch_set);

#endif
