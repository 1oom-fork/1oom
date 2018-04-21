#include "config.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

void *lib_malloc(size_t size)
{
    void *ptr = malloc(size);

    if ((ptr == NULL) && (size > 0)) {
        log_fatal_and_die("couldn't malloc %lu, out of memory!\n", size);
    }

    if (ptr) {
        memset(ptr, 0, size);
    }

    return ptr;
}

void *lib_realloc(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);

    if (new_ptr == NULL) {
        log_fatal_and_die("couldn't realloc %lu, out of memory!\n", size);
    }

    return new_ptr;
}

void lib_free(void *ptr)
{
    free(ptr);
}

char *lib_stralloc(const char *str)
{
    size_t size;
    char *ptr;

    if (str == NULL) {
        exit(EXIT_FAILURE);
    }

    size = strlen(str) + 1;
    ptr = lib_malloc(size);

    memcpy(ptr, str, size);
    return ptr;
}
