#ifndef INC_1OOM_LIB_H
#define INC_1OOM_LIB_H

#include <stdlib.h>

extern void *lib_malloc(size_t size);
extern void *lib_realloc(void *p, size_t size);
extern void lib_free(void *ptr);
extern char *lib_stralloc(const char *str);

#endif
