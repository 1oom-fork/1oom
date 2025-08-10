#ifndef INC_1OOM_UTIL_H
#define INC_1OOM_UTIL_H

#include "config.h"

#include <stdio.h>
#include <strings.h>

#include "types.h"

extern char *util_concat(const char *s, ...);
extern int util_concat_buf(char *buf, int buflen, ...);
extern int util_get_line(char *buf, int bufsize, FILE *f);
extern void util_fname_split(const char *path, char **directory_out, char **name_out);
extern int util_file_try_load_len(const char *name, uint8_t *buf, int wantlen);
extern int util_file_save(const char *name, const uint8_t *src, int size);
extern void util_trim_whitespace(char *str);
extern void util_str_tolower(char *str);
extern bool util_parse_number(const char *str, uint32_t *val_ptr);
extern int32_t *util_parse_numbers(const char *str, char sep, int *numptr);

#if !defined HAVE_STRCASECMP
extern int strcasecmp(const char *s1, const char *s2);
#endif

#endif
