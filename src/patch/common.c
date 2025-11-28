#include "common.h"

static int bin_check(FILE *f, int offset, const uint8_t str[], int len)
{
    fseek(f, offset, SEEK_SET);
    if (feof(f) || ferror(f)) {
        printf("feof/ferror\n");
        return -1;
    }
    for (int i = 0; i < len; ++i) {
        uint8_t c = getc(f) & 0xff;
        if (feof(f) || ferror(f)) {
            printf("feof/ferror\n");
            return -1;
        }
        if (c != str[i]) {
            return -2;
        }
    }
    return 0;
}

static int bin_check_nop(FILE *f, int offset, int len)
{
    fseek(f, offset, SEEK_SET);
    if (feof(f) || ferror(f)) {
        printf("feof/ferror\n");
        return -1;
    }
    for (int i = 0; i < len; ++i) {
        uint8_t c = getc(f) & 0xff;
        if (feof(f) || ferror(f)) {
            printf("feof/ferror\n");
            return -1;
        }
        if (c != 0x90) {
            return -2;
        }
    }
    return 0;
}

static int bin_set_nop(FILE *f, int offset, int len)
{
    fseek(f, offset, SEEK_SET);
    if (feof(f) || ferror(f)) {
        printf("feof/ferror\n");
        return -1;
    }
    for (int i = 0; i < len; ++i) {
        putc(0x90, f);
        if (feof(f) || ferror(f)) {
            printf("feof/ferror\n");
            return -1;
        }
    }
    return 0;
}

static int bin_replace(FILE *f, int offset, const uint8_t str[], int len)
{
    fseek(f, offset, SEEK_SET);
    if (feof(f) || ferror(f)) {
        printf("feof/ferror\n");
        return -1;
    }
    for (int i = 0; i < len; ++i) {
        putc(str[i], f);
        if (feof(f) || ferror(f)) {
            printf("feof/ferror\n");
            return -1;
        }
    }
    return 0;
}

static bin_patch_status_t get_chunk_status(FILE *f, const bin_chunk_t *chunk)
{
    int len = chunk->offset_end - chunk->offset_start;
    if (!bin_check(f, chunk->offset_start, chunk->match, len)) {
        return PATCH_STATUS_ORIGINAL;
    }
    else if (chunk->replace) {
        if (!bin_check(f, chunk->offset_start, chunk->replace, len)) {
            return PATCH_STATUS_PATCHED;
        }
    } else {
        if (!bin_check_nop(f, chunk->offset_start, len)) {
            return PATCH_STATUS_PATCHED;
        }
    }
    return PATCH_STATUS_INVALID;
}

static void apply_chunk(FILE *f, const bin_chunk_t *chunk)
{
    int len = chunk->offset_end - chunk->offset_start;
    if (chunk->replace) {
        bin_replace(f, chunk->offset_start, chunk->replace, len);
    } else {
        bin_set_nop(f, chunk->offset_start, len);
    }
}

static void revert_chunk(FILE *f, const bin_chunk_t *chunk)
{
    int len = chunk->offset_end - chunk->offset_start;
    bin_replace(f, chunk->offset_start, chunk->match, len);
}

static bin_patch_status_t get_patch_status(const bin_patch_t *patch)
{
    FILE *f = fopen(patch->filename, "r+b");
    if (!f) {
        printf("%s not found\n", patch->filename);
        return PATCH_STATUS_INVALID;
    }
    bin_patch_status_t result = PATCH_STATUS_INVALID;
    for (int ci = 0; patch->data[ci].match != NULL; ++ci) {
        const bin_chunk_t *chunk = &patch->data[ci];
        const bin_patch_status_t status = get_chunk_status(f, chunk);
        if (status == PATCH_STATUS_INVALID) {
            result = PATCH_STATUS_INVALID;
            break;
        }
        if (result == PATCH_STATUS_INVALID) {
            result = status;
        }
        if (result != status) {
            result = PATCH_STATUS_INVALID;
            break;
        }
    }
    if (f) {
        fclose(f);
        f = NULL;
    }
    return result;
}

static void apply_patch(const bin_patch_t *patch)
{
    FILE *f = fopen(patch->filename, "r+b");
    if (!f) {
        printf("%s not found\n", patch->filename);
        return;
    }
    for (int ci = 0; patch->data[ci].match != NULL; ++ci) {
        apply_chunk(f, &patch->data[ci]);
    }
    if (f) {
        fclose(f);
        f = NULL;
    }
}

static void remove_patch(const bin_patch_t *patch)
{
    FILE *f = fopen(patch->filename, "r+b");
    if (!f) {
        printf("%s not found\n", patch->filename);
        return;
    }
    for (int ci = 0; patch->data[ci].match != NULL; ++ci) {
        revert_chunk(f, &patch->data[ci]);
    }
    if (f) {
        fclose(f);
        f = NULL;
    }
}

/*-------------------------------------------------------------------------------*/

bin_patch_status_t get_patch_set_status(const bin_patch_t patch_set[])
{
    bin_patch_status_t result = PATCH_STATUS_INVALID;
    for (int pi = 0; patch_set[pi].filename != NULL; ++pi) {
        bin_patch_status_t next_status = get_patch_status(&patch_set[pi]);
        if (next_status == PATCH_STATUS_INVALID) {
            return PATCH_STATUS_INVALID;
        }
        if (result == PATCH_STATUS_INVALID) {
            result = next_status;
        }
        if (next_status != result) {
            return PATCH_STATUS_INVALID;
        }
    }
    return result;
}

void apply_patch_set(const bin_patch_t patch_set[])
{
    for (int pi = 0; patch_set[pi].filename != NULL; ++pi) {
        apply_patch(&patch_set[pi]);
    }
}

void remove_patch_set(const bin_patch_t patch_set[])
{
    for (int pi = 0; patch_set[pi].filename != NULL; ++pi) {
        remove_patch(&patch_set[pi]);
    }
}
