#ifndef INC_1OOM_UTIL_CSTR_H
#define INC_1OOM_UTIL_CSTR_H

#include <stdio.h>

extern int util_cstr_out(FILE *fd, const char *str);
extern int util_cstr_parse_in_place(char *str);

#endif
