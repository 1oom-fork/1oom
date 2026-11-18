#ifndef INC_1OOM_OS_H
#define INC_1OOM_OS_H

/* API to os/ */

#include "config.h"

#include <stdio.h>

#include "options.h"
#include "types.h"

#ifdef IS_WINDOWS
#define FSDEV_PATH_MAX  2048
#define FSDEV_DIR_SEP_STR    "\\"
#define FSDEV_DIR_SEP_CHR    '\\'
#else
#define FSDEV_PATH_MAX  2048
#define FSDEV_DIR_SEP_STR    "/"
#define FSDEV_DIR_SEP_CHR    '/'
#endif

extern const char *idstr_os;

extern void os_shutdown(void);

extern const char **os_get_paths_data(void);
extern const char *os_get_path_data(void);
extern void os_set_path_data(const char *path);
extern const char *os_get_path_user(void);
extern int os_make_path_user(void);
extern int os_make_path_for(const char *filename);

extern uint32_t os_get_time_us(void);

#endif
