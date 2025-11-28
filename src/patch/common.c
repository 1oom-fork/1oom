#include "common.h"

int bin_check(FILE *f, int offset, const uint8_t str[], int len)
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

int bin_check_nop(FILE *f, int offset, int len)
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

int bin_set_nop(FILE *f, int offset, int len)
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

int bin_replace(FILE *f, int offset, const uint8_t str[], int len)
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

bin_patch_status_t get_patch_status(FILE *f, const bin_patch_t *patch)
{
    int len = patch->offset_end - patch->offset_start;
    if (!bin_check(f, patch->offset_start, patch->match, len)) {
        return PATCH_STATUS_ORIGINAL;
    }
    else if (patch->replace) {
        if (!bin_check(f, patch->offset_start, patch->replace, len)) {
            return PATCH_STATUS_PATCHED;
        }
    } else {
        if (!bin_check_nop(f, patch->offset_start, len)) {
            return PATCH_STATUS_PATCHED;
        }
    }
    return PATCH_STATUS_INVALID;
}

void apply_patch(FILE *f, const bin_patch_t *patch)
{
    int len = patch->offset_end - patch->offset_start;
    if (patch->replace) {
        bin_replace(f, patch->offset_start, patch->replace, len);
    } else {
        bin_set_nop(f, patch->offset_start, len);
    }
}

void remove_patch(FILE *f, const bin_patch_t *patch)
{
    int len = patch->offset_end - patch->offset_start;
    bin_replace(f, patch->offset_start, patch->match, len);
}

bin_patch_status_t get_patch_set_status(FILE *f, const bin_patch_t *patch_set)
{
    bin_patch_status_t result = PATCH_STATUS_INVALID;
    for (int ci = 0; patch_set[ci].match != NULL; ++ci) {
        const bin_patch_t *patch = &patch_set[ci];
        const bin_patch_status_t status = get_patch_status(f, patch);
        if (status == PATCH_STATUS_INVALID) {
            return PATCH_STATUS_INVALID;
        }
        if (result == PATCH_STATUS_INVALID) {
            result = status;
        } else if (result != status) {
            return PATCH_STATUS_INVALID;
        }
    }
    return result;
}

void apply_patch_set(FILE *f, const bin_patch_t *patch_set)
{
    for (int ci = 0; patch_set[ci].match != NULL; ++ci) {
        apply_patch(f, &patch_set[ci]);
    }
}

void remove_patch_set(FILE *f, const bin_patch_t *patch_set)
{
    for (int ci = 0; patch_set[ci].match != NULL; ++ci) {
        remove_patch(f, &patch_set[ci]);
    }
}

bin_patch_status_t execute_patcher(const char *filename, const bin_patch_t *patch_set)
{
    FILE *f = fopen(filename, "r+b");
    bin_patch_status_t result = PATCH_STATUS_INVALID;
    if (!f) {
        printf("%s not found\n", filename);
        return PATCH_STATUS_INVALID;
    }
    const bin_patch_status_t status = get_patch_set_status(f, patch_set);
    if (status == PATCH_STATUS_INVALID) {
        result = PATCH_STATUS_INVALID;
        printf("Wrong file\n");
    } else if (status == PATCH_STATUS_PATCHED) {
        remove_patch_set(f, patch_set);
        result = PATCH_STATUS_ORIGINAL;
        printf("Patch removed\n");
    } else if (status == PATCH_STATUS_ORIGINAL) {
        apply_patch_set(f, patch_set);
        result = PATCH_STATUS_PATCHED;
        printf("Patch applied\n");
    }
    if (f) {
        fclose(f);
    }
    getchar();
    return result;
}
