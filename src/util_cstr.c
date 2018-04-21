#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util_cstr.h"
#include "lib.h"
#include "log.h"

/* -------------------------------------------------------------------------- */

int util_cstr_out(FILE *fd, const char *str)
{
    char c;
    while ((c = *str++) != 0) {
        if ((c == '\\') || (c == '"')) {
            if (fputc('\\', fd) == EOF) {
                return -1;
            }
        }
        if (fputc(c, fd) == EOF) {
            return -1;
        }
    }
    return 0;
}

int util_cstr_parse_in_place(char *str)
{
    char *p = str;
    char c;
    while ((c = *str++) != 0) {
        if (c == '"') {
            break;
        }
        if (c == '\\') {
            c = *str++;
            switch (c) {
                case '\\':
                case '"':
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 't':
                    c = '\t';
                    break;
                default:
                    log_error("%s: unhandled escape 0x%02x\n", __func__, c);
                    return -1;
            }
        }
        *p++ = c;
    }
    *p = 0;
    return 0;
}
