#ifndef INC_1OOM_OS_H
#define INC_1OOM_OS_H

/* API to os/ */

#include <stdio.h>

#include "osdefs.h"
#include "options.h"
#include "types.h"

extern const char *idstr_os;

extern int os_early_init(void);
extern int os_init(void);
extern void os_shutdown(void);

extern const struct cmdline_options_s os_cmdline_options[];

extern const char **os_get_paths_data(void);
extern const char *os_get_path_data(void);
extern void os_set_path_data(const char *path);
extern const char *os_get_path_user(void);
extern int os_make_path_user(void);
extern int os_make_path_for(const char *filename);

extern const char *os_get_fname_save(char *buf, int savei/*1..9*/);
extern const char *os_get_fname_cfg(char *buf, const char *gamestr, const char *uistr, const char *hwstr);

extern uint32_t os_get_time_us(void);

#endif
